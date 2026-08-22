#include "search_trace.h"

#include <fstream>
#include <stdexcept>

namespace bilinear_rank {

namespace {

/// The verbs, in the order the enum names them. They are `event_trace.py`'s
/// `search` world and not this file's invention: a renderer that meets one that
/// is not there is a bug rather than something to skip.
const char* const VERB[] = {"open", "bound", "prune", "adopt", "close"};

/// A JSON string body. The only text that reaches here is a fixture name, a
/// symmetry spelling and the literals in `prune`, so this escapes what those can
/// hold and refuses anything else rather than emitting a file that will not parse.
std::string quoted(const std::string& text) {
    std::string out = "\"";
    for (const char character : text) {
        if (character == '"' || character == '\\') out += '\\';
        if (static_cast<unsigned char>(character) < 0x20) {
            throw std::invalid_argument("a control character reached the trace: " + text);
        }
        out += character;
    }
    return out + "\"";
}

}  // namespace

SearchTrace::SearchTrace(std::string tensor, std::size_t target, std::string quotient,
                         std::size_t pool_size)
    : tensor_(std::move(tensor)), target_(target), quotient_(std::move(quotient)),
      pool_size_(pool_size) {}

std::size_t SearchTrace::open(std::size_t parent, std::size_t depth, std::size_t candidate,
                              bool is_root) {
    const std::size_t me = nodes_++;
    events_.push_back({Op::Open, static_cast<std::uint32_t>(me),
                       static_cast<std::uint32_t>(parent),
                       static_cast<std::uint32_t>(candidate),
                       static_cast<std::uint16_t>(depth), is_root, nullptr});
    return me;
}

void SearchTrace::dimension(std::size_t node, std::size_t value) {
    events_.push_back({Op::Bound, static_cast<std::uint32_t>(node),
                       static_cast<std::uint32_t>(value), 0, 0, false, nullptr});
}

void SearchTrace::prune(std::size_t node, const char* why) {
    events_.push_back({Op::Prune, static_cast<std::uint32_t>(node), 0, 0, 0, false, why});
}

void SearchTrace::adopt(std::size_t node, std::size_t products) {
    events_.push_back({Op::Adopt, static_cast<std::uint32_t>(node),
                       static_cast<std::uint32_t>(products), 0, 0, false, nullptr});
}

void SearchTrace::close(std::size_t node) {
    events_.push_back({Op::Close, static_cast<std::uint32_t>(node), 0, 0, 0, false, nullptr});
}

void SearchTrace::write(const std::string& path) const {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write a trace to " + path);

    out << "{\"schema\": \"trace/1\", \"algorithm\": \"decide_rank\", \"world\": \"search\", "
        << "\"seed\": null, \"config\": {\"tensor\": " << quoted(tensor_)
        << ", \"target\": " << target_
        << ", \"quotient\": " << (quotient_.empty() ? "null" : quoted(quotient_))
        << ", \"pool\": " << pool_size_ << "}}\n";

    std::size_t step = 0;
    for (const Event& event : events_) {
        out << "{\"t\": " << step++ << ", \"op\": \"" << VERB[static_cast<std::size_t>(event.op)]
            << "\", \"ids\": [\"n" << event.node << "\"]";
        switch (event.op) {
            case Op::Open:
                out << ", \"attrs\": {\"parent\": ";
                if (event.is_root) out << "null";
                else out << "\"n" << event.number << "\"";
                out << ", \"depth\": " << event.depth;
                if (!event.is_root) out << ", \"branch\": \"+ pool " << event.candidate << "\"";
                out << "}";
                break;
            case Op::Bound:
                out << ", \"attrs\": {\"value\": " << event.number << "}";
                break;
            case Op::Prune:
                out << ", \"attrs\": {\"why\": \"" << event.why << "\"}";
                break;
            case Op::Adopt:
                out << ", \"attrs\": {\"value\": " << event.number << "}";
                break;
            case Op::Close:
                break;
        }
        out << "}\n";
    }
    if (!out) throw std::runtime_error("the trace to " + path + " did not write cleanly");
}

}  // namespace bilinear_rank
