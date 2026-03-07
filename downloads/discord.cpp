#include "../include/interpreter.h"
#include "../include/module_registry.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace discord_lib {
namespace {

constexpr const char kUserAgent[] = "ValenScript discord/1.0";

struct HttpResponse {
    bool ok = false;
    int status = 0;
    std::string url;
    std::string contentType;
    std::string body;
    std::string error;
};

struct ScopedHInternet {
    explicit ScopedHInternet(HINTERNET h = nullptr) : handle(h) {}
    ScopedHInternet(const ScopedHInternet&) = delete;
    ScopedHInternet& operator=(const ScopedHInternet&) = delete;
    ~ScopedHInternet() {
        if (handle) {
            WinHttpCloseHandle(handle);
        }
    }
    HINTERNET get() const { return handle; }
    operator bool() const { return handle != nullptr; }

private:
    HINTERNET handle;
};

static std::string trimNulls(std::string s) {
    while (!s.empty() && s.back() == '\0') {
        s.pop_back();
    }
    return s;
}

static std::wstring toWide(const std::string& input) {
    if (input.empty()) {
        return L"";
    }
    int required = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (required <= 0) {
        throw std::runtime_error("discord: failed UTF-8->UTF-16 conversion");
    }
    std::wstring out(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, out.data(), required);
    out.resize(static_cast<size_t>(required - 1));
    return out;
}

static std::string wideToUtf8(const std::wstring& input) {
    if (input.empty()) {
        return "";
    }
    int required = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return "";
    }
    std::string out(static_cast<size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, out.data(), required, nullptr, nullptr);
    return trimNulls(out);
}

static std::string queryHeaderString(HINTERNET request, DWORD query) {
    DWORD size = 0;
    WinHttpQueryHeaders(request, query, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &size, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
        return "";
    }

    std::wstring buffer(static_cast<size_t>(size / sizeof(wchar_t)), L'\0');
    if (!WinHttpQueryHeaders(request, query, WINHTTP_HEADER_NAME_BY_INDEX, buffer.data(), &size, WINHTTP_NO_HEADER_INDEX)) {
        return "";
    }
    return wideToUtf8(buffer.c_str());
}

struct ParsedUrl {
    bool secure = false;
    INTERNET_PORT port = 0;
    std::wstring host;
    std::wstring pathAndQuery;
};

static ParsedUrl crackUrl(const std::string& url) {
    ParsedUrl out;
    std::wstring wurl = toWide(url);

    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength = static_cast<DWORD>(-1);
    uc.dwHostNameLength = static_cast<DWORD>(-1);
    uc.dwUrlPathLength = static_cast<DWORD>(-1);
    uc.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
        throw std::runtime_error("discord: invalid URL");
    }

    out.secure = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    out.port = uc.nPort;
    out.host.assign(uc.lpszHostName, uc.dwHostNameLength);

    std::wstring path;
    if (uc.lpszUrlPath && uc.dwUrlPathLength > 0) {
        path.assign(uc.lpszUrlPath, uc.dwUrlPathLength);
    } else {
        path = L"/";
    }
    std::wstring extra;
    if (uc.lpszExtraInfo && uc.dwExtraInfoLength > 0) {
        extra.assign(uc.lpszExtraInfo, uc.dwExtraInfoLength);
    }
    out.pathAndQuery = path + extra;
    return out;
}

