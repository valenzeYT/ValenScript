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
