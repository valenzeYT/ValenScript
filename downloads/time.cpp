#include "../include/module_registry.h"
#include "../include/interpreter.h"
#include <chrono>
#include <cstdint>
#include <vector>

namespace time_lib {

std::int64_t get_unix() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

std::int64_t get_unix_ms() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

std::int64_t get_unix_us() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

} // namespace time_lib

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("time", [](Interpreter& interp) {
        interp.registerModuleFunction("time", "unix",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 0, "time.unix");
                                          return Value::fromNumber(static_cast<double>(time_lib::get_unix()));
                                      });
        interp.registerModuleFunction("time", "unix_ms",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 0, "time.unix_ms");
                                          return Value::fromNumber(static_cast<double>(time_lib::get_unix_ms()));
                                      });
        interp.registerModuleFunction("time", "unix_us",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 0, "time.unix_us");
                                          return Value::fromNumber(static_cast<double>(time_lib::get_unix_us()));
                                      });
    });
}
