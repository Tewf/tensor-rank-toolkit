# Deciding tensor rank by handing rank(T) <= r to a solver rather than enumerating it: the
# three encoders that state the question, the process that runs an external SAT or SMT
# solver on it, and the check that a returned model really reconstructs the tensor. No
# solver library is linked here: kissat, cryptominisat and cvc5 are found on PATH at run
# time, so the build depends on Givaro alone and still passes its tests without one installed.
add_library(satisfiability STATIC boolean_formula.cpp formula_to_tensor.cpp
                                 binary_encoding.cpp prime_field_encoding.cpp
                                 field_theory_encoding.cpp rank_question.cpp
                                 model_decomposition.cpp
                                 symmetry_breaking.cpp
                                 solver_process.cpp)
target_include_directories(satisfiability PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(satisfiability PUBLIC run_limits linear_algebra formats cli PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
