#include "gf2_leaf_on_card.h"

namespace bilinear_rank {

namespace {

const LeafOnCard* registered = nullptr;

/// The first failure and not the last, because the first is the one with a cause
/// a reader can act on: everything after a card falls over tends to be the same
/// sticky error repeated once per leaf.
std::string& first_failure() {
    static std::string what;
    return what;
}

}  // namespace

void register_leaf_on_card(const LeafOnCard* backend) { registered = backend; }
const LeafOnCard* leaf_on_card() { return registered; }

void note_card_failure(const std::string& what) {
    if (first_failure().empty()) first_failure() = what;
}

const std::string& card_failure() { return first_failure(); }

}  // namespace bilinear_rank
