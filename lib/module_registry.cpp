#include "../include/module_registry.h"
#include <filesystem>
#include <unordered_map>
#include <utility>
#include <vector>
#include <windows.h>
#include <iostream>

namespace module_registry {

namespace {
std::unordered_map<std::string, ModuleInitializer>& registry() {
    static std::unordered_map<std::string, ModuleInitializer> map;
    return map;
}

std::filesystem::path exeDir() {
    char buf[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buf).parent_path();
}
} // namespace

void registerModule(const std::string& name, ModuleInitializer initializer) {
    registry()[name] = std::move(initializer);
}

const ModuleInitializer* getModule(const std::string& name) {
    auto it = registry().find(name);
    if (it == registry().end()) {
        return nullptr;
    }
    return &it->second;
}

bool loadPlugin(const std::string& name) {
    if (getModule(name)) {
        return true;
    }

    const std::filesystem::path dllName = name + ".dll";
    std::vector<std::filesystem::path> candidates;
    const std::filesystem::path exePath = exeDir();
    candidates.push_back(exePath / "lib" / dllName);
    candidates.push_back(exePath / ".." / "lib" / dllName);
    candidates.push_back(std::filesystem::path("lib") / dllName);

    std::filesystem::path dllPath;
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            dllPath = path;
            break;
        }
    }
    if (dllPath.empty()) {
        return false;
    }

    HMODULE handle = LoadLibraryA(dllPath.string().c_str());
    if (!handle) {
        const DWORD err = GetLastError();
        std::cerr << "Failed to load plugin '" << name << "' from " << dllPath.string()
                  << " (error " << err << ")\n";
        return false;
    }

    using RegisterFn = void (*)();
    auto fn = reinterpret_cast<RegisterFn>(GetProcAddress(handle, "register_module"));
    if (!fn) {
        const DWORD err = GetLastError();
        std::cerr << "Failed to find register_module in '" << name << "' (error " << err << ")\n";
        FreeLibrary(handle);
        return false;
    }

    static std::vector<HMODULE> handles;
    handles.push_back(handle);

    fn();

    return getModule(name) != nullptr;
}

} // namespace module_registry
