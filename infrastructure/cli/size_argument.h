#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

/// Reading a memory size off the command line, the way a person writes one.
namespace cli {

/// `"512M"`, `"4G"`, `"2048K"` or a plain count of bytes.
///
/// Throws on anything else rather than guessing: a budget silently read as two
/// bytes, or as two gigabytes when two megabytes were meant, is worse than a
/// refusal, since the whole point of the number is to stop a machine dying.
inline std::size_t parse_size(const std::string& text) {
    if (text.empty()) throw std::runtime_error("empty size");

    std::size_t multiplier = 1;
    std::string digits = text;
    const char suffix = static_cast<char>(std::toupper(text.back()));
    if (suffix == 'K' || suffix == 'M' || suffix == 'G') {
        multiplier = suffix == 'K' ? (std::size_t(1) << 10)
                                   : (suffix == 'M' ? (std::size_t(1) << 20) : (std::size_t(1) << 30));
        digits.pop_back();
    }
    if (digits.empty()) throw std::runtime_error("'" + text + "' has no number in it");
    for (const char digit : digits) {
        if (digit < '0' || digit > '9') throw std::runtime_error("'" + text + "' is not a size");
    }

    const std::size_t value = std::stoull(digits);
    if (value > std::size_t(-1) / multiplier) throw std::runtime_error("'" + text + "' is too large");
    return value * multiplier;
}

}  // namespace cli
