#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string>
#include <vector>
#include <sstream>

namespace net_lib {
namespace {
class WsaInit {
public:
    WsaInit() { WSAStartup(MAKEWORD(2, 2), &data); }
    ~WsaInit() { WSACleanup(); }
private:
    WSADATA data;
};

std::vector<std::string> address_list(const IP_ADAPTER_UNICAST_ADDRESS* addr) {
    std::vector<std::string> out;
    while (addr) {
        char buffer[INET6_ADDRSTRLEN] = {};
        DWORD size = sizeof(buffer);
        if (addr->Address.lpSockaddr) {
            if (addr->Address.lpSockaddr->sa_family == AF_INET) {
                auto* in = reinterpret_cast<sockaddr_in*>(addr->Address.lpSockaddr);
                inet_ntop(AF_INET, &in->sin_addr, buffer, size);
            } else if (addr->Address.lpSockaddr->sa_family == AF_INET6) {
                auto* in6 = reinterpret_cast<sockaddr_in6*>(addr->Address.lpSockaddr);
                inet_ntop(AF_INET6, &in6->sin6_addr, buffer, size);
            }
            if (buffer[0]) {
                out.push_back(buffer);
            }
        }
        addr = addr->Next;
    }
    return out;
}

std::string join_addresses(const std::vector<std::string>& list) {
    std::ostringstream oss;
    for (size_t i = 0; i < list.size(); ++i) {
        if (i) oss << ",";
        oss << list[i];
    }
    return oss.str();
}
} // namespace

std::string local_ip() {
    WsaInit init;
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return "";
    sockaddr_in remote{};
    remote.sin_family = AF_INET;
    inet_pton(AF_INET, "1.1.1.1", &remote.sin_addr);
    remote.sin_port = htons(53);
    connect(sock, reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
    sockaddr_in local{};
    int len = sizeof(local);
    getsockname(sock, reinterpret_cast<sockaddr*>(&local), &len);
    closesocket(sock);
    char buffer[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &local.sin_addr, buffer, sizeof(buffer));
    return buffer;
}

std::string interfaces() {
    WsaInit init;
    ULONG size = 0;
    GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &size);
    std::vector<BYTE> buffer(size);
    IP_ADAPTER_ADDRESSES* addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, addresses, &size) != NO_ERROR) {
        return "";
    }
    std::ostringstream oss;
    for (IP_ADAPTER_ADDRESSES* adapter = addresses; adapter; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp) continue;
        std::vector<std::string> addrs = address_list(adapter->FirstUnicastAddress);
        if (addrs.empty()) continue;
        if (!oss.str().empty()) oss << "|";
        oss << adapter->AdapterName << ":" << join_addresses(addrs);
    }
    return oss.str();
}

std::string resolve(const std::string& host) {
    WsaInit init;
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0) {
        return "";
    }
    std::ostringstream oss;
    for (addrinfo* ptr = result; ptr; ptr = ptr->ai_next) {
        char buffer[INET6_ADDRSTRLEN] = {};
        void* addr = nullptr;
        if (ptr->ai_family == AF_INET) {
            addr = &reinterpret_cast<sockaddr_in*>(ptr->ai_addr)->sin_addr;
        } else if (ptr->ai_family == AF_INET6) {
            addr = &reinterpret_cast<sockaddr_in6*>(ptr->ai_addr)->sin6_addr;
        }
        if (addr && inet_ntop(ptr->ai_family, addr, buffer, sizeof(buffer))) {
            if (!oss.str().empty()) oss << ",";
            oss << buffer;
        }
    }
    freeaddrinfo(result);
    return oss.str();
}

} // namespace net_lib
