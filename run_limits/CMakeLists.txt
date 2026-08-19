# How much of the machine one run may take: memory, and cores. Shared by every
# method, so it belongs to none of them.
#
# memory_budget.h is the one place that decides how much a run may ask for, since
# both bulk allocations grow exponentially in the shape. parallel.h spreads
# independent work across cores, one worker by default so a run reproduces what
# was published.
add_library(run_limits STATIC child_process.cpp memory_budget.cpp parallel.cpp)
target_include_directories(run_limits PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(run_limits PRIVATE project_warnings)
