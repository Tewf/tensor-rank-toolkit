#include "device.h"
#include "leaf_backend.h"
#include "span_ranks_backend.h"

/// Linking this object is the registration, and there is nothing else to it.
///
/// It is one file of its own for two reasons. **`measure-leaf` must not link
/// it**: the harness times the shipped leaf against the card, and a shipped leaf
/// that had quietly started answering on the card would report the card against
/// itself. And **nothing in `decide-rank` should have to know that CUDA exists**:
/// the alternative was a header guarded by a macro and an `#ifdef` around a call
/// in `main`, which puts the card into a file that has no business naming it.
///
/// An object library rather than a static one, because a static library
/// contributes only the objects something already references, and by design
/// nothing references this.
namespace {

struct Registration {
    Registration() {
        bilinear_rank::register_leaf_on_card(&gpu_leaf::card_backend());
        // One seam per question, and they are different questions: a leaf is
        // asked which pool elements lie in a subspace, and this one is asked the
        // rank of every element of a span. A command links this object and gets
        // whichever of the two it actually calls.
        bilinear_rank::register_span_ranks_on_card(&gpu_leaf::span_ranks_backend());
        // A third registration, saying not that a *kernel* exists but that a
        // *card* is present. `run_limits::available` asks the probe afresh every
        // time, so a card that disappears between leaves is noticed rather than
        // cached.
        run_limits::register_gpu_backend(&gpu_leaf::card_present);
    }
};

const Registration registered;

}  // namespace
