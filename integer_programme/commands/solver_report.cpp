/// Which integer programming solvers this machine has, best first.
#include <iostream>

#include "solver_chain.h"

int main() {
    std::cout << "integer programming backends, in preference order:\n";
    for (const optimisation::Backend backend : optimisation::ranked_backends()) {
        std::cout << "  " << (optimisation::is_available(backend) ? "present" : "absent ") << "  "
                  << optimisation::name_of(backend) << "\n";
    }
    std::cout << "\nThe first present backend answers; the built-in is always present and is the\n"
                 "only one whose answer needs no checking, so it is also the only one that may\n"
                 "report a problem infeasible.\n";
    return 0;
}
