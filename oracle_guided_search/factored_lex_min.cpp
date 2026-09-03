#include "factored_lex_min.h"

#include "axis_presentation.h"

// Before any PermLib header: PermLib calls `boost::next` in three places and
// includes nothing that declares it, and a qualified name inside a template is
// looked up where the template is defined. `vendor/permlib/README.md` says so.
#include <boost/next_prior.hpp>

// PermLib reaches a Boost header that announces its own deprecation with a
// `#pragma message`. A SYSTEM include directory silences warnings and not
// pragmas, so this is the only way to keep the build quiet, and a build that
// prints one line nobody can act on is how a build comes to print twenty.
#define BOOST_ALLOW_DEPRECATED_HEADERS

#include <permlib/permlib_api.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "memory_budget.h"

namespace bilinear_rank {

namespace {

using axes::BaseChange;
using axes::Group;
using axes::Perm;
using axes::Transversal;

/// One PermLib permutation of `L ⊔ R` from one factored generator: a left vector
/// keeps its index, right vector `r` becomes the point `row_count + r`.
Perm::ptr on_axes(const FactoredGenerator& generator, std::size_t row_count,
                  std::size_t column_count) {
    if (generator.left.size() != row_count || generator.right.size() != column_count) {
        throw std::runtime_error(
            "a factored generator does not permute the vector lists it was given with");
    }
    Perm::perm image(row_count + column_count);
    for (std::size_t row = 0; row < row_count; ++row) {
        image[row] = static_cast<permlib::dom_int>(generator.left[row]);
    }
    for (std::size_t column = 0; column < column_count; ++column) {
        image[row_count + column] =
            static_cast<permlib::dom_int>(row_count + generator.right[column]);
    }
    return Perm::ptr(new Perm(image));
}

/// Two images of one set are compared by their sorted cells, entry by entry. Every
/// image has the same length, so `std::vector`'s own order is that order.
const std::vector<std::size_t>& least_of(
    const std::vector<std::vector<std::size_t>>& candidates) {
    if (candidates.empty()) {
        // Unreachable by the argument in `LeastImageSearch`: every step keeps the
        // candidates attaining its minimum, and a candidate that attained it has a
        // cell to branch on. Said out loud rather than left to a null dereference,
        // because a canonical form that quietly stopped covering the orbit is the
        // one failure nothing downstream catches.
        throw std::runtime_error("the smallest-image search lost every candidate");
    }
    return *std::min_element(candidates.begin(), candidates.end());
}

/// Two branches can carry one candidate onto the same image; keeping one of each
/// bounds the list without changing what it covers, because everything a candidate
/// does from here on is a function of its cells and of the answer so far.
void drop_repeats(std::vector<std::vector<std::size_t>>& candidates) {
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
}

/// `[linton2004]`'s smallest-image search, run on the product action `L × R` of a
/// group presented on `L ⊔ R`.
///
/// The algorithm is unchanged and worth restating in the terms it is used in here.
/// The answer is built one cell at a time, least first. At stage `i` the first `i`
/// cells `m_0 < … < m_{i-1}` of the least image are known, and a list of
/// **candidates** is held, each an image of the set whose `i` least cells are
/// exactly those, such that every image with that prefix is `h(c)` for some
/// candidate `c` and some `h` in `G^(i)`, the pointwise stabiliser of
/// `m_0 … m_{i-1}`. By the identity in
/// [`factored_lex_min.h`](factored_lex_min.h) that group is the pointwise
/// stabiliser of the rows and columns of those cells, which is a prefix of a base
/// of `G` on `L ⊔ R`. So `m_i` is the least cell any candidate can put next, the
/// candidates attaining it are kept, and each is branched over the transversal
/// elements that put one of its cells there.
///
/// **What the grid presentation did in one step this does in two.** The flat order
/// `left * right_count + right` is lexicographic in `(row, column)`, so the least
/// next cell has the least attainable row, and among those the least attainable
/// column. Minimising the row is an orbit computation in `L` alone; minimising the
/// column, once the row is committed and the base extended by it, one in `R` alone.
/// Cells of one row are contiguous in the flat order, so the row commit is shared
/// by all of them: a **row step** whenever the open row has run out of uncommitted
/// cells, a **column step** for each cell of it. Same stabiliser chain, read at the
/// granularity the two axes offer rather than at the grid's.
///
/// **Why the two-step is the same minimum.** Write `least_row` for the row a row
/// step commits. Every `h` in `G^(i)` sends every uncommitted cell to a row at
/// least `least_row`, so no image with the prefix has an earlier cell; and an image
/// whose next cell lies in `least_row` is `h'(u(c))` with `u` the transversal
/// element that put a cell of `c` in that row and `h'` in `G^(i+1)`, since `h' =
/// h u⁻¹` fixes `least_row`. So minimising the column over `G^(i+1)` and over the
/// branched candidates ranges over exactly the columns the full minimisation
/// ranges over. The same argument, one axis later, is why a column step may commit
/// a column already in the base without extending it: `G^(i)` fixes such a point,
/// so the only candidates that can attain it already have a cell there.
///
/// **Why the answer's rows never go backwards.** A row step happens only when no
/// candidate has an uncommitted cell in the open row `m`, and `m` is a base point,
/// so `G^(i)` fixes it and no other row's orbit can reach it; every uncommitted
/// cell is past the last committed one, so its row is at least `m`, hence past `m`.
/// The minimum a row step finds is therefore strictly greater than the row before
/// it, which is what makes the committed cells an increasing sequence and the
/// candidate prefixes stable.
///
/// A candidate is a **sorted vector of flat cell indices** and nothing marks which
/// of them are committed, because the committed ones are exactly the leading `i`:
/// an image kept at stage `i` has `m_0 … m_{i-1}` as its `i` least cells, by the
/// invariant that defines the list.
class LeastImageSearch {
   public:
    LeastImageSearch(std::size_t row_count, std::size_t column_count, const Group& group)
        : row_count_(row_count),
          column_count_(column_count),
          group_(group),
          change_(group_),
          in_base_(group_.n, 0),
          stamp_(group_.n, 0),
          cached_at_(group_.n, 0),
          cache_(group_.n, 0),
          branched_(group_.n, 0) {}

