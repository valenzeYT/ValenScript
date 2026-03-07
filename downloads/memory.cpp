#include "../include/interpreter.h"
#include <vector>
#include "../include/module_registry.h"
#include <windows.h>

namespace memory_lib {
namespace {
MEMORYSTATUSEX snapshot() {
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    return ms;
}
} // namespace

double total_physical_bytes() {
    return static_cast<double>(snapshot().ullTotalPhys);
}

double available_physical_bytes() {
    return static_cast<double>(snapshot().ullAvailPhys);
}

double used_physical_bytes() {
    MEMORYSTATUSEX ms = snapshot();
    return static_cast<double>(ms.ullTotalPhys - ms.ullAvailPhys);
}

double total_virtual_bytes() {
    return static_cast<double>(snapshot().ullTotalVirtual);
}

double available_virtual_bytes() {
    return static_cast<double>(snapshot().ullAvailVirtual);
}

double used_virtual_bytes() {
    MEMORYSTATUSEX ms = snapshot();
    return static_cast<double>(ms.ullTotalVirtual - ms.ullAvailVirtual);
}

double total_page_file_bytes() {
    return static_cast<double>(snapshot().ullTotalPageFile);
}

double available_page_file_bytes() {
    return static_cast<double>(snapshot().ullAvailPageFile);
}

double used_page_file_bytes() {
    MEMORYSTATUSEX ms = snapshot();
    return static_cast<double>(ms.ullTotalPageFile - ms.ullAvailPageFile);
}

double load_percent() {
    return static_cast<double>(snapshot().dwMemoryLoad);
}

} // namespace memory_lib

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("memory", [](Interpreter& interp) {
                    interp.registerModuleFunction("memory", "load_percent", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.load_percent");
                        return Value::fromNumber(memory_lib::load_percent());
                    });
                    interp.registerModuleFunction("memory", "total_physical_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.total_physical_bytes");
                        return Value::fromNumber(memory_lib::total_physical_bytes());
                    });
                    interp.registerModuleFunction("memory", "available_physical_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.available_physical_bytes");
                        return Value::fromNumber(memory_lib::available_physical_bytes());
                    });
                    interp.registerModuleFunction("memory", "used_physical_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.used_physical_bytes");
                        return Value::fromNumber(memory_lib::used_physical_bytes());
                    });
                    interp.registerModuleFunction("memory", "total_virtual_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.total_virtual_bytes");
                        return Value::fromNumber(memory_lib::total_virtual_bytes());
                    });
                    interp.registerModuleFunction("memory", "available_virtual_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.available_virtual_bytes");
                        return Value::fromNumber(memory_lib::available_virtual_bytes());
                    });
                    interp.registerModuleFunction("memory", "used_virtual_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.used_virtual_bytes");
                        return Value::fromNumber(memory_lib::used_virtual_bytes());
                    });
                    interp.registerModuleFunction("memory", "total_page_file_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.total_page_file_bytes");
                        return Value::fromNumber(memory_lib::total_page_file_bytes());
                    });
                    interp.registerModuleFunction("memory", "available_page_file_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.available_page_file_bytes");
                        return Value::fromNumber(memory_lib::available_page_file_bytes());
                    });
                    interp.registerModuleFunction("memory", "used_page_file_bytes", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "memory.used_page_file_bytes");
                        return Value::fromNumber(memory_lib::used_page_file_bytes());
                    });

    });
}
