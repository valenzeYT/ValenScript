#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <windows.h>
#include <tlhelp32.h>

namespace cpu_lib {
namespace {
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

    while (!value.empty() && (value.back() == '\0' || value.back() == '\r' || value.back() == '\n')) {
        value.pop_back();
    }
    return value;
}

double read_reg_number(HKEY root, const char* path, const char* name) {
    HKEY key = nullptr;
    if (RegOpenKeyExA(root, path, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return 0.0;
    }

    DWORD type = 0;
    DWORD data = 0;
    DWORD size = sizeof(data);
    if (RegQueryValueExA(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(&data), &size) != ERROR_SUCCESS ||
        type != REG_DWORD) {
        RegCloseKey(key);
        return 0.0;
    }
    RegCloseKey(key);
    return static_cast<double>(data);
}

std::uint64_t ft_to_u64(const FILETIME& ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return static_cast<std::uint64_t>(uli.QuadPart);
}
} // namespace

int logical_cores() {
    unsigned int n = std::thread::hardware_concurrency();
    return (n == 0) ? 1 : static_cast<int>(n);
}

std::string architecture() {
    SYSTEM_INFO si{};
    GetNativeSystemInfo(&si);
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
        case PROCESSOR_ARCHITECTURE_ARM64: return "arm64";
        case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
        case PROCESSOR_ARCHITECTURE_ARM: return "arm";
        default: return "unknown";
    }
}

double page_size() {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    return static_cast<double>(si.dwPageSize);
}

double allocation_granularity() {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    return static_cast<double>(si.dwAllocationGranularity);
}

double process_id() {
    return static_cast<double>(GetCurrentProcessId());
}

double process_thread_count() {
    DWORD pid = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0.0;
    }

    THREADENTRY32 te{};
    te.dwSize = sizeof(te);
    int count = 0;
    if (Thread32First(snapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                ++count;
            }
        } while (Thread32Next(snapshot, &te));
    }
    CloseHandle(snapshot);
    return static_cast<double>(count);
}

double usage_percent(int sampleMs) {
    if (sampleMs < 50) {
        sampleMs = 50;
    }

    FILETIME idle1{}, kernel1{}, user1{};
    FILETIME idle2{}, kernel2{}, user2{};

    if (!GetSystemTimes(&idle1, &kernel1, &user1)) {
        return 0.0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(sampleMs));
    if (!GetSystemTimes(&idle2, &kernel2, &user2)) {
        return 0.0;
    }

    std::uint64_t idle = ft_to_u64(idle2) - ft_to_u64(idle1);
    std::uint64_t kernel = ft_to_u64(kernel2) - ft_to_u64(kernel1);
    std::uint64_t user = ft_to_u64(user2) - ft_to_u64(user1);

    std::uint64_t total = kernel + user;
    if (total == 0) {
        return 0.0;
    }

    double busy = static_cast<double>(total - idle) * 100.0 / static_cast<double>(total);
    if (busy < 0.0) busy = 0.0;
    if (busy > 100.0) busy = 100.0;
    return busy;
}

double process_cpu_time_ms() {
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        return 0.0;
    }
    std::uint64_t ticks = ft_to_u64(kernel) + ft_to_u64(user);
    return static_cast<double>(ticks) / 10000.0;
}

std::string vendor() {
    return read_reg_string(HKEY_LOCAL_MACHINE,
                           "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                           "VendorIdentifier");
}

std::string brand() {
    return read_reg_string(HKEY_LOCAL_MACHINE,
                           "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                           "ProcessorNameString");
}

double base_mhz() {
    return read_reg_number(HKEY_LOCAL_MACHINE,
                           "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                           "~MHz");
}

} // namespace cpu_lib
