# How much of the machine one run may take: memory, and cores. Shared by every
# method, so it belongs to none of them.
#
# machine.h is the one place that asks the operating system what the machine has,
# so a default can be derived from it instead of fitted to this laptop.
# memory_budget.h is the one place that decides how much a run may ask for, since
# both bulk allocations grow exponentially in the shape. parallel.h spreads
# independent work across cores, one worker by default so a run reproduces what
# was published. device.h ranks the processors a bulk question may be asked on,
# and is a seam with no GPU backend behind it yet.
add_library(run_limits STATIC child_process.cpp device.cpp machine.cpp memory_budget.cpp parallel.cpp)
target_include_directories(run_limits PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(run_limits PRIVATE project_warnings)

# An instrument, not a tool: it asks nothing about a tensor and answers no
# question on the command list. It prints what this machine and this working
# directory bound a run to, which until now could only be inferred from a plan
# printed after a search had already started.
add_executable(show-limits show_limits.cpp)
target_link_libraries(show-limits PRIVATE run_limits cli project_warnings)

add_subdirectory(tests)