    std::vector<std::size_t> of(std::vector<std::size_t> cells);

   private:
    permlib::dom_int row_of(std::size_t cell) const {
        return static_cast<permlib::dom_int>(cell / column_count_);
    }
    permlib::dom_int column_point_of(std::size_t cell) const {
        return static_cast<permlib::dom_int>(row_count_ + cell % column_count_);
    }
    std::size_t flat(std::size_t row, std::size_t column) const {
        return row * column_count_ + column;
    }

    bool open_row_has_work(const std::vector<std::vector<std::size_t>>& candidates,
                           std::size_t placed, permlib::dom_int row) const;
    permlib::dom_int commit_row(std::vector<std::vector<std::size_t>>& candidates,
                                std::size_t placed);
    permlib::dom_int commit_column(std::vector<std::vector<std::size_t>>& candidates,
                                   std::size_t placed, permlib::dom_int row, bool last);

    /// A cell this step is not minimising over, so no branch comes from it. No axis
    /// point can collide with it: the domain is `row_count + column_count`, and
    /// `⟨4,4,4⟩`'s 131 070 is nowhere near the point type's ceiling.
    static constexpr permlib::dom_int kOffAxis = std::numeric_limits<permlib::dom_int>::max();

    /// Commit `point` and rebuild the candidate list from the branches that put an
    /// uncommitted cell of `passing` there. `axis` reads the point a cell offers,
    /// or `kOffAxis` for a cell this step does not look at.
    template <class Axis>
    void commit(std::vector<std::vector<std::size_t>>& candidates,
                const std::vector<const std::vector<std::size_t>*>& passing,
                std::size_t placed, permlib::dom_int point, const Axis& axis);

