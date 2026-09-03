# A descent heuristic for finding bilinear map rank, corrected and extended.
# Three steps: a matroid greedy that is exact for the basis it picks, then two
# that relax the constraint it works under and guarantee nothing.
add_library(greedy_heuristic STATIC span_enumeration.cpp minimum_weight_basis.cpp
                                       gf2_span_walk.cpp sorted_span.cpp
                                       minimise_rank.cpp algorithm_recovery.cpp
                                       candidate_pool.cpp card_failure.cpp
                                       span_ranks_on_card.cpp)
target_include_directories(greedy_heuristic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(greedy_heuristic PUBLIC linear_algebra run_limits
                                           PRIVATE project_warnings)

add_subdirectory(commands)
add_subdirectory(tests)
