#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/// What the search walked, said in the vocabulary of the search and not of a
/// picture.
///
/// A trace is JSON Lines: a header naming the run, then one event per thing that
/// happened, stamped with the step that produced it. It holds no coordinate, no
/// colour and no frame number, because where a node is drawn is a reading of the
/// run and not a fact about it. The format is
/// [after-hours' `event_trace.py`](https://github.com/Tewf/after-hours/blob/main/event_trace.py),
/// which is the only checker; nothing here restates its rules, and its
/// `search` world is the five verbs below.
///
/// **This is off unless a trace is asked for.** Every hook is a null pointer
/// test against a pointer that is null on every published run, next to an atomic
/// increment that already costs more, so no measurement in this repository moves
/// because the hooks exist.
///
/// **One worker only.** Two workers interleave their nodes and a tree cannot be
/// read back out of the result; `decide-rank --trace` refuses `--threads` above
/// one rather than writing something that looks like a tree and is not.
namespace bilinear_rank {

/// Collects events in order, numbering each node as it is opened.
class SearchTrace {
public:
    SearchTrace(std::string tensor, std::size_t target, std::string quotient,
                std::size_t pool_size);

    /// A node is entered. Returns its id, which its children name as parent.
    /// `candidate` is the pool index that reached it; the root's is ignored.
    std::size_t open(std::size_t parent, std::size_t depth, std::size_t candidate, bool is_root);

    /// The one thing the search tests at a node. `what-a-node-cannot-tell-you.md`
    /// records four attempts to test a second and why none of them fires.
    void dimension(std::size_t node, std::size_t value);

    void prune(std::size_t node, const char* why);
    void adopt(std::size_t node, std::size_t products);
    void close(std::size_t node);

    std::size_t nodes() const { return nodes_; }
    std::size_t events() const { return events_.size(); }
    void write(const std::string& path) const;

private:
    enum class Op : std::uint8_t { Open, Bound, Prune, Adopt, Close };
    struct Event {
        Op op;
        std::uint32_t node;
        std::uint32_t number;      // parent, dimension, or product count
        std::uint32_t candidate;
        std::uint16_t depth;
        bool is_root;
        const char* why;           // a literal, only ever set on a prune
    };

    std::string tensor_;
    std::size_t target_;
    std::string quotient_;
    std::size_t pool_size_;
    std::size_t nodes_ = 0;
    std::vector<Event> events_;
};

/// Where a child sits, handed down the recursion. A default-constructed one
/// traces nothing, which is what every call that does not ask for a trace passes.
///
/// It is one parameter rather than four because the two searches it threads
/// through already take eleven and thirteen, and a recursion nobody can read the
/// signature of is how a wrong argument gets passed silently.
struct TraceNode {
    SearchTrace* trace = nullptr;
    std::size_t parent = 0;
    std::size_t depth = 0;
    std::size_t candidate = 0;

    TraceNode child(std::size_t me, std::size_t index) const {
        return TraceNode{trace, me, depth + 1, index};
    }
};

/// Opens a node and guarantees it is settled exactly once.
///
/// The plain search returns from six places and the quotiented one from more,
/// and a node that is opened and never closed makes the trace unreadable in a
/// way that no test of the search itself would catch. So the close is a
/// destructor: a `return` that forgets to say why still says that the node is
/// done, and `prune` or `adopt` mark it settled so the destructor stays quiet.
class TraceScope {
public:
    TraceScope(const TraceNode& where, bool is_root)
        : trace_(where.trace),
          id_(where.trace != nullptr
                  ? where.trace->open(where.parent, where.depth, where.candidate, is_root)
                  : 0) {}
    ~TraceScope() { if (trace_ != nullptr) trace_->close(id_); }

    TraceScope(const TraceScope&) = delete;
    TraceScope& operator=(const TraceScope&) = delete;

    std::size_t id() const { return id_; }
    void dimension(std::size_t value) { if (trace_ != nullptr) trace_->dimension(id_, value); }
    void prune(const char* why) { if (trace_ != nullptr) { trace_->prune(id_, why); trace_ = nullptr; } }
    void adopt(std::size_t products) { if (trace_ != nullptr) { trace_->adopt(id_, products); trace_ = nullptr; } }

private:
    SearchTrace* trace_;
    std::size_t id_;
};

}  // namespace bilinear_rank
