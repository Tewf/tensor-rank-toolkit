#include "pool_orbits.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "candidate_pool.h"

namespace bilinear_rank {

namespace {

using Vector = std::vector<int64_t>;

/// `μᵀ u`, normalised so its leading nonzero entry is one, which is how the
/// vector lists are held.
Vector moved(const Field& field, const Matrix& half, const Vector& vector) {
    Vector image(vector.size(), 0);
    for (std::size_t row = 0; row < vector.size(); ++row) {
        if (field.isZero(vector[row])) continue;
        for (std::size_t column = 0; column < vector.size(); ++column) {
            field.axpyin(image[column], vector[row], half(row, column));
        }
    }
    for (std::size_t index = 0; index < image.size(); ++index) {
        if (field.isZero(image[index])) continue;
        Element scale;
        field.inv(scale, image[index]);
        for (std::size_t rest = index; rest < image.size(); ++rest) {
            field.mulin(image[rest], scale);
        }
        break;
    }
    return image;
}

std::string key_of(const Vector& vector) {
    return std::string(reinterpret_cast<const char*>(vector.data()),
                       vector.size() * sizeof(int64_t));
}

/// How one half of each automorphism permutes one vector list.
std::vector<std::vector<std::uint32_t>> permutations_of(const Field& field,
                                                        const std::vector<Matrix>& halves,
                                                        const std::vector<Vector>& vectors) {
    std::unordered_map<std::string, std::uint32_t> index_of;
    for (std::uint32_t index = 0; index < vectors.size(); ++index) {
        index_of.emplace(key_of(vectors[index]), index);
    }

    std::vector<std::vector<std::uint32_t>> permutations;
    permutations.reserve(halves.size());
    for (const Matrix& half : halves) {
        std::vector<std::uint32_t> images(vectors.size());
        for (std::size_t index = 0; index < vectors.size(); ++index) {
            const auto found = index_of.find(key_of(moved(field, half, vectors[index])));
            if (found == index_of.end()) {
                throw std::runtime_error("an automorphism sent a vector outside the list");
            }
            images[index] = found->second;
        }
        permutations.push_back(std::move(images));
    }
    return permutations;
}

/// Orbits of `0 .. count-1` under the permutations, lowest index first, and the
/// transversal that reaches each element from its representative.
struct Orbits {
    std::vector<std::uint32_t> representatives;
    std::vector<std::uint32_t> of;        // which representative each index belongs to
    std::vector<std::vector<std::uint32_t>> word;  // generators applied, in order, to get there
};

Orbits orbits_under(const std::vector<std::vector<std::uint32_t>>& permutations,
                    std::size_t count) {
    Orbits orbits;
    orbits.of.assign(count, static_cast<std::uint32_t>(-1));
    orbits.word.assign(count, {});

    for (std::uint32_t start = 0; start < count; ++start) {
        if (orbits.of[start] != static_cast<std::uint32_t>(-1)) continue;
        orbits.representatives.push_back(start);
        orbits.of[start] = start;

        std::vector<std::uint32_t> frontier{start};
        while (!frontier.empty()) {
            const std::uint32_t reached = frontier.back();
            frontier.pop_back();
            for (std::size_t element = 0; element < permutations.size(); ++element) {
                const std::uint32_t image = permutations[element][reached];
                if (orbits.of[image] != static_cast<std::uint32_t>(-1)) continue;
                orbits.of[image] = start;
                orbits.word[image] = orbits.word[reached];
                orbits.word[image].push_back(static_cast<std::uint32_t>(element));
                frontier.push_back(image);
            }
        }
    }
    return orbits;
}

/// The generators applied in order, as one automorphism.
Automorphism along(const Field& field, const std::vector<Automorphism>& group,
                   const std::vector<std::uint32_t>& word, std::size_t rows,
                   std::size_t columns) {
    Automorphism walked = identity_automorphism(field, rows, columns);
    for (const std::uint32_t element : word) walked = compose(field, walked, group[element]);
    return walked;
}

}  // namespace

FactoredAction factored_action(const Field& field, const std::vector<Automorphism>& group,
                               std::size_t rows, std::size_t columns) {
    std::vector<Matrix> lefts, rights;
    lefts.reserve(group.size());
    rights.reserve(group.size());
    for (const Automorphism& sigma : group) {
        lefts.push_back(sigma.left);
        rights.push_back(sigma.right);
    }
    return {permutations_of(field, lefts, normalised_vectors(field, rows)),
            permutations_of(field, rights, normalised_vectors(field, columns))};
}

std::vector<std::uint32_t> pool_orbit_representatives(const Field& field,
                                                      const std::vector<Automorphism>& group,
                                                      const FactoredAction& action,
                                                      std::size_t rows, std::size_t columns,
                                                      std::size_t left_count,
                                                      std::size_t right_count) {
    const Orbits left_orbits = orbits_under(action.left, left_count);
    std::vector<std::uint32_t> representatives;

    for (const std::uint32_t left : left_orbits.representatives) {
        // Schreier's lemma. The generators that happen to fix this vector do
        // **not** generate its stabiliser, and using them alone splits orbits
        // that are really one: 41 where there are 13 at <3,3,3>. The stabiliser
        // is generated by `t(g·x)⁻¹ · g · t(x)` over every orbit element `x` and
        // every generator `g`, where `t` walks from the representative.
        std::vector<std::vector<std::uint32_t>> fixing;
        for (std::uint32_t point = 0; point < left_count; ++point) {
            if (left_orbits.of[point] != left) continue;
            const Automorphism to_point = along(field, group, left_orbits.word[point], rows, columns);

            for (std::size_t element = 0; element < group.size(); ++element) {
                const std::uint32_t image = action.left[element][point];
                const Automorphism to_image =
                    along(field, group, left_orbits.word[image], rows, columns);
                const Automorphism schreier = compose(
                    field, compose(field, to_point, group[element]), inverse(field, to_image));
                fixing.push_back(
                    factored_action(field, {schreier}, rows, columns).right.front());
            }
        }
        for (const std::uint32_t right : orbits_under(fixing, right_count).representatives) {
            representatives.push_back(static_cast<std::uint32_t>(left * right_count + right));
        }
    }
    return representatives;
}

std::vector<Matrix> rank_one_orbit_representatives(const Field& field,
                                                   const std::vector<Automorphism>& group,
                                                   std::size_t rows, std::size_t columns) {
    const std::vector<Vector> lefts = normalised_vectors(field, rows);
    const std::vector<Vector> rights = normalised_vectors(field, columns);
    if (group.empty()) throw std::runtime_error("no group to quotient by");

    const FactoredAction action = factored_action(field, group, rows, columns);

    std::vector<Matrix> representatives;
    for (const std::uint32_t index : pool_orbit_representatives(
             field, group, action, rows, columns, lefts.size(), rights.size())) {
        const Vector& left = lefts[index / rights.size()];
        const Vector& right = rights[index % rights.size()];

        Matrix map(rows, columns);
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t column = 0; column < columns; ++column) {
                field.mul(map(row, column), left[row], right[column]);
            }
        }
        representatives.push_back(std::move(map));
    }
    return representatives;
}

