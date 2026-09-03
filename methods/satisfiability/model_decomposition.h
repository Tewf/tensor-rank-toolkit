#pragma once

#include <vector>

#include "binary_encoding.h"
#include "field_theory_encoding.h"
#include "prime_field_encoding.h"
#include "types.h"

/// What a solver's answer means, once you have one.
///
/// Every encoding here states the same question in a different language, so
/// every one of them needs the same thing done to a model that comes back:
/// read the unknowns out as field elements, multiply them into rank-one
/// matrices, and check that recombining those really is the tensor. That is one
/// job, and it lives here rather than three times over beside the encoders.
///
/// **The check is not optional.** A solver is a large program, an encoding is a
/// small one written by hand, and the cheap thing to do with a "yes" is to
/// disbelieve it until the matrices reproduce the tensor. A model that does not
/// reconstruct is a bug in the encoding, never a decomposition.
namespace satisfiability {

/// The rank-one matrices a satisfying model stands for, one per product.
///
/// Each is the outer product `a⁽ˡ⁾ ⊗ b⁽ˡ⁾`, which is what the rest of this
/// repository calls one multiplication. Empty when the model is unsatisfiable.
std::vector<Matrix> decomposition_from_model(const Field& field, const BinaryEncoding& encoding,
                                             const formats::Model& model);
std::vector<Matrix> decomposition_from_model(const Field& field,
                                             const PrimeFieldEncoding& encoding,
                                             const formats::Model& model);
std::vector<Matrix> decomposition_from_model(const Field& field,
                                             const FieldTheoryEncoding& encoding,
                                             const formats::SmtModel& model);

/// Whether those matrices, weighted as the model says, really are the tensor.
bool model_reconstructs(const Field& field, const formats::Tensor& tensor,
                        const BinaryEncoding& encoding, const formats::Model& model);
bool model_reconstructs(const Field& field, const formats::Tensor& tensor,
                        const PrimeFieldEncoding& encoding, const formats::Model& model);
bool model_reconstructs(const Field& field, const formats::Tensor& tensor,
                        const FieldTheoryEncoding& encoding,
                        const formats::SmtModel& model);

}  // namespace satisfiability
