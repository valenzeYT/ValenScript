#include <string>
#include <vector>

namespace history_lib {
namespace {
std::vector<std::string> g_undo;
std::vector<std::string> g_redo;
}

void push(const std::string& action) {
    g_undo.push_back(action);
    g_redo.clear();
}

std::string undo() {
    if (g_undo.empty()) return "";
    std::string action = g_undo.back();
    g_undo.pop_back();
    g_redo.push_back(action);
    return action;
}

std::string redo() {
    if (g_redo.empty()) return "";
    std::string action = g_redo.back();
    g_redo.pop_back();
    g_undo.push_back(action);
    return action;
}

bool can_undo() { return !g_undo.empty(); }
bool can_redo() { return !g_redo.empty(); }

void clear() {
    g_undo.clear();
    g_redo.clear();
}

} // namespace history_lib