    /// The strong generators fixing the committed base pointwise: a generating set
    /// of the subgroup still free to move an uncommitted cell.
    std::vector<Perm::ptr> still_free() const;
    /// The least point in the orbit of `point`, memoised for the current level.
    /// `floor` is the least point the orbit could possibly reach, so reaching it
    /// ends the walk early.
    permlib::dom_int orbit_least(permlib::dom_int point, const std::vector<Perm::ptr>& free,
                                 permlib::dom_int floor);
    std::vector<std::size_t> moved(const Perm& element,
                                   const std::vector<std::size_t>& cells) const;
    /// True when the stabiliser of the committed base is trivial, so no candidate
    /// can move again and the least of them is the answer.
    bool settled() const;
    void extend_base(permlib::dom_int point);

    std::size_t row_count_;
    std::size_t column_count_;
    Group group_;
    BaseChange change_;
    std::vector<unsigned long> base_;
    std::vector<char> in_base_;

    // Orbit scratch, stamped rather than cleared: a generation counter costs one
    // compare where a reset costs the whole domain, and the domain is walked once
    // per uncommitted cell per level. The counters start at zero for each search
    // because the object does, so none of them can wrap into a stale stamp.
    std::vector<std::uint32_t> stamp_;
    std::vector<permlib::dom_int> frontier_;
    std::uint32_t generation_ = 0;
    std::vector<std::uint32_t> cached_at_;
    std::vector<permlib::dom_int> cache_;
    std::uint32_t level_ = 0;
    std::vector<std::uint32_t> branched_;
    std::uint32_t branch_ = 0;
};

std::vector<std::size_t> LeastImageSearch::of(std::vector<std::size_t> cells) {
    std::sort(cells.begin(), cells.end());
    cells.erase(std::unique(cells.begin(), cells.end()), cells.end());
    const std::size_t count = cells.size();

    std::vector<std::vector<std::size_t>> candidates{std::move(cells)};
    std::vector<std::size_t> answer;
    answer.reserve(count);

    bool row_open = false;
    permlib::dom_int row = 0;
    for (std::size_t placed = 0; placed < count; ++placed) {
        if (settled()) return least_of(candidates);
        if (!row_open || !open_row_has_work(candidates, placed, row)) {
            row = commit_row(candidates, placed);
            row_open = true;
        }
        answer.push_back(flat(row, commit_column(candidates, placed, row, placed + 1 == count)));
    }
    return answer;
}

bool LeastImageSearch::open_row_has_work(
    const std::vector<std::vector<std::size_t>>& candidates, std::size_t placed,
    permlib::dom_int row) const {
    for (const std::vector<std::size_t>& candidate : candidates) {
        for (std::size_t which = placed; which < candidate.size(); ++which) {
            if (row_of(candidate[which]) == row) return true;
        }
    }
    return false;
}

permlib::dom_int LeastImageSearch::commit_row(
    std::vector<std::vector<std::size_t>>& candidates, std::size_t placed) {
    const std::vector<Perm::ptr> free = still_free();
    ++level_;

    // `row_count_` stands above every row and is never attained, so a candidate
    // with nothing left to place could not win. At a row step every candidate has
    // something left to place: they all hold the same number of cells and fewer
    // than that are committed, so nothing is excluded by the sentinel here.
    permlib::dom_int least = static_cast<permlib::dom_int>(row_count_);
    std::vector<const std::vector<std::size_t>*> passing;
    for (const std::vector<std::size_t>& candidate : candidates) {
        permlib::dom_int here = static_cast<permlib::dom_int>(row_count_);
        for (std::size_t which = placed; which < candidate.size(); ++which) {
            here = std::min(here, orbit_least(row_of(candidate[which]), free, 0));
        }
        if (here < least) {
            least = here;
            passing.clear();
        }
        if (here == least) passing.push_back(&candidate);
    }

    commit(candidates, passing, placed, least,
           [this](std::size_t cell) { return row_of(cell); });
    return least;
}

permlib::dom_int LeastImageSearch::commit_column(
    std::vector<std::vector<std::size_t>>& candidates, std::size_t placed,
    permlib::dom_int row, bool last) {
    const std::vector<Perm::ptr> free = still_free();
    ++level_;

    // A candidate with no uncommitted cell in the open row keeps the sentinel and
    // loses, which is right rather than merely convenient: its next cell is in a
    // later row, and a later row is a larger flat index than any cell of this one.
    // The open row was chosen because some candidate does have one, so the passing
    // list is never empty.
    permlib::dom_int least = static_cast<permlib::dom_int>(column_count_);
    std::vector<const std::vector<std::size_t>*> passing;
    for (const std::vector<std::size_t>& candidate : candidates) {
        permlib::dom_int here = static_cast<permlib::dom_int>(column_count_);
        for (std::size_t which = placed; which < candidate.size(); ++which) {
            if (row_of(candidate[which]) != row) continue;
            const permlib::dom_int image =
                orbit_least(column_point_of(candidate[which]), free,
                            static_cast<permlib::dom_int>(row_count_));
            here = std::min(here, static_cast<permlib::dom_int>(image - row_count_));
        }
        if (here < least) {
            least = here;
            passing.clear();
        }
        if (here == least) passing.push_back(&candidate);
    }

    // The last cell of the answer needs no candidates after it, and branching for
    // it would extend the base for nothing.
    if (last) return least;
    commit(candidates, passing, placed,
           static_cast<permlib::dom_int>(row_count_ + least),
           [this, row](std::size_t cell) {
               return row_of(cell) == row ? column_point_of(cell) : kOffAxis;
           });
    return least;
}

template <class Axis>
void LeastImageSearch::commit(std::vector<std::vector<std::size_t>>& candidates,
                              const std::vector<const std::vector<std::size_t>*>& passing,
                              std::size_t placed, permlib::dom_int point, const Axis& axis) {
    std::vector<std::vector<std::size_t>> next;

    // A column of one row can be a column of an earlier one, so the point to commit
    // may already be a base point. Then the subgroup still free fixes it, its orbit
    // is itself, and the only candidates that could have attained it are the ones
    // that already have an uncommitted cell there, carried by the identity. The
    // base does not grow, because stabilising a fixed point again is the same
    // group, and a repeated base point is not a base.
    if (in_base_[point]) {
        for (const std::vector<std::size_t>* candidate : passing) next.push_back(*candidate);
        drop_repeats(next);
        candidates = std::move(next);
        return;
    }

    extend_base(point);
    const std::size_t level = base_.size() - 1;
    for (const std::vector<std::size_t>* candidate : passing) {
        ++branch_;
        for (std::size_t which = placed; which < candidate->size(); ++which) {
            const permlib::dom_int here = axis((*candidate)[which]);
            // One branch per distinct axis point rather than per cell: the element
            // that carries a cell onto the committed point depends on that cell's
            // point on this axis and on nothing else.
            if (here == kOffAxis || branched_[here] == branch_) continue;
            branched_[here] = branch_;
            const std::unique_ptr<Perm> carry(group_.U[level].at(here));
            // Null exactly when this cell's point is not in the committed point's
            // orbit, so no element of the free subgroup could have put it there.
            if (!carry) continue;
            carry->invertInplace();
            next.push_back(moved(*carry, *candidate));
        }
    }
    drop_repeats(next);
    candidates = std::move(next);
}

std::vector<Perm::ptr> LeastImageSearch::still_free() const {
    const std::size_t committed = std::min(base_.size(), group_.B.size());
    std::vector<Perm::ptr> free;
    free.reserve(group_.S.size());
    std::copy_if(group_.S.begin(), group_.S.end(), std::back_inserter(free),
                 permlib::PointwiseStabilizerPredicate<Perm>(
                     group_.B.begin(), group_.B.begin() + committed));
    return free;
}

permlib::dom_int LeastImageSearch::orbit_least(permlib::dom_int point,
                                               const std::vector<Perm::ptr>& free,
                                               permlib::dom_int floor) {
    if (cached_at_[point] == level_) return cache_[point];

    permlib::dom_int least = point;
    if (point != floor) {
        ++generation_;
        frontier_.clear();
        frontier_.push_back(point);
        stamp_[point] = generation_;
        for (std::size_t index = 0; index < frontier_.size() && least != floor; ++index) {
            const permlib::dom_int reached = frontier_[index];
            for (const Perm::ptr& element : free) {
                const permlib::dom_int image = *element / reached;
                if (stamp_[image] == generation_) continue;
                stamp_[image] = generation_;
                frontier_.push_back(image);
                if (image < least) least = image;
            }
        }
    }
    cached_at_[point] = level_;
    cache_[point] = least;
    return least;
}

std::vector<std::size_t> LeastImageSearch::moved(
    const Perm& element, const std::vector<std::size_t>& cells) const {
    std::vector<std::size_t> image;
    image.reserve(cells.size());
    for (const std::size_t cell : cells) {
        const std::size_t row = element / row_of(cell);
        const std::size_t column = (element / column_point_of(cell)) - row_count_;
        image.push_back(flat(row, column));
    }
    std::sort(image.begin(), image.end());
    return image;
}

bool LeastImageSearch::settled() const {
    for (std::size_t level = base_.size(); level < group_.B.size(); ++level) {
        if (group_.U[level].size() > 1) return false;
    }
    return true;
}

void LeastImageSearch::extend_base(permlib::dom_int point) {
    base_.push_back(point);
    in_base_[point] = 1;
    change_.change(group_, base_.begin(), base_.end());
}

}  // namespace

FactoredGrid::FactoredGrid(std::size_t left_count, std::size_t right_count,
                           const std::vector<FactoredGenerator>& generators)
    : presentation_(std::make_unique<Presentation>()) {
    presentation_->rows = left_count;
    presentation_->columns = right_count;
    presentation_->generators = generators;
    if (generators.empty()) return;

    const std::size_t degree = left_count + right_count;
    // The same fault caught at run time as well as at compile time, because the
    // static check only sees the type and this sees the number. Past the ceiling
    // PermLib does not fail, it wraps, and a wrapped index is a wrong answer before
    // it is a crash.
    if (degree > std::numeric_limits<permlib::dom_int>::max()) {
        throw std::runtime_error(
            "an axis domain of " + std::to_string(degree) +
            " points does not fit this build's point type, which holds " +
            std::to_string(
                static_cast<std::uint64_t>(std::numeric_limits<permlib::dom_int>::max())) +
            "; the canonical form is refused rather than given a wrapped index");
    }
    // Priced before it is taken, like every other bulk allocation here.
    run_limits::require_room("the group presented on " + std::to_string(degree) + " axis points", degree,
                 kBytesPerAxisPoint);

    std::list<Perm::ptr> permutations;
    for (const FactoredGenerator& generator : generators) {
        permutations.push_back(on_axes(generator, left_count, right_count));
    }
    presentation_->group = permlib::construct(static_cast<permlib::dom_int>(degree),
                                              permutations.begin(), permutations.end());
}

FactoredGrid::~FactoredGrid() = default;
FactoredGrid::FactoredGrid(FactoredGrid&&) noexcept = default;
FactoredGrid& FactoredGrid::operator=(FactoredGrid&&) noexcept = default;

std::size_t FactoredGrid::left_count() const { return presentation_->rows; }
std::size_t FactoredGrid::right_count() const { return presentation_->columns; }
std::size_t FactoredGrid::cell_count() const {
    return presentation_->rows * presentation_->columns;
}

std::vector<std::size_t> FactoredGrid::least_image(const std::vector<std::size_t>& cells) const {
    if (!presentation_->group) {
        std::vector<std::size_t> sorted = cells;
        std::sort(sorted.begin(), sorted.end());
        sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
        return sorted;
    }
    LeastImageSearch search(presentation_->rows, presentation_->columns, *presentation_->group);
    return search.of(cells);
}

}  // namespace bilinear_rank
