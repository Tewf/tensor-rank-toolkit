# The only test that links both sides of the orbit-cube boundary, because the
# question it asks needs both: do the cubes give the answer the whole formula
# gives? The contract they meet is
# `methods/bilinear_rank/orbit_reduction/orbit_cube_boundary/`.
add_executable(test_orbit_cubes test_orbit_cubes.cpp)
target_link_libraries(test_orbit_cubes PRIVATE orbit_reduction greedy_heuristic
                                               map_construction satisfiability formats
                                               testing project_warnings)
add_test(NAME orbit_cubes COMMAND test_orbit_cubes)

# The validation the boundary actually rests on: the same question whole and
# split into cubes, at 7 products where <2,2,2> is satisfiable and at 6 where it
# is not. `methods/bilinear_rank/orbit_reduction/orbit_cube_boundary/validation.md` recorded 65.4 s,
# five for five, on 2026-08-16; the refutation at 6 is nearly all of it.
add_test(NAME orbit_cubes_preserve_the_answer COMMAND test_orbit_cubes --with-solver)
set_tests_properties(orbit_cubes_preserve_the_answer PROPERTIES LABELS slow TIMEOUT 900)

# The only test that can catch a wrong symmetry group: the quotiented tree and
# the whole tree must answer the same question the same way.
add_executable(test_symmetry_agreement test_symmetry_agreement.cpp)
target_link_libraries(test_symmetry_agreement PRIVATE orbit_reduction greedy_heuristic
                                                      formats testing project_warnings)
add_test(NAME symmetry_agreement COMMAND test_symmetry_agreement ${PROJECT_SOURCE_DIR}/evidence/fixtures)

# The cheap rejection rule against the exact one. Two claims: the verdicts agree,
# which is what a rule that removed a whole orbit would break, and no orbit of
# `⟨2,2,2⟩` is emptied by the generator-only test, checked against a sabotaged
# rule that empties orbits, so a green line here means something.
add_executable(test_generator_rejection test_generator_rejection.cpp)
target_link_libraries(test_generator_rejection PRIVATE orbit_reduction greedy_heuristic
                                                       formats testing project_warnings)
add_test(NAME generator_rejection COMMAND test_generator_rejection
                                          ${PROJECT_SOURCE_DIR}/evidence/fixtures)

# The same question for the heuristic rather than the tree: quotienting its pool
# must not move where it stops. That is the argument orbit_heuristic.h makes, and
# it had no test at all.
add_executable(test_orbit_heuristic test_orbit_heuristic.cpp)
target_link_libraries(test_orbit_heuristic PRIVATE orbit_reduction greedy_heuristic
                                                   formats testing project_warnings)
add_test(NAME orbit_heuristic COMMAND test_orbit_heuristic ${PROJECT_SOURCE_DIR}/evidence/fixtures)
