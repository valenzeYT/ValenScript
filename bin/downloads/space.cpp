#include "../include/interpreter.h"
#include "../include/module_registry.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <cctype>
#include <ctime>
#include <iomanip>
#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace space_lib {
namespace {

constexpr const char kNasaHost[] = "api.nasa.gov";
constexpr const char kUserAgent[] = "ValenScript space/1.0";
constexpr const char kApiKey[] = "DEMO_KEY";

struct HttpResult {
    bool ok = false;
    int status = 0;
    std::string url;
    std::string contentType;
    std::string body;
    std::string error;
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
        throw std::runtime_error("space: failed UTF-8->UTF-16 conversion");
    }
    std::wstring out(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, out.data(), required);
    out.resize(static_cast<size_t>(required - 1));
    return out;
}

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

static std::string percentEncode(const std::string& input) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex;
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << static_cast<char>(c);
        } else {
            oss << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }
    return oss.str();
}

static std::string todayDate() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    char buf[11] = {};
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &local);
    return std::string(buf);
}

static std::string buildQuery(const std::vector<std::pair<std::string, std::string>>& params) {
    std::string out;
    bool first = true;
    for (const auto& kv : params) {
        if (kv.first.empty()) {
            continue;
        }
        out += first ? "?" : "&";
        first = false;
        out += percentEncode(kv.first);
        out += "=";
        out += percentEncode(kv.second);
    }
    return out;
}

static std::string readHeaderString(HINTERNET request, DWORD query) {
    DWORD size = 0;
    WinHttpQueryHeaders(request, query, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &size, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
        return "";
    }
    std::wstring buffer(static_cast<size_t>(size / sizeof(wchar_t)), L'\0');
    if (!WinHttpQueryHeaders(request, query, WINHTTP_HEADER_NAME_BY_INDEX, buffer.data(), &size, WINHTTP_NO_HEADER_INDEX)) {
        return "";
    }

    const int needed = WideCharToMultiByte(CP_UTF8, 0, buffer.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return "";
    }
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, buffer.c_str(), -1, out.data(), needed, nullptr, nullptr);
    return trimNulls(out);
}

static HttpResult httpGet(const std::string& resource,
                          std::vector<std::pair<std::string, std::string>> params) {
    HttpResult result;

    bool hasApiKey = false;
    for (const auto& kv : params) {
        if (kv.first == "api_key") {
            hasApiKey = true;
            break;
        }
    }
    if (!hasApiKey) {
        params.emplace_back("api_key", kApiKey);
    }

    std::string path = resource;
    if (path.empty() || path[0] != '/') {
        path = "/" + path;
    }

    const std::string target = path + buildQuery(params);
    result.url = std::string("https://") + kNasaHost + target;

    const std::wstring hostW = toWide(kNasaHost);
    const std::wstring pathW = toWide(target);

    ScopedHInternet session(WinHttpOpen(toWide(kUserAgent).c_str(),
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME,
                                        WINHTTP_NO_PROXY_BYPASS,
                                        0));
    if (!session) {
        throw std::runtime_error("space: WinHttpOpen failed: " + std::to_string(GetLastError()));
    }

    ScopedHInternet connect(WinHttpConnect(session.get(), hostW.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connect) {
        throw std::runtime_error("space: WinHttpConnect failed: " + std::to_string(GetLastError()));
    }

    ScopedHInternet request(WinHttpOpenRequest(connect.get(),
                                               L"GET",
                                               pathW.c_str(),
                                               nullptr,
                                               WINHTTP_NO_REFERER,
                                               WINHTTP_DEFAULT_ACCEPT_TYPES,
                                               WINHTTP_FLAG_SECURE));
    if (!request) {
        throw std::runtime_error("space: WinHttpOpenRequest failed: " + std::to_string(GetLastError()));
    }

    // Keep requests snappy by default.
    WinHttpSetTimeouts(request.get(), 8000, 8000, 8000, 15000);

    if (!WinHttpSendRequest(request.get(),
                            WINHTTP_NO_ADDITIONAL_HEADERS,
                            0,
                            WINHTTP_NO_REQUEST_DATA,
                            0,
                            0,
                            0)) {
        throw std::runtime_error("space: WinHttpSendRequest failed: " + std::to_string(GetLastError()));
    }

    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        throw std::runtime_error("space: WinHttpReceiveResponse failed: " + std::to_string(GetLastError()));
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (WinHttpQueryHeaders(request.get(),
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &status,
                            &statusSize,
                            WINHTTP_NO_HEADER_INDEX)) {
        result.status = static_cast<int>(status);
    }

    result.contentType = readHeaderString(request.get(), WINHTTP_QUERY_CONTENT_TYPE);

    std::string body;
    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            throw std::runtime_error("space: WinHttpQueryDataAvailable failed: " + std::to_string(GetLastError()));
        }
        if (available == 0) {
            break;
        }
        std::vector<char> buffer(available);
        DWORD read = 0;
        if (!WinHttpReadData(request.get(), buffer.data(), available, &read)) {
            throw std::runtime_error("space: WinHttpReadData failed: " + std::to_string(GetLastError()));
        }
        body.append(buffer.data(), static_cast<size_t>(read));
    }

    result.body = std::move(body);
    result.ok = (result.status >= 200 && result.status <= 299);
    return result;
}

