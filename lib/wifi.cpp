#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace wifi_lib {
namespace {
std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        out.push_back(line);
    }
    return out;
}

std::string run_command(const std::string& command) {
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }

    std::string out;
    char buffer[512];
    while (std::fgets(buffer, static_cast<int>(sizeof(buffer)), pipe) != nullptr) {
        out += buffer;
    }
    _pclose(pipe);
    return out;
}

std::string extract_key_value(const std::vector<std::string>& lines, const std::string& key) {
    const std::string keyLower = to_lower(key);
    for (const std::string& raw : lines) {
        std::string line = trim(raw);
        std::string lower = to_lower(line);
        if (lower.rfind(keyLower, 0) != 0) {
            continue;
        }

        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string keyPart = trim(line.substr(0, colon));
        if (to_lower(keyPart) != keyLower) {
            continue;
        }
        return trim(line.substr(colon + 1));
    }
    return "";
}

std::vector<std::string> extract_all_profile_names(const std::vector<std::string>& lines) {
    std::vector<std::string> names;
    for (const std::string& raw : lines) {
        std::string line = trim(raw);
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string left = to_lower(trim(line.substr(0, colon)));
        if (left != "all user profile") {
            continue;
        }

        std::string value = trim(line.substr(colon + 1));
        if (!value.empty()) {
            names.push_back(value);
        }
    }
    return names;
}

std::vector<std::string> interface_names_vec() {
    std::string out = run_command("netsh wlan show interfaces 2>nul");
    std::vector<std::string> lines = split_lines(out);
    std::vector<std::string> names;
    for (const std::string& raw : lines) {
        std::string line = trim(raw);
        if (to_lower(line).rfind("name", 0) != 0) {
            continue;
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = to_lower(trim(line.substr(0, colon)));
        if (key != "name") {
            continue;
        }
        std::string value = trim(line.substr(colon + 1));
        if (!value.empty()) {
            names.push_back(value);
        }
    }
    return names;
}

std::string escape_cmd_arg(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"') {
            out += "\\\"";
        } else {
            out += c;
        }
    }
    return out;
}
} // namespace

bool available() {
    std::string out = to_lower(run_command("netsh wlan show interfaces 2>nul"));
    if (out.empty()) {
        return false;
    }
    if (out.find("there is no wireless interface") != std::string::npos) {
        return false;
    }
    if (out.find("the wireless autoconfig service") != std::string::npos) {
        return false;
    }
    return true;
}

int interface_count() {
    return static_cast<int>(interface_names_vec().size());
}

std::string interface_names() {
    std::vector<std::string> names = interface_names_vec();
    std::string out;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i) out += "|";
        out += names[i];
    }
    return out;
}

std::string state() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show interfaces 2>nul"));
    return extract_key_value(lines, "State");
}

bool is_connected() {
    return to_lower(state()) == "connected";
}

std::string connected_ssid() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show interfaces 2>nul"));
    return extract_key_value(lines, "SSID");
}

std::string connected_bssid() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show interfaces 2>nul"));
    return extract_key_value(lines, "BSSID");
}

double signal_percent() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show interfaces 2>nul"));
    std::string sig = extract_key_value(lines, "Signal");
    size_t percent = sig.find('%');
    if (percent == std::string::npos) {
        return 0.0;
    }
    std::string num = trim(sig.substr(0, percent));
    if (num.empty()) {
        return 0.0;
    }
    return std::atof(num.c_str());
}

double signal_bars() {
    double pct = signal_percent();
    if (pct <= 0.0) return 0.0;
    if (pct >= 100.0) return 5.0;
    return (pct / 100.0) * 5.0;
}

std::string radio_type() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show interfaces 2>nul"));
    return extract_key_value(lines, "Radio type");
}

std::string authentication() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show interfaces 2>nul"));
    return extract_key_value(lines, "Authentication");
}

std::string cipher() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show interfaces 2>nul"));
    return extract_key_value(lines, "Cipher");
}

std::string profile() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show interfaces 2>nul"));
    return extract_key_value(lines, "Profile");
}

std::string profiles() {
    std::vector<std::string> lines = split_lines(run_command("netsh wlan show profiles 2>nul"));
    std::vector<std::string> names = extract_all_profile_names(lines);
    std::string out;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i) out += "|";
        out += names[i];
    }
    return out;
}

std::string password(const std::string& profileName) {
    if (profileName.empty()) {
        return "";
    }
    std::string safeName = escape_cmd_arg(profileName);
    std::string cmd = "netsh wlan show profile name=\"" + safeName + "\" key=clear 2>nul";
    std::vector<std::string> lines = split_lines(run_command(cmd));
    return extract_key_value(lines, "Key Content");
}

bool connect(const std::string& profileName) {
    std::string cmd = "netsh wlan connect name=\"" + profileName + "\" >nul 2>&1";
    return std::system(cmd.c_str()) == 0;
}

bool disconnect() {
    return std::system("netsh wlan disconnect >nul 2>&1") == 0;
}

double ping_ms(const std::string& host, int timeoutMs) {
    if (timeoutMs < 1) {
        timeoutMs = 1;
    }
    std::string cmd = "ping -n 1 -w " + std::to_string(timeoutMs) + " " + host + " 2>nul";
    std::string out = to_lower(run_command(cmd));

    size_t pos = out.find("time=");
    if (pos != std::string::npos) {
        pos += 5;
        size_t end = out.find("ms", pos);
        if (end != std::string::npos) {
            return std::atof(trim(out.substr(pos, end - pos)).c_str());
        }
    }

    pos = out.find("time<");
    if (pos != std::string::npos) {
        return 1.0;
    }

    return -1.0;
}

bool can_ping(const std::string& host, int timeoutMs) {
    return ping_ms(host, timeoutMs) >= 0.0;
}

} // namespace wifi_lib
