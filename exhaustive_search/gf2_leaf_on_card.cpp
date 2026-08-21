#include "gf2_leaf_on_card.h"

namespace bilinear_rank {

namespace {

const LeafOnCard* registered = nullptr;

}  // namespace

void register_leaf_on_card(const LeafOnCard* backend) { registered = backend; }
const LeafOnCard* leaf_on_card() { return registered; }

}  // namespace bilinear_rank