namespace json {

class Parser {
public:
    explicit Parser(const std::string& text) : s(text), i(0) {}

    Value parse() {
        skipWs();
        Value out = parseValue();
        skipWs();
        if (i != s.size()) {
            throw std::runtime_error("space.json: trailing characters");
        }
        return out;
    }

private:
    const std::string& s;
    size_t i;

    void skipWs() {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
            ++i;
        }
    }

    bool match(const std::string& token) {
        if (s.compare(i, token.size(), token) == 0) {
            i += token.size();
            return true;
        }
        return false;
    }

    Value parseValue() {
        skipWs();
        if (i >= s.size()) {
            throw std::runtime_error("space.json: unexpected end");
        }
        char c = s[i];
        if (c == '{') {
            return parseObject();
        }
        if (c == '[') {
            return parseArray();
        }
        if (c == '"') {
            return Value::fromString(parseString());
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return Value::fromNumber(parseNumber());
        }
        if (match("true")) {
            return Value::fromBool(true);
        }
        if (match("false")) {
            return Value::fromBool(false);
        }
        if (match("null")) {
            return Value::fromString("null");
        }
        throw std::runtime_error("space.json: invalid token");
    }

    Value parseObject() {
        ++i; // {
        skipWs();
        std::unordered_map<std::string, Value> map;
        if (i < s.size() && s[i] == '}') {
            ++i;
            return Value::fromMap(std::move(map));
        }

        while (true) {
            skipWs();
            if (i >= s.size() || s[i] != '"') {
                throw std::runtime_error("space.json: object key must be string");
            }
            std::string key = parseString();
            skipWs();
            if (i >= s.size() || s[i] != ':') {
                throw std::runtime_error("space.json: expected ':'");
            }
            ++i;
            map[key] = parseValue();
            skipWs();
            if (i >= s.size()) {
                throw std::runtime_error("space.json: unexpected end in object");
            }
            if (s[i] == '}') {
                ++i;
                break;
            }
            if (s[i] != ',') {
                throw std::runtime_error("space.json: expected ','");
            }
            ++i;
        }

        return Value::fromMap(std::move(map));
    }

    Value parseArray() {
        ++i; // [
        skipWs();
        std::vector<Value> list;
        if (i < s.size() && s[i] == ']') {
            ++i;
            return Value::fromList(std::move(list));
        }

        while (true) {
            list.push_back(parseValue());
            skipWs();
            if (i >= s.size()) {
                throw std::runtime_error("space.json: unexpected end in array");
            }
            if (s[i] == ']') {
                ++i;
                break;
            }
            if (s[i] != ',') {
                throw std::runtime_error("space.json: expected ','");
            }
            ++i;
        }

        return Value::fromList(std::move(list));
    }

