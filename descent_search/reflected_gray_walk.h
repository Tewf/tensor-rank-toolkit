#pragma once

#include <cstddef>
#include <vector>

/// Every base-`p` digit string of a fixed length, one digit and one step apart.
///
/// The reflected Gray code of radix `p` orders the `p^n` strings so that
/// consecutive ones differ in exactly **one** digit, and there by exactly `+1`
/// or `-1`. Counting in base `p` does not: `0,2,2 -> 1,0,0` moves three digits
/// at once, and a walker that maintains anything derived from the string has to
/// rebuild it.
///
/// That is the whole reason this exists. Its two callers keep a linear
/// combination of basis rows alongside the string —
/// [`../exhaustive_search/subspace_walk.h`](../exhaustive_search/subspace_walk.h)
/// over the elements of a leaf's subspace, and
/// [`span_element_ranks`](minimum_weight_basis.h) over the elements of a span —
/// and under this order the update between two consecutive strings is one row
/// added or one row subtracted, so the combination costs one pass over the row
/// rather than one pass per nonzero digit, and no field multiplication at all.
///
/// **It sits here rather than beside the leaf that first wanted it** because the
/// second caller is in this module and this module is the one both can reach:
/// `exhaustive_search` links `descent_search` and not the other way round. It is
/// the same enumeration [`span_enumeration.h`](span_enumeration.h) walks in index
/// order, in the order that makes consecutive elements one step apart, which is
/// why the two live side by side.
///
/// **The successor is loop-free**, which is the other half of the point: it is
/// Knuth's Algorithm H, `[knuth-4a]` 7.2.1.1, the focus-pointer method. A
/// successor that scanned for the digit to move would be `O(n)` a step and would
/// hand back exactly the factor of `n` the order was adopted to remove. `focus_`
/// is Knuth's `f`, a union-find-flavoured chain of pointers that names the next
/// digit to move in `O(1)`, and `direction_` is his `o`, which way each digit is
/// currently travelling.
///
/// **The radix must be at least two.** It is a field's characteristic
/// everywhere here, so it is prime and the condition is free; at radix one a
/// digit could never reach `radix - 1` and the walk would not terminate.
namespace bilinear_rank {

class ReflectedGrayWalk {
   public:
    /// Which digit the last step moved, and which way it went.
    struct Step {
        std::size_t digit = 0;
        bool upward = true;
    };

    /// The walk starts on the all-zero string, before any step.
    ReflectedGrayWalk(std::size_t digits, std::size_t radix)
        : radix_(radix), string_(digits, 0), focus_(digits + 1), direction_(digits, 1) {
        for (std::size_t position = 0; position <= digits; ++position) focus_[position] = position;
    }

    /// Move to the next string, or say that all `radix^digits` have been visited.
    ///
    /// Constant work: one focus pointer read, one digit changed, and at most one
    /// reflection recorded.
    bool advance(Step& step) {
        // A walk over no digits is the all-zero string on its own and takes no
        // step. Said here rather than left to the test below because it is also
        // what puts every index in this function provably inside its array: with
        // the string empty there is nothing after this line to reach.
        if (string_.empty()) return false;

        const std::size_t position = focus_[0];
        focus_[0] = 0;
        if (position == string_.size()) return false;

        const bool upward = direction_[position] > 0;
        if (upward) {
            ++string_[position];
        } else {
            --string_[position];
        }
        step.digit = position;
        step.upward = upward;

        // A digit that has reached either end turns round, and hands the focus
        // on to the digit above it so the next step moves that one instead.
        if (string_[position] == 0 || string_[position] + 1 == radix_) {
            direction_[position] = -direction_[position];
            focus_[position] = focus_[position + 1];
            focus_[position + 1] = position + 1;
        }
        return true;
    }

    /// The string as it stands, least significant digit first.
    const std::vector<std::size_t>& string() const { return string_; }

   private:
    std::size_t radix_;
    std::vector<std::size_t> string_;
    std::vector<std::size_t> focus_;
    std::vector<int> direction_;
};

}  // namespace bilinear_rank
