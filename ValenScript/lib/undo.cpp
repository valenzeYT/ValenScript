#include <string>
#include <vector>

namespace undo_lib {

static std::vector<std::string> undoStack;
static std::vector<std::string> redoStack;

void push(const std::string& action) {
    undoStack.push_back(action);
    redoStack.clear();
}

std::string undo() {
    if (undoStack.empty()) return "";
    std::string action = undoStack.back();
    undoStack.pop_back();
    redoStack.push_back(action);
    return action;
}

std::string redo() {
    if (redoStack.empty()) return "";
    std::string action = redoStack.back();
    redoStack.pop_back();
    undoStack.push_back(action);
    return action;
}

bool can_undo() {
    return !undoStack.empty();
}

bool can_redo() {
    return !redoStack.empty();
}

void clear() {
    undoStack.clear();
    redoStack.clear();
}

} // namespace undo_lib
