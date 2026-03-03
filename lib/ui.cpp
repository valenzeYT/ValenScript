#include <algorithm>
#include <string>

namespace ui_lib {
namespace {
std::string g_status;
double g_progress = 0.0;
}

void set_status(const std::string& text) { g_status = text; }
std::string status() { return g_status; }

void set_progress(double v) {
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    g_progress = v;
}
double progress() { return g_progress; }

} // namespace ui_lib
