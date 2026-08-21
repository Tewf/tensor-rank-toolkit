#include "automorphism.h"

#include "pool_orbits.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "matrix_ops.h"
#include "solver.h"
#include "memory_budget.h"
#include "span_queries.h"

namespace bilinear_rank {

namespace {

/// The entries of both halves, read as bytes, which is what tells two
/// automorphisms apart when closing a group.
std::string fingerprint(const Automorphism& sigma) {
    std::string bytes;
    bytes.reserve((sigma.left.entry_count() + sigma.right.entry_count()) * sizeof(Element));
    for (const Matrix* half : {&sigma.left, &sigma.right}) {
        const char* start = reinterpret_cast<const char*>(half->data());
        bytes.append(start, half->entry_count() * sizeof(Element));
    }
    return bytes;
}

}  // namespace

Matrix act_on(const Field& field, const Automorphism& sigma, const Matrix& form) {
    const Matrix transposed = linear_algebra::transpose<Field>(sigma.left);
    return linear_algebra::multiply(field, linear_algebra::multiply(field, transposed, form),
                                    sigma.right);
}

Automorphism identity_automorphism(const Field& field, std::size_t rows, std::size_t columns) {
    Automorphism sigma{Matrix(rows, rows), Matrix(columns, columns)};
    for (std::size_t index = 0; index < rows; ++index) {
        field.assign(sigma.left(index, index), field.one);
    }
    for (std::size_t index = 0; index < columns; ++index) {
        field.assign(sigma.right(index, index), field.one);
    }
    return sigma;
}

Automorphism compose(const Field& field, const Automorphism& first, const Automorphism& second) {
    return {linear_algebra::multiply(field, first.left, second.left),
            linear_algebra::multiply(field, first.right, second.right)};
}

Automorphism inverse(const Field& field, const Automorphism& sigma) {
    Automorphism undone;
    if (!linear_algebra::invert(field, sigma.left, undone.left) ||
        !linear_algebra::invert(field, sigma.right, undone.right)) {
        throw std::runtime_error("an automorphism was not invertible");
    }
    return undone;
}

std::vector<Automorphism> group_closure(const Field& field,
                                        const std::vector<Automorphism>& generators) {
    if (generators.empty()) return {};

    std::vector<Automorphism> group{identity_automorphism(
        field, generators.front().left.rows(), generators.front().right.rows())};
    std::unordered_set<std::string> seen{fingerprint(group.front())};

    for (std::size_t frontier = 0; frontier < group.size(); ++frontier) {
        for (const Automorphism& generator : generators) {
            Automorphism reached = compose(field, group[frontier], generator);
            if (!seen.insert(fingerprint(reached)).second) continue;

            require_room("the group being closed", group.size() + 1,
                         bytes_per_matrix(reached.left.entry_count()) +
                             bytes_per_matrix(reached.right.entry_count()));
            group.push_back(std::move(reached));
        }
    }
    return group;
}

std::vector<Automorphism> stabiliser_of(const Field& field, const std::vector<Matrix>& slices,
                                        const std::vector<Automorphism>& group) {
    std::vector<Automorphism> stabiliser;
    for (const Automorphism& sigma : group) {
        std::vector<Matrix> moved;
        moved.reserve(slices.size());
        for (const Matrix& slice : slices) moved.push_back(act_on(field, sigma, slice));

        // Setwise: the image has to span what it came from, not to match slice
        // by slice. Both directions, since a map into a space of equal dimension
        // is onto it, and the dimensions are equal because the action is
        // invertible.
        if (linear_algebra::spans_all(field, moved, slices)) stabiliser.push_back(sigma);
    }
    return stabiliser;
}

Matrix scalar_class_representative(const Field& field, Matrix matrix) {
    for (std::size_t entry = 0; entry < matrix.entry_count(); ++entry) {
        if (field.isZero(matrix.data()[entry])) continue;

        Element scale;
        field.inv(scale, matrix.data()[entry]);
        for (std::size_t rest = entry; rest < matrix.entry_count(); ++rest) {
            field.mulin(matrix.data()[rest], scale);
        }
        return matrix;
    }
    return matrix;
}

std::vector<std::vector<std::uint32_t>> permutation_action_on(const Field& field,
                                                  const std::vector<Automorphism>& group,
                                                  const std::vector<Matrix>& pool) {
    std::unordered_map<std::string, std::uint32_t> index_of;
    index_of.reserve(pool.size());
    for (std::uint32_t index = 0; index < pool.size(); ++index) {
        const char* start = reinterpret_cast<const char*>(pool[index].data());
        index_of.emplace(std::string(start, pool[index].entry_count() * sizeof(Element)), index);
    }

    require_room("the group's action on the pool", group.size() * pool.size(),
                 sizeof(std::uint32_t));

    std::vector<std::vector<std::uint32_t>> permutations;
    permutations.reserve(group.size());
    for (const Automorphism& sigma : group) {
        std::vector<std::uint32_t> images(pool.size());
        for (std::size_t index = 0; index < pool.size(); ++index) {
            const Matrix moved =
                scalar_class_representative(field, act_on(field, sigma, pool[index]));
            const char* start = reinterpret_cast<const char*>(moved.data());
            const auto found =
                index_of.find(std::string(start, moved.entry_count() * sizeof(Element)));
            if (found == index_of.end()) {
                throw std::runtime_error(
                    "the pool is not closed under this group: an image left it");
            }
            images[index] = found->second;
        }
        permutations.push_back(std::move(images));
    }
    return permutations;
}

namespace {

/// One walk, written once, over whichever way the action is presented.
template <class Action>
std::vector<std::uint32_t> representatives_under(const Action& next, std::size_t elements,
                                                 const std::vector<std::uint32_t>& candidates) {
    if (elements == 0) return candidates;

    std::unordered_set<std::uint32_t> here(candidates.begin(), candidates.end());
    std::unordered_set<std::uint32_t> spoken_for;

    std::vector<std::uint32_t> representatives;
    for (const std::uint32_t candidate : candidates) {
        if (spoken_for.count(candidate) != 0) continue;
        representatives.push_back(candidate);

        // Breadth first, so the action may be a generating set rather than the
        // whole group. That is what lets this run at sizes where the group has
        // millions of elements and only nine of them can be held.
        std::vector<std::uint32_t> frontier{candidate};
        spoken_for.insert(candidate);
        while (!frontier.empty()) {
            const std::uint32_t reached = frontier.back();
            frontier.pop_back();
            for (std::size_t element = 0; element < elements; ++element) {
                const std::uint32_t image = next(element, reached);
                if (here.count(image) == 0) continue;
                if (spoken_for.insert(image).second) frontier.push_back(image);
            }
        }
    }
    return representatives;
}

}  // namespace

std::vector<std::uint32_t> orbit_representatives(const PoolAction& action,
                                                 const std::vector<std::uint32_t>& candidates) {
    return representatives_under(
        [&action](std::size_t element, std::uint32_t point) { return action.image(element, point); },
        action.size(), candidates);
}

std::vector<std::uint32_t> orbit_representatives(const std::vector<std::vector<std::uint32_t>>& action,
                                         const std::vector<std::uint32_t>& candidates) {
    if (action.empty()) return candidates;

    std::unordered_set<std::uint32_t> here(candidates.begin(), candidates.end());
    std::unordered_set<std::uint32_t> spoken_for;

    std::vector<std::uint32_t> representatives;
    for (const std::uint32_t candidate : candidates) {
        if (spoken_for.count(candidate) != 0) continue;
        representatives.push_back(candidate);

        // Breadth first, so `action` may be a generating set rather than the
        // whole group. That is what lets this run at sizes where the group has
        // millions of elements and only nine of them can be held.
        std::vector<std::uint32_t> frontier{candidate};
        spoken_for.insert(candidate);
        while (!frontier.empty()) {
            const std::uint32_t reached = frontier.back();
            frontier.pop_back();
            for (const std::vector<std::uint32_t>& permutation : action) {
                const std::uint32_t image = permutation[reached];
                if (here.count(image) == 0) continue;
                if (spoken_for.insert(image).second) frontier.push_back(image);
            }
        }
    }
    return representatives;
}

}  // namespace bilinear_rank
