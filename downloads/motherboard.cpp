#include "../include/interpreter.h"
#include <vector>
#include "../include/module_registry.h"
#include <string>
#include <windows.h>

namespace motherboard_lib {
namespace {
std::string trim_nulls(std::string s) {
    while (!s.empty() && s.back() == '\0') {
        s.pop_back();
    }
    return s;
}

std::string read_bios_value(const char* valueName) {
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\BIOS",
                      0,
                      KEY_READ,
                      &key) != ERROR_SUCCESS) {
        return "";
    }

    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExA(key, valueName, nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) || size == 0) {
        RegCloseKey(key);
        return "";
    }

    std::string value(size, '\0');
    if (RegQueryValueExA(key, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(&value[0]), &size) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return "";
    }
    RegCloseKey(key);
    return trim_nulls(value);
}
} // namespace

std::string baseboard_manufacturer() { return read_bios_value("BaseBoardManufacturer"); }
std::string baseboard_product() { return read_bios_value("BaseBoardProduct"); }
std::string baseboard_version() { return read_bios_value("BaseBoardVersion"); }
std::string baseboard_serial() { return read_bios_value("BaseBoardSerialNumber"); }
std::string bios_vendor() { return read_bios_value("BIOSVendor"); }
std::string bios_version() { return read_bios_value("BIOSVersion"); }
std::string bios_release_date() { return read_bios_value("BIOSReleaseDate"); }
std::string system_manufacturer() { return read_bios_value("SystemManufacturer"); }
std::string system_product_name() { return read_bios_value("SystemProductName"); }
std::string system_sku() { return read_bios_value("SystemSKU"); }
std::string system_family() { return read_bios_value("SystemFamily"); }

bool has_data() {
    return !baseboard_manufacturer().empty() ||
           !baseboard_product().empty() ||
           !bios_vendor().empty();
}

} // namespace motherboard_lib

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("motherboard", [](Interpreter& interp) {
                    interp.registerModuleFunction("motherboard", "manufacturer", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.manufacturer");
                        return Value::fromString(motherboard_lib::baseboard_manufacturer());
                    });
                    interp.registerModuleFunction("motherboard", "product", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.product");
                        return Value::fromString(motherboard_lib::baseboard_product());
                    });
                    interp.registerModuleFunction("motherboard", "version", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.version");
                        return Value::fromString(motherboard_lib::baseboard_version());
                    });
                    interp.registerModuleFunction("motherboard", "serial", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.serial");
                        return Value::fromString(motherboard_lib::baseboard_serial());
                    });
                    interp.registerModuleFunction("motherboard", "bios_vendor", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.bios_vendor");
                        return Value::fromString(motherboard_lib::bios_vendor());
                    });
                    interp.registerModuleFunction("motherboard", "bios_version", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.bios_version");
                        return Value::fromString(motherboard_lib::bios_version());
                    });
                    interp.registerModuleFunction("motherboard", "bios_release_date", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.bios_release_date");
                        return Value::fromString(motherboard_lib::bios_release_date());
                    });
                    interp.registerModuleFunction("motherboard", "system_manufacturer", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.system_manufacturer");
                        return Value::fromString(motherboard_lib::system_manufacturer());
                    });
                    interp.registerModuleFunction("motherboard", "system_product", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.system_product");
                        return Value::fromString(motherboard_lib::system_product_name());
                    });
                    interp.registerModuleFunction("motherboard", "system_sku", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.system_sku");
                        return Value::fromString(motherboard_lib::system_sku());
                    });
                    interp.registerModuleFunction("motherboard", "system_family", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.system_family");
                        return Value::fromString(motherboard_lib::system_family());
                    });
                    interp.registerModuleFunction("motherboard", "has_data", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "motherboard.has_data");
                        return Value::fromBool(motherboard_lib::has_data());
                    });

    });
}
