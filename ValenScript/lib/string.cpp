#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace string_lib {

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

bool starts_with(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

bool ends_with(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string replace_all(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

double length(const std::string& s) { return static_cast<double>(s.size()); }

std::vector<std::string> split(const std::string& s, const std::string& delim) {
    std::vector<std::string> out;
    if (delim.empty()) {
        for (char c : s) out.push_back(std::string(1, c));
        return out;
    }
    size_t start = 0;
    size_t pos = 0;
    while ((pos = s.find(delim, start)) != std::string::npos) {
        out.push_back(s.substr(start, pos - start));
        start = pos + delim.size();
    }
    out.push_back(s.substr(start));
    return out;
}

std::string join(const std::vector<std::string>& items, const std::string& delim) {
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) out += delim;
        out += items[i];
    }
    return out;
}

std::string substring(const std::string& s, int start, int len) {
    if (start < 0) start = 0;
    if (start > static_cast<int>(s.size())) return "";
    if (len < 0) len = 0;
    if (start + len > static_cast<int>(s.size())) len = static_cast<int>(s.size()) - start;
    return s.substr(static_cast<size_t>(start), static_cast<size_t>(len));
}

std::string left(const std::string& s, int n) {
    if (n <= 0) return "";
    if (n >= static_cast<int>(s.size())) return s;
    return s.substr(0, static_cast<size_t>(n));
}

std::string right(const std::string& s, int n) {
    if (n <= 0) return "";
    if (n >= static_cast<int>(s.size())) return s;
    return s.substr(s.size() - static_cast<size_t>(n));
}

std::string repeat(const std::string& s, int count) {
    if (count <= 0) return "";
    std::string out;
    out.reserve(s.size() * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) out += s;
    return out;
}

std::string reverse(std::string s) {
    std::reverse(s.begin(), s.end());
    return s;
}

double index_of(const std::string& s, const std::string& sub) {
    size_t pos = s.find(sub);
    if (pos == std::string::npos) return -1.0;
    return static_cast<double>(pos);
}

double last_index_of(const std::string& s, const std::string& sub) {
    size_t pos = s.rfind(sub);
    if (pos == std::string::npos) return -1.0;
    return static_cast<double>(pos);
}

std::string pad_left(const std::string& s, int width, const std::string& ch) {
    if (width <= static_cast<int>(s.size()) || ch.empty()) return s;
    return repeat(ch, width - static_cast<int>(s.size())) + s;
}

std::string pad_right(const std::string& s, int width, const std::string& ch) {
    if (width <= static_cast<int>(s.size()) || ch.empty()) return s;
    return s + repeat(ch, width - static_cast<int>(s.size()));
}

std::string remove_all(const std::string& s, const std::string& sub) {
    return replace_all(s, sub, "");
}

double count(const std::string& s, const std::string& sub) {
    if (sub.empty()) return 0.0;
    size_t pos = 0;
    int c = 0;
    while ((pos = s.find(sub, pos)) != std::string::npos) {
        ++c;
        pos += sub.size();
    }
    return static_cast<double>(c);
}

std::string capitalize(std::string s) {
    if (s.empty()) return s;
    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    for (size_t i = 1; i < s.size(); ++i) {
        s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
    }
    return s;
}

std::string title(std::string s) {
    bool newWord = true;
    for (char& c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            newWord = true;
        } else if (newWord) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            newWord = false;
        } else {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return s;
}

bool is_alpha(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isalpha(c) != 0; });
}

bool is_digit(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

bool is_alnum(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isalnum(c) != 0; });
}

bool is_space(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) != 0; });
}

std::string char_at(const std::string& s, int idx) {
    if (idx < 0 || idx >= static_cast<int>(s.size())) return "";
    return std::string(1, s[static_cast<size_t>(idx)]);
}

std::string from_char(int code) {
    if (code < 0) code = 0;
    if (code > 255) code = 255;
    return std::string(1, static_cast<char>(code));
}

double to_char(const std::string& s) {
    if (s.empty()) return -1.0;
    return static_cast<double>(static_cast<unsigned char>(s[0]));
}

std::string remove_prefix(const std::string& s, const std::string& prefix) {
    if (starts_with(s, prefix)) return s.substr(prefix.size());
    return s;
}

std::string remove_suffix(const std::string& s, const std::string& suffix) {
    if (!ends_with(s, suffix)) return s;
    return s.substr(0, s.size() - suffix.size());
}

} // namespace string_lib
