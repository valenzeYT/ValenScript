#include "../include/module_registry.h"
#include <unordered_map>
#include <utility>

namespace module_registry {

namespace {
std::unordered_map<std::string, ModuleInitializer>& registry() {
    static std::unordered_map<std::string, ModuleInitializer> map;
    return map;
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

} // namespace module_registry
