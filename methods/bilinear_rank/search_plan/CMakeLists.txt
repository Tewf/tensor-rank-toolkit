# One place where a run decides how it will be carried out, and says why.
#
# search_plan.h holds the seven choices and the three rules that settle them;
# plan_file.h writes one down and reads it back, so a run elsewhere makes the
# same choices on different hardware. It owns no rule of its own: each is asked
# of the module that already had it, which is why this links four of them and
# restates none.
#
# It sits above `orbit_reduction`, which sits above `exhaustive`, because
# a plan names the rejection rule and the leaf route both. That is the reason it
# is a directory of its own rather than a file inside either: neither can see
# the other's enum, and a copy of one in the other is how the plan and the run
# would come to disagree.
add_library(search_plan STATIC search_plan.cpp plan_file.cpp)
target_include_directories(search_plan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(search_plan PUBLIC exhaustive orbit_reduction run_limits cli
                                  PRIVATE project_warnings)

add_subdirectory(tests)
