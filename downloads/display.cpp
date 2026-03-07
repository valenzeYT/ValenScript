#include "../include/interpreter.h"
#include "../include/module_registry.h"
#include <windows.h>
#include <string>
#include <vector>

namespace display_lib {
namespace {
std::string to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string out(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &out[0], size, nullptr, nullptr);
    return out;
}

struct MonitorDetail {
    std::string name;
    int width;
    int height;
    int refresh;
};

std::vector<MonitorDetail> enumerate_monitors() {
    std::vector<MonitorDetail> out;
    DISPLAY_DEVICEW device{};
    device.cb = sizeof(device);
    DWORD index = 0;
    while (EnumDisplayDevicesW(nullptr, index, &device, 0)) {
        if (!(device.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
            ++index;
            continue;
        }
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        if (EnumDisplaySettingsW(device.DeviceName, ENUM_CURRENT_SETTINGS, &mode)) {
            out.push_back({
                to_utf8(device.DeviceString),
                static_cast<int>(mode.dmPelsWidth),
                static_cast<int>(mode.dmPelsHeight),
                static_cast<int>(mode.dmDisplayFrequency)
            });
        }
        ++index;
    }
    return out;
}
} // namespace

int monitor_count() {
    return static_cast<int>(enumerate_monitors().size());
}

std::string monitor_list() {
    auto monitors = enumerate_monitors();
    std::string result;
    for (size_t i = 0; i < monitors.size(); ++i) {
        if (i) result += "|";
        result += monitors[i].name + ":" +
                  std::to_string(monitors[i].width) + "x" + std::to_string(monitors[i].height) +
                  "@" + std::to_string(monitors[i].refresh);
    }
    return result;
}

int primary_width() {
    auto monitors = enumerate_monitors();
    if (monitors.empty()) return 0;
    return monitors[0].width;
}

int primary_height() {
    auto monitors = enumerate_monitors();
    if (monitors.empty()) return 0;
    return monitors[0].height;
}

int primary_refresh_rate() {
    auto monitors = enumerate_monitors();
    if (monitors.empty()) return 0;
    return monitors[0].refresh;
}

std::string monitor_info(int index) {
    auto monitors = enumerate_monitors();
    if (index < 0 || static_cast<size_t>(index) >= monitors.size()) return "";
    const auto& m = monitors[index];
    return m.name + ":" + std::to_string(m.width) + "x" + std::to_string(m.height) + "@" + std::to_string(m.refresh);
}

} // namespace display_lib

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("display", [](Interpreter& interp) {
                    interp.registerModuleFunction("display", "monitor_count", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "display.monitor_count");
                        return Value::fromNumber(static_cast<double>(display_lib::monitor_count()));
                    });
                    interp.registerModuleFunction("display", "monitor_list", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "display.monitor_list");
                        return Value::fromString(display_lib::monitor_list());
                    });
                    interp.registerModuleFunction("display", "primary_width", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "display.primary_width");
                        return Value::fromNumber(static_cast<double>(display_lib::primary_width()));
                    });
                    interp.registerModuleFunction("display", "primary_height", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "display.primary_height");
                        return Value::fromNumber(static_cast<double>(display_lib::primary_height()));
                    });
                    interp.registerModuleFunction("display", "primary_refresh", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "display.primary_refresh");
                        return Value::fromNumber(static_cast<double>(display_lib::primary_refresh_rate()));
                    });
                    interp.registerModuleFunction("display", "monitor_info", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "display.monitor_info");
                        int idx = static_cast<int>(interp.expectNumber(args[0], "display.monitor_info expects index"));
                        return Value::fromString(display_lib::monitor_info(idx));
                    });

    });
}