static HttpResponse httpRequest(const std::string& method,
                                const std::string& url,
                                const std::string& body,
                                const std::string& contentType) {
    HttpResponse res;
    res.url = url;

    ParsedUrl p = crackUrl(url);
    const DWORD flags = p.secure ? WINHTTP_FLAG_SECURE : 0;

    ScopedHInternet session(WinHttpOpen(toWide(kUserAgent).c_str(),
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME,
                                        WINHTTP_NO_PROXY_BYPASS,
                                        0));
    if (!session) {
        throw std::runtime_error("discord: WinHttpOpen failed: " + std::to_string(GetLastError()));
    }

    ScopedHInternet connect(WinHttpConnect(session.get(), p.host.c_str(), p.port, 0));
    if (!connect) {
        throw std::runtime_error("discord: WinHttpConnect failed: " + std::to_string(GetLastError()));
    }

    ScopedHInternet request(WinHttpOpenRequest(connect.get(),
                                               toWide(method).c_str(),
                                               p.pathAndQuery.c_str(),
                                               nullptr,
                                               WINHTTP_NO_REFERER,
                                               WINHTTP_DEFAULT_ACCEPT_TYPES,
                                               flags));
    if (!request) {
        throw std::runtime_error("discord: WinHttpOpenRequest failed: " + std::to_string(GetLastError()));
    }

    WinHttpSetTimeouts(request.get(), 8000, 8000, 8000, 20000);

    std::wstring headers;
    if (!contentType.empty()) {
        headers = L"Content-Type: " + toWide(contentType) + L"\r\n";
    }

    const DWORD totalSize = static_cast<DWORD>(body.size());
    const bool okSend = WinHttpSendRequest(request.get(),
                                           headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
                                           headers.empty() ? 0 : static_cast<DWORD>(headers.size()),
                                           body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()),
                                           body.empty() ? 0 : totalSize,
                                           totalSize,
                                           0) == TRUE;
    if (!okSend) {
        throw std::runtime_error("discord: WinHttpSendRequest failed: " + std::to_string(GetLastError()));
    }

    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        throw std::runtime_error("discord: WinHttpReceiveResponse failed: " + std::to_string(GetLastError()));
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (WinHttpQueryHeaders(request.get(),
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &status,
                            &statusSize,
                            WINHTTP_NO_HEADER_INDEX)) {
        res.status = static_cast<int>(status);
    }

    res.contentType = queryHeaderString(request.get(), WINHTTP_QUERY_CONTENT_TYPE);

    std::string responseBody;
    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            throw std::runtime_error("discord: WinHttpQueryDataAvailable failed: " + std::to_string(GetLastError()));
        }
        if (available == 0) {
            break;
        }
        std::vector<char> buffer(available);
        DWORD read = 0;
        if (!WinHttpReadData(request.get(), buffer.data(), available, &read)) {
            throw std::runtime_error("discord: WinHttpReadData failed: " + std::to_string(GetLastError()));
        }
        responseBody.append(buffer.data(), static_cast<size_t>(read));
    }

    res.body = std::move(responseBody);
    res.ok = (res.status >= 200 && res.status <= 299);
    return res;
}

static Value toValue(const HttpResponse& r) {
    std::unordered_map<std::string, Value> out;
    out["ok"] = Value::fromBool(r.ok);
    out["status"] = Value::fromNumber(static_cast<double>(r.status));
    out["url"] = Value::fromString(r.url);
    out["content_type"] = Value::fromString(r.contentType);
    out["body"] = Value::fromString(r.body);
    if (!r.error.empty()) {
        out["error"] = Value::fromString(r.error);
    }
    return Value::fromMap(std::move(out));
}

static Value safeRequest(const std::string& method,
                         const std::string& url,
                         const std::string& body,
                         const std::string& contentType) {
    try {
        HttpResponse r = httpRequest(method, url, body, contentType);
        return toValue(r);
    } catch (const std::exception& e) {
        HttpResponse r;
        r.ok = false;
        r.status = 0;
        r.url = url;
        r.error = e.what();
        return toValue(r);
    } catch (...) {
        HttpResponse r;
        r.ok = false;
        r.status = 0;
        r.url = url;
        r.error = "discord: unknown error";
        return toValue(r);
    }
}

static std::string jsonEscapeString(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (unsigned char c : input) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    out += oss.str();
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    return out;
}

static Value webhookSendText(const std::string& webhookUrl, const std::string& content) {
    const std::string payload = std::string("{\"content\":\"") + jsonEscapeString(content) + "\"}";
    return safeRequest("POST", webhookUrl, payload, "application/json");
}

} // namespace

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("discord", [](Interpreter& interp) {
        interp.registerModuleFunction("discord", "webhook_send_text", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 2, "discord.webhook_send_text");
            const std::string url = interp.expectString(args[0], "discord.webhook_send_text url must be a string");
            const std::string content = interp.expectString(args[1], "discord.webhook_send_text content must be a string");
            return webhookSendText(url, content);
        });

        interp.registerModuleFunction("discord", "webhook_send_json", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 2, "discord.webhook_send_json");
            const std::string url = interp.expectString(args[0], "discord.webhook_send_json url must be a string");
            const std::string payload = interp.expectString(args[1], "discord.webhook_send_json payload must be a string");
            return safeRequest("POST", url, payload, "application/json");
        });

        interp.registerModuleFunction("discord", "webhook_info", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 1, "discord.webhook_info");
            const std::string url = interp.expectString(args[0], "discord.webhook_info url must be a string");
            return safeRequest("GET", url, "", "");
        });

        interp.registerModuleFunction("discord", "request", [&interp](const std::vector<Value>& args) -> Value {
            if (args.size() < 2 || args.size() > 4) {
                throw std::runtime_error("discord.request expects 2-4 argument(s): method, url, body?, content_type?");
            }
            const std::string method = interp.expectString(args[0], "discord.request method must be a string");
            const std::string url = interp.expectString(args[1], "discord.request url must be a string");
            std::string body;
            std::string contentType;
            if (args.size() >= 3) {
                body = interp.expectString(args[2], "discord.request body must be a string");
            }
            if (args.size() == 4) {
                contentType = interp.expectString(args[3], "discord.request content_type must be a string");
            }
            return safeRequest(method, url, body, contentType);
        });
    });
}

} // namespace discord_lib
