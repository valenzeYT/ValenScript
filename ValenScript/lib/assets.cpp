#include <algorithm>
#include <string>
#include <vector>

namespace assets_lib {
namespace {
std::vector<std::string> g_assets;
}

void add(const std::string& path) { g_assets.push_back(path); }

void remove(const std::string& path) {
    g_assets.erase(std::remove(g_assets.begin(), g_assets.end(), path), g_assets.end());
}

bool has(const std::string& path) {
    return std::find(g_assets.begin(), g_assets.end(), path) != g_assets.end();
}

int count() { return static_cast<int>(g_assets.size()); }

void clear() { g_assets.clear(); }

} // namespace assets_lib