std::vector<std::pair<std::vector<Element>, std::vector<Element>>>
matrix_multiplication_orbit_vectors(const Field& field, std::size_t rows, std::size_t inner,
                                    std::size_t columns) {
    std::vector<std::pair<std::vector<Element>, std::vector<Element>>> representatives;

    for (std::size_t left_rank = 1; left_rank <= std::min(rows, inner); ++left_rank) {
        for (std::size_t right_rank = 1; right_rank <= std::min(inner, columns); ++right_rank) {
            const std::size_t lowest =
                left_rank + right_rank > inner ? left_rank + right_rank - inner : 0;
            for (std::size_t through = lowest; through <= std::min(left_rank, right_rank);
                 ++through) {
                // U is the identity on its first `left_rank` coordinates.
                Matrix u(rows, inner);
                for (std::size_t index = 0; index < left_rank; ++index) {
                    field.assign(u(index, index), field.one);
                }

                // V puts `through` of its rows inside what U keeps and the rest
                // outside, which is exactly what makes rank(UV) come out right.
                Matrix v(inner, columns);
                for (std::size_t index = 0; index < through; ++index) {
                    field.assign(v(index, index), field.one);
                }
                for (std::size_t index = 0; index + through < right_rank; ++index) {
                    field.assign(v(left_rank + index, through + index), field.one);
                }

                representatives.emplace_back(
                    std::vector<Element>(u.data(), u.data() + u.entry_count()),
                    std::vector<Element>(v.data(), v.data() + v.entry_count()));
            }
        }
    }
    return representatives;
}

std::vector<Matrix> matrix_multiplication_pool_orbits(const Field& field, std::size_t rows,
                                                      std::size_t inner, std::size_t columns) {
    std::vector<Matrix> representatives;
    for (const auto& [left, right] :
         matrix_multiplication_orbit_vectors(field, rows, inner, columns)) {
        Matrix map(left.size(), right.size());
        for (std::size_t row = 0; row < left.size(); ++row) {
            if (field.isZero(left[row])) continue;
            for (std::size_t column = 0; column < right.size(); ++column) {
                field.mul(map(row, column), left[row], right[column]);
            }
        }
        representatives.push_back(std::move(map));
    }
    return representatives;
}

}  // namespace bilinear_rank
