#include <fstream>
#include <string>
#include <unordered_map>

namespace project_lib {
namespace {
std::string g_name = "untitled";
std::unordered_map<std::string, std::string> g_values;
}

void new_project(const std::string& name) {
    g_name = name;
    g_values.clear();
}

std::string name() { return g_name; }

void set_value(const std::string& key, const std::string& value) { g_values[key] = value; }

std::string get_value(const std::string& key) {
    auto it = g_values.find(key);
    if (it == g_values.end()) return "";
    return it->second;
}

void save(const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    out << "name=" << g_name << "\n";
    for (const auto& kv : g_values) {
        out << kv.first << "=" << kv.second << "\n";
    }
}

std::string load(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return "";
    std::string line;
    g_values.clear();
    while (std::getline(in, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string k = line.substr(0, pos);
        std::string v = line.substr(pos + 1);
        if (k == "name") {
            g_name = v;
        } else {
            g_values[k] = v;
        }
    }
    return g_name;
}

} // namespace project_lib
