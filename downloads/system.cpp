#include "../include/interpreter.h"
#include "../include/module_registry.h"
#include <string>
#include <vector>
#include <windows.h>
#include <Lmcons.h>

namespace system_lib {
namespace {
std::string trim_nulls(std::string s) {
    while (!s.empty() && s.back() == '\0') {
        s.pop_back();
    }
    return s;
}

std::string utf16_to_utf8(const std::wstring& ws) {
    if (ws.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) return "";
    std::vector<char> out(static_cast<size_t>(sizeNeeded), '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), sizeNeeded, nullptr, nullptr);
    return trim_nulls(std::string(out.data()));
}

std::string read_reg_string(HKEY root, const char* path, const char* name) {
    HKEY key = nullptr;
    if (RegOpenKeyExA(root, path, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return "";
    }

    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExA(key, name, nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) || size == 0) {
        RegCloseKey(key);
        return "";
    }

    std::string value(size, '\0');
    if (RegQueryValueExA(key, name, nullptr, nullptr, reinterpret_cast<LPBYTE>(&value[0]), &size) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return "";
    }
    RegCloseKey(key);
    return trim_nulls(value);
}
} // namespace

std::string computer_name() {
    char buffer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameA(buffer, &size)) return "";
    return std::string(buffer, size);
}

std::string user_name() {
    char buffer[UNLEN + 1] = {};
    DWORD size = UNLEN + 1;
    if (!GetUserNameA(buffer, &size)) return "";
    if (size > 0 && buffer[size - 1] == '\0') {
        --size;
    }
    return std::string(buffer, size);
}

std::string os_name() {
    return read_reg_string(HKEY_LOCAL_MACHINE,
                           "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                           "ProductName");
}

std::string os_display_version() {
    return read_reg_string(HKEY_LOCAL_MACHINE,
                           "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                           "DisplayVersion");
}

std::string os_build_number() {
    return read_reg_string(HKEY_LOCAL_MACHINE,
                           "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                           "CurrentBuildNumber");
}

bool is_64bit() {
#if defined(_WIN64)
    return true;
#else
    BOOL wow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &wow64);
    return wow64 == TRUE;
#endif
}

double uptime_seconds() {
    return static_cast<double>(GetTickCount64()) / 1000.0;
}

std::string temp_dir() {
    char buffer[MAX_PATH + 1] = {};
    DWORD len = GetTempPathA(MAX_PATH, buffer);
    if (len == 0 || len > MAX_PATH) return "";
    return std::string(buffer, len);
}

std::string current_dir() {
    char buffer[MAX_PATH + 1] = {};
    DWORD len = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (len == 0 || len > MAX_PATH) return "";
    return std::string(buffer, len);
}

std::string windows_dir() {
    char buffer[MAX_PATH + 1] = {};
    UINT len = GetWindowsDirectoryA(buffer, MAX_PATH);
    if (len == 0 || len > MAX_PATH) return "";
    return std::string(buffer, len);
}

std::string system_dir() {
    char buffer[MAX_PATH + 1] = {};
    UINT len = GetSystemDirectoryA(buffer, MAX_PATH);
    if (len == 0 || len > MAX_PATH) return "";
    return std::string(buffer, len);
}

std::string locale() {
    wchar_t wbuf[LOCALE_NAME_MAX_LENGTH] = {};
    int len = GetUserDefaultLocaleName(wbuf, LOCALE_NAME_MAX_LENGTH);
    if (len <= 0) return "";
    return utf16_to_utf8(std::wstring(wbuf));
}

std::string timezone() {
    TIME_ZONE_INFORMATION tz{};
    DWORD status = GetTimeZoneInformation(&tz);
    if (status == TIME_ZONE_ID_INVALID) {
        return "";
    }
    std::wstring wname = tz.StandardName;
    if (status == TIME_ZONE_ID_DAYLIGHT && tz.DaylightName[0] != L'\0') {
        wname = tz.DaylightName;
    }
    return utf16_to_utf8(wname);
}

std::string machine_guid() {
    return read_reg_string(HKEY_LOCAL_MACHINE,
                           "SOFTWARE\\Microsoft\\Cryptography",
                           "MachineGuid");
}

} // namespace system_lib

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("system", [](Interpreter& interp) {
                    interp.registerModuleFunction("system", "computer_name", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.computer_name");
                        return Value::fromString(system_lib::computer_name());
                    });
                    interp.registerModuleFunction("system", "user_name", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.user_name");
                        return Value::fromString(system_lib::user_name());
                    });
                    interp.registerModuleFunction("system", "os_name", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.os_name");
                        return Value::fromString(system_lib::os_name());
                    });
                    interp.registerModuleFunction("system", "os_display_version", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.os_display_version");
                        return Value::fromString(system_lib::os_display_version());
                    });
                    interp.registerModuleFunction("system", "os_build_number", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.os_build_number");
                        return Value::fromString(system_lib::os_build_number());
                    });
                    interp.registerModuleFunction("system", "is_64bit", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.is_64bit");
                        return Value::fromBool(system_lib::is_64bit());
                    });
                    interp.registerModuleFunction("system", "uptime_seconds", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.uptime_seconds");
                        return Value::fromNumber(system_lib::uptime_seconds());
                    });
                    interp.registerModuleFunction("system", "temp_dir", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.temp_dir");
                        return Value::fromString(system_lib::temp_dir());
                    });
                    interp.registerModuleFunction("system", "current_dir", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.current_dir");
                        return Value::fromString(system_lib::current_dir());
                    });
                    interp.registerModuleFunction("system", "windows_dir", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.windows_dir");
                        return Value::fromString(system_lib::windows_dir());
                    });
                    interp.registerModuleFunction("system", "system_dir", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.system_dir");
                        return Value::fromString(system_lib::system_dir());
                    });
                    interp.registerModuleFunction("system", "locale", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.locale");
                        return Value::fromString(system_lib::locale());
                    });
                    interp.registerModuleFunction("system", "timezone", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.timezone");
                        return Value::fromString(system_lib::timezone());
                    });
                    interp.registerModuleFunction("system", "machine_guid", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "system.machine_guid");
                        return Value::fromString(system_lib::machine_guid());
                    });

    });
}
