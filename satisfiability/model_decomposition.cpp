#include "model_decomposition.h"

namespace satisfiability {

namespace {

bool holds(const linear_algebra::Model& model, int variable) {
    const std::size_t index = static_cast<std::size_t>(variable);
    return index < model.values.size() && model.values[index];
}

/// The field value a one-hot group is carrying, or zero if none of it is set.
std::size_t value_in(const linear_algebra::Model& model, const std::vector<int>& group) {
    for (std::size_t value = 0; value < group.size(); ++value) {
        if (holds(model, group[value])) return value;
    }
    return 0;
}

std::size_t value_of(const linear_algebra::SmtModel& model, const std::string& name) {
    const auto found = model.values.find(name);
    return found == model.values.end() ? 0 : found->second;
}

/// Sum `terms`, each weighted by what the model gives its output coordinate,
/// and compare against the slice. The three encodings differ only in how a
/// weight is read, so that is the one thing passed in.
template <class Weight>
bool rebuilds(const Field& field, const linear_algebra::Tensor& tensor,
              const std::vector<Matrix>& terms, std::size_t slices, Weight weight_of) {
    for (std::size_t slice = 0; slice < slices; ++slice) {
        Matrix rebuilt(tensor.rows(), tensor.columns());
        for (std::size_t term = 0; term < terms.size(); ++term) {
            const std::size_t weight = weight_of(term, slice);
            if (weight == 0) continue;

            Element scalar;
            field.init(scalar, static_cast<int64_t>(weight));
            for (std::size_t entry = 0; entry < rebuilt.entry_count(); ++entry) {
                field.axpyin(rebuilt.data()[entry], scalar, terms[term].data()[entry]);
            }
        }
        for (std::size_t entry = 0; entry < rebuilt.entry_count(); ++entry) {
            if (!field.areEqual(rebuilt.data()[entry], tensor.slices[slice].data()[entry])) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

std::vector<Matrix> decomposition_from_model(const Field& field, const BinaryEncoding& encoding,
                                             const linear_algebra::Model& model) {
    if (!model.satisfiable) return {};

    std::vector<Matrix> terms;
    terms.reserve(encoding.products);
    for (std::size_t term = 0; term < encoding.products; ++term) {
        Matrix outer(encoding.rows, encoding.columns);
        for (std::size_t row = 0; row < encoding.rows; ++row) {
            if (!holds(model, encoding.left[term * encoding.rows + row])) continue;
            for (std::size_t column = 0; column < encoding.columns; ++column) {
                if (!holds(model, encoding.right[term * encoding.columns + column])) continue;
                field.assign(outer(row, column), field.one);
            }
        }
        terms.push_back(std::move(outer));
    }
    return terms;
}

std::vector<Matrix> decomposition_from_model(const Field& field,
                                             const PrimeFieldEncoding& encoding,
                                             const linear_algebra::Model& model) {
    if (!model.satisfiable) return {};

    std::vector<Matrix> terms;
    terms.reserve(encoding.products);
    for (std::size_t term = 0; term < encoding.products; ++term) {
        Matrix outer(encoding.rows, encoding.columns);
        for (std::size_t row = 0; row < encoding.rows; ++row) {
            const std::size_t left = value_in(model, encoding.left_group(term, row));
            if (left == 0) continue;
            for (std::size_t column = 0; column < encoding.columns; ++column) {
                const std::size_t right = value_in(model, encoding.right_group(term, column));
                field.init(outer(row, column), static_cast<int64_t>(left * right));
            }
        }
        terms.push_back(std::move(outer));
    }
    return terms;
}

std::vector<Matrix> decomposition_from_model(const Field& field,
                                             const FieldTheoryEncoding& encoding,
                                             const linear_algebra::SmtModel& model) {
    if (!model.satisfiable) return {};

    std::vector<Matrix> terms;
    terms.reserve(encoding.products);
    for (std::size_t term = 0; term < encoding.products; ++term) {
        Matrix outer(encoding.rows, encoding.columns);
        for (std::size_t row = 0; row < encoding.rows; ++row) {
            const std::size_t left =
                value_of(model, FieldTheoryEncoding::left_name(term, row));
            if (left == 0) continue;
            for (std::size_t column = 0; column < encoding.columns; ++column) {
                const std::size_t right =
                    value_of(model, FieldTheoryEncoding::right_name(term, column));
                field.init(outer(row, column), static_cast<int64_t>(left * right));
            }
        }
        terms.push_back(std::move(outer));
    }
    return terms;
}

bool model_reconstructs(const Field& field, const linear_algebra::Tensor& tensor,
                        const BinaryEncoding& encoding, const linear_algebra::Model& model) {
    if (!model.satisfiable) return false;
    return rebuilds(field, tensor, decomposition_from_model(field, encoding, model),
                    encoding.slices, [&](std::size_t term, std::size_t slice) -> std::size_t {
                        return holds(model, encoding.output[term * encoding.slices + slice]) ? 1
                                                                                             : 0;
                    });
}

bool model_reconstructs(const Field& field, const linear_algebra::Tensor& tensor,
                        const PrimeFieldEncoding& encoding, const linear_algebra::Model& model) {
    if (!model.satisfiable) return false;
    return rebuilds(field, tensor, decomposition_from_model(field, encoding, model),
                    encoding.slices, [&](std::size_t term, std::size_t slice) {
                        return value_in(model, encoding.output_group(term, slice));
                    });
}

bool model_reconstructs(const Field& field, const linear_algebra::Tensor& tensor,
                        const FieldTheoryEncoding& encoding,
                        const linear_algebra::SmtModel& model) {
    if (!model.satisfiable) return false;
    return rebuilds(field, tensor, decomposition_from_model(field, encoding, model),
                    encoding.slices, [&](std::size_t term, std::size_t slice) {
                        return value_of(model, FieldTheoryEncoding::output_name(term, slice));
                    });
}

}  // namespace satisfiability