    std::string parseString() {
        ++i; // opening quote
        std::string out;
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') {
                return out;
            }
            if (c == '\\') {
                if (i >= s.size()) {
                    throw std::runtime_error("space.json: bad escape");
                }
                char e = s[i++];
                switch (e) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        if (i + 4 > s.size()) {
                            throw std::runtime_error("space.json: bad unicode escape");
                        }
                        unsigned int code = 0;
                        for (int d = 0; d < 4; ++d) {
                            char hex = s[i++];
                            code <<= 4;
                            if (hex >= '0' && hex <= '9') {
                                code |= static_cast<unsigned int>(hex - '0');
                            } else if (hex >= 'a' && hex <= 'f') {
                                code |= static_cast<unsigned int>(10 + hex - 'a');
                            } else if (hex >= 'A' && hex <= 'F') {
                                code |= static_cast<unsigned int>(10 + hex - 'A');
                            } else {
                                throw std::runtime_error("space.json: invalid unicode escape");
                            }
                        }
                        if (code <= 0x7f) {
                            out.push_back(static_cast<char>(code));
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("space.json: unsupported escape");
                }
            } else {
                out.push_back(c);
            }
        }
        throw std::runtime_error("space.json: unterminated string");
    }

    double parseNumber() {
        size_t start = i;
        if (s[i] == '-') {
            ++i;
        }
        if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) {
            throw std::runtime_error("space.json: invalid number");
        }
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
            ++i;
        }
        if (i < s.size() && s[i] == '.') {
            ++i;
            if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) {
                throw std::runtime_error("space.json: invalid number");
            }
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
                ++i;
            }
        }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
            ++i;
            if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
                ++i;
            }
            if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) {
                throw std::runtime_error("space.json: invalid exponent");
            }
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
                ++i;
            }
        }
        return std::stod(s.substr(start, i - start));
    }
};

static Value parse(const std::string& text) {
    Parser parser(text);
    return parser.parse();
}

} // namespace json

static Value toValue(const HttpResult& res) {
    std::unordered_map<std::string, Value> out;
    out["ok"] = Value::fromBool(res.ok);
    out["status"] = Value::fromNumber(static_cast<double>(res.status));
    out["url"] = Value::fromString(res.url);
    out["content_type"] = Value::fromString(res.contentType);
    out["body"] = Value::fromString(res.body);
    if (!res.error.empty()) {
        out["error"] = Value::fromString(res.error);
    }

    // Parse JSON opportunistically so callers can index into it easily.
    Value parsed = Value::fromString("null");
    try {
        if (!res.body.empty() && (res.body.front() == '{' || res.body.front() == '[')) {
            parsed = json::parse(res.body);
        }
    } catch (...) {
        parsed = Value::fromString("null");
    }
    out["json"] = std::move(parsed);

    return Value::fromMap(std::move(out));
}

static Value errorValue(const std::string& message) {
    HttpResult res;
    res.ok = false;
    res.status = 0;
    res.error = message;
    return toValue(res);
}

static Value nasaGet(const std::string& resource,
                     const std::vector<std::pair<std::string, std::string>>& params) {
    try {
        HttpResult res = httpGet(resource, params);
        return toValue(res);
    } catch (const std::exception& e) {
        return errorValue(e.what());
    } catch (...) {
        return errorValue("space: unknown error");
    }
}

static std::vector<std::pair<std::string, std::string>> mapToParams(const Value& map) {
    std::vector<std::pair<std::string, std::string>> params;
    if (map.type != ValueType::MAP) {
        return params;
    }
    params.reserve(map.map.size());
    for (const auto& kv : map.map) {
        const Value& v = kv.second;
        if (v.type == ValueType::STRING) {
            params.emplace_back(kv.first, v.str);
        } else if (v.type == ValueType::NUMBER) {
            std::ostringstream oss;
            oss << std::setprecision(17) << v.number;
            params.emplace_back(kv.first, oss.str());
        } else if (v.type == ValueType::BOOL) {
            params.emplace_back(kv.first, v.boolean ? "true" : "false");
        } else {
            // Skip nested objects/arrays: caller should pre-stringify those.
        }
    }
    return params;
}

static Value apod(const std::string& date) {
    std::vector<std::pair<std::string, std::string>> params;
    params.emplace_back("thumbs", "true");
    if (!date.empty()) {
        params.emplace_back("date", date);
    }
    return nasaGet("/planetary/apod", params);
}

