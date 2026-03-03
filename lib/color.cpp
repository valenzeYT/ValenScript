#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace color_lib {
namespace {
std::string to_hex_component(int value) {
    const int clamped = std::clamp(value, 0, 255);
    std::ostringstream oss;
    oss << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << clamped;
    return oss.str();
}

std::string normalize_hex(std::string input) {
    if (input.empty()) {
        throw std::runtime_error("Color hex cannot be empty");
    }
    if (input.front() == '#') {
        input.erase(0, 1);
    }
    if (input.size() == 3) {
        std::string expanded;
        for (char c : input) {
            expanded.push_back(c);
            expanded.push_back(c);
        }
        input = expanded;
    }
    if (input.size() != 6) {
        throw std::runtime_error("Color hex must be 3 or 6 hexadecimal digits");
    }
    for (char& c : input) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            throw std::runtime_error("Color hex contains invalid character");
        }
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return input;
}

std::array<int, 3> parse_hex(const std::string& hex) {
    const std::string normalized = normalize_hex(hex);
    std::array<int, 3> out{};
    for (int i = 0; i < 3; ++i) {
        std::string component = normalized.substr(i * 2, 2);
        out[i] = std::stoi(component, nullptr, 16);
    }
    return out;
}

std::string build_hex(const std::array<int, 3>& comps) {
    return "#" + to_hex_component(comps[0]) + to_hex_component(comps[1]) + to_hex_component(comps[2]);
}
} // namespace

std::string from_rgb(double r, double g, double b) {
    const std::array<int, 3> comps = {
        static_cast<int>(std::round(r)),
        static_cast<int>(std::round(g)),
        static_cast<int>(std::round(b))
    };
    return build_hex(comps);
}

std::string from_hex(const std::string& hex) {
    return "#" + normalize_hex(hex);
}

std::array<int, 3> to_components(const std::string& hex) {
    return parse_hex(hex);
}

int red(const std::string& hex) {
    return parse_hex(hex)[0];
}

int green(const std::string& hex) {
    return parse_hex(hex)[1];
}

int blue(const std::string& hex) {
    return parse_hex(hex)[2];
}

double brightness(const std::string& hex) {
    const auto comps = parse_hex(hex);
    const double value = 0.299 * comps[0] + 0.587 * comps[1] + 0.114 * comps[2];
    return value / 255.0;
}

std::string invert(const std::string& hex) {
    const auto comps = parse_hex(hex);
    std::array<int, 3> inverted{};
    for (int i = 0; i < 3; ++i) {
        inverted[i] = 255 - comps[i];
    }
    return build_hex(inverted);
}

std::string blend(const std::string& base, const std::string& other, double ratio) {
    const auto a = parse_hex(base);
    const auto b = parse_hex(other);
    const double t = std::clamp(ratio, 0.0, 1.0);
    std::array<int, 3> mixed{};
    for (int i = 0; i < 3; ++i) {
        mixed[i] = static_cast<int>(std::round(a[i] + (b[i] - a[i]) * t));
    }
    return build_hex(mixed);
}

std::string lighten(const std::string& hex, double amount) {
    const auto comps = parse_hex(hex);
    const double t = std::clamp(amount, 0.0, 1.0);
    std::array<int, 3> result{};
    for (int i = 0; i < 3; ++i) {
        result[i] = static_cast<int>(std::round(comps[i] + (255 - comps[i]) * t));
    }
    return build_hex(result);
}

} // namespace color_lib
