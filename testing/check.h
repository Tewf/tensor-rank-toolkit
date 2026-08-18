#pragma once

#include <iostream>
#include <string>

/// A test is a program that prints what it checked and exits non-zero if any of
/// it was wrong. CTest supplies the reporting, so nothing else is needed and no
/// test framework is worth a dependency here.
namespace check {

inline int failure_count = 0;

inline void equal(const std::string& what, long long actual, long long expected) {
    if (actual == expected) {
        std::cout << "  ok    " << what << " = " << actual << "\n";
    } else {
        std::cout << "  FAIL  " << what << " = " << actual << ", expected " << expected << "\n";
        ++failure_count;
    }
}

/// Some interfaces are the words themselves. An argument parser's refusal is
/// read by a person, and a message naming the function that threw instead of the
/// flag that was wrong is the fault, not the throwing. So the sentence is
/// asserted on, and both sentences are printed when they differ.
inline void text(const std::string& what, const std::string& actual, const std::string& expected) {
    if (actual == expected) {
        std::cout << "  ok    " << what << "\n";
    } else {
        std::cout << "  FAIL  " << what << "\n        was      " << actual
                  << "\n        expected " << expected << "\n";
        ++failure_count;
    }
}

inline int report(const std::string& suite) {
    if (failure_count == 0) {
        std::cout << suite << ": all checks passed\n";
        return 0;
    }
    std::cout << suite << ": " << failure_count << " check(s) failed\n";
    return 1;
}

}  // namespace check
