#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>

namespace perf_lib {
namespace {
std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_starts;
}

void start(const std::string& label) { g_starts[label] = std::chrono::steady_clock::now(); }

double stop(const std::string& label) {
    auto it = g_starts.find(label);
    if (it == g_starts.end()) return 0.0;
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> dt = now - it->second;
    g_starts.erase(it);
    return dt.count();
}

double now_ms() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double, std::milli>(now).count();
}

void sleep_ms(double ms) {
    if (ms <= 0.0) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(ms)));
}

} // namespace perf_lib
