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
