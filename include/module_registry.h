#ifndef MODULE_REGISTRY_H
#define MODULE_REGISTRY_H

#include <functional>
#include <string>

class Interpreter;

namespace module_registry {
using ModuleInitializer = std::function<void(Interpreter&)>;

void registerModule(const std::string& name, ModuleInitializer initializer);
const ModuleInitializer* getModule(const std::string& name);

} // namespace module_registry

#endif
