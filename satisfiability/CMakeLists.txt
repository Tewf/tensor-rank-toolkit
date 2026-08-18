add_library(satisfiability STATIC boolean_formula.cpp formula_to_tensor.cpp
                                 binary_encoding.cpp prime_field_encoding.cpp
                                 field_theory_encoding.cpp rank_question.cpp
                                 model_decomposition.cpp
                                 symmetry_breaking.cpp
                                 solver_process.cpp)
target_include_directories(satisfiability PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(satisfiability PUBLIC linear_algebra formats cli PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
