/// The setwise stabiliser of a set of grid cells, from the axis presentation.
///
/// The other question [`factored_lex_min.h`](factored_lex_min.h) answers about a
/// grid's group is its least image, and that one is a search written out by hand.
/// This one is `[permlib]`'s own classic backtrack with a different predicate, so
/// it shares the presentation and nothing else: the two roles are two files, and
/// [`axis_presentation.h`](axis_presentation.h) is the presentation they share.
///
/// One presentation, not two, because the BSGS is the expensive part: building it
/// twice would cost twice, and at `⟨4,4,4⟩` twice is 96 MB and a second
/// Schreier-Sims.
#include "axis_presentation.h"

#include <permlib/search/classic/backtrack_search.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace bilinear_rank {

namespace {

using axes::BaseChange;
using axes::Group;
using axes::Perm;
using axes::Transversal;

/// What a backtrack search has to ask of a group element to name the setwise
/// stabiliser of a set of **cells**, from a group presented on the axes.
///
/// `operator()` is the whole condition and it is asked about cells, so what comes
/// back is `Stab_G(S)` exactly. `childRestriction` is the pruning, and there the
/// coarse condition is not only allowed but wanted: an element carrying the cell
/// set to itself certainly carries touched rows to touched rows and touched columns
/// to touched columns, so refusing anything else refuses only branches that could
/// not have succeeded. Being too permissive in a child restriction costs time;
/// being too permissive in `operator()` would return a group larger than the
/// stabiliser, which is the error nothing downstream catches.
class GridStabiliserPredicate : public permlib::SubgroupPredicate<Perm> {
   public:
    GridStabiliserPredicate(std::size_t row_count, std::size_t column_count,
                            std::vector<std::size_t> cells, std::vector<char> touched,
                            unsigned int touched_count)
        : row_count_(row_count),
          column_count_(column_count),
          cells_(std::move(cells)),
          touched_(std::move(touched)),
          touched_count_(touched_count) {}

    bool operator()(const Perm& element) const override {
        for (const std::size_t cell : cells_) {
            const std::size_t row =
                element / static_cast<permlib::dom_int>(cell / column_count_);
            const std::size_t column =
                (element /
                 static_cast<permlib::dom_int>(row_count_ + cell % column_count_)) -
                row_count_;
            if (!std::binary_search(cells_.begin(), cells_.end(),
                                    row * column_count_ + column)) {
                return false;
            }
        }
        return true;
    }

    bool childRestriction(const Perm& element, unsigned int,
                          unsigned long point) const override {
        return touched_[element / static_cast<permlib::dom_int>(point)] != 0;
    }

    /// Once the images of the touched rows and columns are fixed, the image of
    /// every cell of the set is fixed, so nothing deeper can change the verdict.
    unsigned int limit() const override { return touched_count_; }

   private:
    std::size_t row_count_;
    std::size_t column_count_;
    std::vector<std::size_t> cells_;
    std::vector<char> touched_;
    unsigned int touched_count_;
};

/// `[permlib]`'s classic backtrack, with the predicate above rather than its own.
///
/// `BacktrackSearch::construct` is protected and the limit fields with it, which is
/// why this exists at all: it is the three lines `SetStabilizerSearch::construct`
/// runs before handing over, with the cell predicate in place of the point one.
/// `breakAfterChildRestriction` is safe for the same reason it is safe there: the
/// base is prefixed by the touched points, so `[permlib]`'s base order sorts them
/// first and a level's images leave that prefix once and for good.
class GridStabiliserSearch : public permlib::classic::BacktrackSearch<Group, Transversal> {
   public:
    explicit GridStabiliserSearch(const Group& group)
        : permlib::classic::BacktrackSearch<Group, Transversal>(group, 0, true) {}

    /// Takes ownership of `predicate`, as `[permlib]`'s own searches do.
    void look_for(GridStabiliserPredicate* predicate) {
        this->m_limitLevel = predicate->limit();
        this->m_limitBase = this->m_limitLevel;
        this->m_limitInitialized = true;
        permlib::classic::BacktrackSearch<Group, Transversal>::construct(predicate, false);
    }
};

}  // namespace

std::vector<FactoredGenerator> FactoredGrid::setwise_stabiliser(
    const std::vector<std::size_t>& cells) const {
    if (!presentation_->group) return {};
    // Nothing to stabilise is everything stabilising it, which is what
    // `[permlib]`'s own `setStabilizer` answers for an empty set. The generators
    // given are handed back rather than a strong generating set for the same group:
    // both generate `G`, and a caller that wants orbits does not care which.
    if (cells.empty()) return presentation_->generators;

    std::vector<std::size_t> sorted = cells;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    const std::size_t rows = presentation_->rows;
    const std::size_t columns = presentation_->columns;
    std::vector<char> touched(rows + columns, 0);
    std::vector<unsigned long> prefix;
    for (const std::size_t cell : sorted) {
        for (const std::size_t point : {cell / columns, rows + cell % columns}) {
            if (touched[point]) continue;
            touched[point] = 1;
            prefix.push_back(point);
        }
    }

    // Prefixing the base with the touched points is what makes the search's limit
    // legitimate and its child restriction safe to break on, exactly as it is for
    // `[permlib]`'s point-set stabiliser.
    Group copy(*presentation_->group);
    BaseChange change(copy);
    change.change(copy, prefix.begin(), prefix.end());

    GridStabiliserSearch search(copy);
    search.look_for(new GridStabiliserPredicate(rows, columns, sorted, std::move(touched),
                                                static_cast<unsigned int>(prefix.size())));

    Group fixing(copy.n);
    search.search(fixing);

    std::vector<FactoredGenerator> generators;
    for (const Perm::ptr& element : fixing.S) {
        FactoredGenerator factored;
        factored.left.resize(rows);
        for (std::size_t row = 0; row < rows; ++row) {
            factored.left[row] = *element / static_cast<permlib::dom_int>(row);
        }
        factored.right.resize(columns);
        for (std::size_t column = 0; column < columns; ++column) {
            factored.right[column] = static_cast<std::uint32_t>(
                (*element / static_cast<permlib::dom_int>(rows + column)) - rows);
        }
        generators.push_back(std::move(factored));
    }
    return generators;
}

}  // namespace bilinear_rank
