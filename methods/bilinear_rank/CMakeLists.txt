# The vocabulary both searches share and neither owns: the rank-one pool
# and its addressed odometer, the reflected Gray order both leaves step in,
# the decomposition <-> (L,R,P) recovery, and the aliases that name the
# domain. This directory is the namespace's home: everything under it
# declares `bilinear_rank`, and what sits at this root is what rule seven
# of the conventions sends to the common ancestor.
add_library(bilinear_rank STATIC candidate_pool.cpp algorithm_recovery.cpp)
target_include_directories(bilinear_rank PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bilinear_rank PUBLIC linear_algebra formats
                                    PRIVATE project_warnings)

add_subdirectory(commands)