static Value neoFeed(const std::string& date) {
    std::string d = date.empty() ? todayDate() : date;
    std::vector<std::pair<std::string, std::string>> params;
    params.emplace_back("start_date", d);
    params.emplace_back("end_date", d);
    return nasaGet("/neo/rest/v1/feed", params);
}

static Value epicNatural(const std::string& date) {
    if (date.empty()) {
        return nasaGet("/EPIC/api/natural", {});
    }
    return nasaGet("/EPIC/api/natural/date/" + date, {});
}

static Value marsRoverPhotos(const std::string& rover, const std::string& date) {
    const std::string useRover = rover.empty() ? "curiosity" : rover;
    const std::string useDate = date.empty() ? todayDate() : date;
    std::vector<std::pair<std::string, std::string>> params;
    params.emplace_back("earth_date", useDate);
    return nasaGet("/mars-photos/api/v1/rovers/" + useRover + "/photos", params);
}

static Value snapshot(const std::string& date) {
    std::unordered_map<std::string, Value> out;
    out["reference_date"] = Value::fromString(date.empty() ? todayDate() : date);
    out["apod"] = apod(date);
    out["neo"] = neoFeed(date);
    out["epic"] = epicNatural(date);
    out["mars_photos"] = marsRoverPhotos("curiosity", date);
    return Value::fromMap(std::move(out));
}

} // namespace

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("space", [](Interpreter& interp) {
        interp.registerModuleFunction("space", "get", [&interp](const std::vector<Value>& args) -> Value {
            if (args.empty() || args.size() > 2) {
                throw std::runtime_error("space.get expects 1 or 2 argument(s): resource, paramsMap?");
            }
            std::string resource = interp.expectString(args[0], "space.get resource must be a string");
            std::vector<std::pair<std::string, std::string>> params;
            if (args.size() == 2) {
                if (args[1].type != ValueType::MAP) {
                    throw std::runtime_error("space.get params must be a map");
                }
                params = mapToParams(args[1]);
            }
            return nasaGet(resource, params);
        });

        interp.registerModuleFunction("space", "apod", [&interp](const std::vector<Value>& args) -> Value {
            if (args.size() > 1) {
                throw std::runtime_error("space.apod expects 0 or 1 argument(s): date?");
            }
            std::string date;
            if (!args.empty()) {
                date = interp.expectString(args[0], "space.apod date must be a string");
            }
            return apod(date);
        });

        interp.registerModuleFunction("space", "neo", [&interp](const std::vector<Value>& args) -> Value {
            if (args.size() > 1) {
                throw std::runtime_error("space.neo expects 0 or 1 argument(s): date?");
            }
            std::string date;
            if (!args.empty()) {
                date = interp.expectString(args[0], "space.neo date must be a string");
            }
            return neoFeed(date);
        });

        interp.registerModuleFunction("space", "epic", [&interp](const std::vector<Value>& args) -> Value {
            if (args.size() > 1) {
                throw std::runtime_error("space.epic expects 0 or 1 argument(s): date?");
            }
            std::string date;
            if (!args.empty()) {
                date = interp.expectString(args[0], "space.epic date must be a string");
            }
            return epicNatural(date);
        });

        interp.registerModuleFunction("space", "mars_photos", [&interp](const std::vector<Value>& args) -> Value {
            if (args.size() > 2) {
                throw std::runtime_error("space.mars_photos expects 0-2 argument(s): rover?, date?");
            }
            std::string rover;
            std::string date;
            if (args.size() >= 1) {
                rover = interp.expectString(args[0], "space.mars_photos rover must be a string");
            }
            if (args.size() == 2) {
                date = interp.expectString(args[1], "space.mars_photos date must be a string");
            }
            return marsRoverPhotos(rover, date);
        });

        interp.registerModuleFunction("space", "snapshot", [&interp](const std::vector<Value>& args) -> Value {
            if (args.size() > 1) {
                throw std::runtime_error("space.snapshot expects 0 or 1 argument(s): date?");
            }
            std::string date;
            if (!args.empty()) {
                date = interp.expectString(args[0], "space.snapshot date must be a string");
            }
            return snapshot(date);
        });
    });
}

} // namespace space_lib
