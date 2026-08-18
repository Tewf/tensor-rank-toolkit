#include "flip_graph.h"

#include <random>

namespace bilinear_rank {

namespace {

/// The three modes a term has, so a flip can be written once instead of three
/// times. `[kauers2023]` states the move for one arrangement and says it applies
/// to any permutation of the three; this is that permutation, made data.
enum Mode { kLeft = 0, kRight = 1, kOutput = 2 };

std::vector<Element>& part(Term& term, Mode mode) {
    return mode == kLeft ? term.left : (mode == kRight ? term.right : term.output);
}

/// Do these two factors point the same way, and by what scalar?
///
/// Over GF(2) this is plain equality, the only nonzero scalar being one. Over
/// any larger field it is equality **up to a scalar**, and that scalar is the
/// whole difficulty `[chen2025]` names when it asks for this walk over `Z3`,
/// `Z5` and `Z7`: since `(αu)⊗v⊗w = u⊗(αv)⊗w`, one term has `p−1` spellings, and
/// a walk that compares spellings rather than directions sees a fraction of the
/// flips that are really available and wanders a space it should be quotienting.
///
/// Comparing directions is that quotient, taken here rather than by picking a
/// normal form: a normal form has to choose which factor keeps the scalar, and
/// that choice is wrong for whichever mode the next flip happens to share.
bool aligned(const Field& field, const std::vector<Element>& first,
             const std::vector<Element>& second, Element& factor) {
    return scalar_multiple(field, first, second, factor) && !field.isZero(factor);
}

void scale(const Field& field, std::vector<Element>& into, const Element& factor) {
    for (Element& entry : into) field.mulin(entry, factor);
}

/// Divide `mode` by `factor` and pay for it in `paying`. The term is unchanged,
/// since `(α⁻¹u)⊗v⊗(αw)` is `u⊗v⊗w`; only its spelling moves.
void rescale(const Field& field, Term& term, Mode mode, Mode paying, const Element& factor) {
    Element inverse;
    field.inv(inverse, factor);
    scale(field, part(term, mode), inverse);
    scale(field, part(term, paying), factor);
}

bool is_zero(const Field& field, const std::vector<Element>& vector) {
    for (const Element& entry : vector) {
        if (!field.isZero(entry)) return false;
    }
    return true;
}

/// `into += from`, in the field.
void add_into(const Field& field, std::vector<Element>& into, const std::vector<Element>& from) {
    for (std::size_t index = 0; index < into.size(); ++index) {
        field.addin(into[index], from[index]);
    }
}

/// `into -= from`.
void subtract_from(const Field& field, std::vector<Element>& into,
                   const std::vector<Element>& from) {
    for (std::size_t index = 0; index < into.size(); ++index) {
        field.subin(into[index], from[index]);
    }
}

/// One available flip: the pair, the mode they share, and the scalar between
/// their two copies of it.
struct Move {
    std::size_t first;
    std::size_t second;
    Mode shared;
    Element factor;  // part(second, shared) == factor · part(first, shared)
};

/// The two modes that are not this one, in order.
std::pair<Mode, Mode> others(Mode shared) {
    if (shared == kLeft) return {kRight, kOutput};
    if (shared == kRight) return {kLeft, kOutput};
    return {kLeft, kRight};
}

}  // namespace

Scheme scheme_of(const Algorithm& algorithm) {
    Scheme scheme;
    scheme.reserve(algorithm.product_count());
    for (std::size_t product = 0; product < algorithm.product_count(); ++product) {
        Term term;
        term.left = algorithm.left.row(product);
        term.right = algorithm.right.row(product);
        term.output.resize(algorithm.decode.rows());
        for (std::size_t output = 0; output < algorithm.decode.rows(); ++output) {
            term.output[output] = algorithm.decode(output, product);
        }
        scheme.push_back(std::move(term));
    }
    return scheme;
}

Algorithm algorithm_of(const Scheme& scheme) {
    Algorithm algorithm;
    if (scheme.empty()) return algorithm;

    algorithm.left = Matrix(scheme.size(), scheme.front().left.size());
    algorithm.right = Matrix(scheme.size(), scheme.front().right.size());
    algorithm.decode = Matrix(scheme.front().output.size(), scheme.size());
    for (std::size_t product = 0; product < scheme.size(); ++product) {
        for (std::size_t index = 0; index < scheme[product].left.size(); ++index) {
            algorithm.left(product, index) = scheme[product].left[index];
        }
        for (std::size_t index = 0; index < scheme[product].right.size(); ++index) {
            algorithm.right(product, index) = scheme[product].right[index];
        }
        for (std::size_t index = 0; index < scheme[product].output.size(); ++index) {
            algorithm.decode(index, product) = scheme[product].output[index];
        }
    }
    return algorithm;
}

std::size_t merge_shared_terms(const Field& field, Scheme& scheme) {
    std::size_t removed = 0;
    for (bool again = true; again;) {
        again = false;

        // A term whose any factor is zero computes nothing.
        for (std::size_t index = 0; index < scheme.size(); ++index) {
            if (!is_zero(field, scheme[index].left) && !is_zero(field, scheme[index].right) &&
                !is_zero(field, scheme[index].output)) {
                continue;
            }
            scheme.erase(scheme.begin() + static_cast<std::ptrdiff_t>(index));
            ++removed;
            again = true;
            break;
        }
        if (again) continue;

        // Two terms agreeing on two modes: the third mode adds up and one term
        // goes. This is `[kauers2023, Prop. 3]` in the case that costs nothing
        // to detect, widened to agreement up to a scalar in each of the two
        // modes. Over GF(2) both scalars are one and this is the original rule.
        for (std::size_t first = 0; first < scheme.size() && !again; ++first) {
            for (std::size_t second = first + 1; second < scheme.size() && !again; ++second) {
                for (const Mode shared : {kLeft, kRight, kOutput}) {
                    const auto [other, third] = others(shared);
                    Element along;
                    Element across;
                    if (!aligned(field, part(scheme[first], shared), part(scheme[second], shared),
                                 along) ||
                        !aligned(field, part(scheme[first], other), part(scheme[second], other),
                                 across)) {
                        continue;
                    }
                    // The second term is `(along·A)⊗(across·B)⊗Γ`, which is
                    // `A⊗B⊗(along·across·Γ)`. Once its third factor carries both
                    // scalars, the two terms differ in that factor alone.
                    Element combined;
                    field.mul(combined, along, across);
                    scale(field, part(scheme[second], third), combined);
                    add_into(field, part(scheme[first], third), part(scheme[second], third));
                    scheme.erase(scheme.begin() + static_cast<std::ptrdiff_t>(second));
                    ++removed;
                    again = true;
                    break;
                }
            }
        }
    }
    return removed;
}

Scheme random_flip_walk(const Field& field, Scheme scheme, std::size_t steps, std::uint64_t seed,
            FlipReport* report) {
    std::mt19937_64 generator(seed);
    merge_shared_terms(field, scheme);

    Scheme best = scheme;
    std::size_t flips = 0;
    std::size_t reductions = 0;

    for (std::size_t step = 0; step < steps && scheme.size() > 1; ++step) {
        // Every pair whose factors point the same way, in any mode: those are
        // the flips available from here.
        std::vector<Move> moves;
        for (std::size_t first = 0; first < scheme.size(); ++first) {
            for (std::size_t second = 0; second < scheme.size(); ++second) {
                if (first == second) continue;
                for (const Mode shared : {kLeft, kRight, kOutput}) {
                    Element factor;
                    if (aligned(field, part(scheme[first], shared), part(scheme[second], shared),
                                factor)) {
                        moves.push_back({first, second, shared, factor});
                    }
                }
            }
        }
        if (moves.empty()) break;

        const Move move =
            moves[std::uniform_int_distribution<std::size_t>(0, moves.size() - 1)(generator)];
        const auto [other, third] = others(move.shared);

        // The identity below needs the shared factor to be shared exactly, not
        // merely in direction. Moving the scalar out of it and into the third
        // factor costs nothing and leaves the term itself alone.
        rescale(field, scheme[move.second], move.shared, third, move.factor);

        // A⊗B⊗Γ + A⊗B'⊗Γ' = A⊗(B+B')⊗Γ + A⊗B'⊗(Γ'−Γ)
        const std::vector<Element> was_third = part(scheme[move.first], third);
        add_into(field, part(scheme[move.first], other), part(scheme[move.second], other));
        subtract_from(field, part(scheme[move.second], third), was_third);
        ++flips;

        const std::size_t gone = merge_shared_terms(field, scheme);
        reductions += gone;
        if (scheme.size() < best.size()) best = scheme;
    }

    if (report != nullptr) {
        report->flips = flips;
        report->reductions = reductions;
        report->best = best.size();
    }
    return best;
}

}  // namespace bilinear_rank
