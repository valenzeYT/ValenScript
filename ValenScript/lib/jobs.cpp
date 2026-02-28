#include <deque>
#include <string>

namespace jobs_lib {
namespace {
std::deque<std::string> g_queue;
}

void push(const std::string& job) { g_queue.push_back(job); }

std::string next() {
    if (g_queue.empty()) return "";
    std::string job = g_queue.front();
    g_queue.pop_front();
    return job;
}

int count() { return static_cast<int>(g_queue.size()); }
void clear() { g_queue.clear(); }

} // namespace jobs_lib
