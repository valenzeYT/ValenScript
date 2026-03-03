#include "../include/module_registry.h"
#include "../include/interpreter.h"

namespace {
struct ExampleModuleRegistrar {
    ExampleModuleRegistrar() {
        module_registry::registerModule("example", [](Interpreter& interp) {
            interp.registerModuleFunction("example", "greet",
                                          [&interp](const std::vector<Value>& args) -> Value {
                                              interp.expectArity(args, 0, "example.greet");
                                              return Value::fromString("example module greeting");
                                          });
        });
    }
};

static ExampleModuleRegistrar exampleModuleRegistrar;
} // namespace
