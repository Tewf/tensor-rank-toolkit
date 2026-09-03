#include "card_failure.h"

namespace bilinear_rank {

namespace {

std::string& first_failure() {
    static std::string what;
    return what;
}

}  // namespace

void note_card_failure(const std::string& what) {
    if (first_failure().empty()) first_failure() = what;
}

const std::string& card_failure() { return first_failure(); }

}  // namespace bilinear_rank
