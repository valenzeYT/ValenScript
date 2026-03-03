#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string trim(const std::string& s) {
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

static std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

static std::string read_text(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return "";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

static void collect_imports_from_file(const fs::path& path, std::set<std::string>& out) {
    std::istringstream ss(read_text(path));
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }
        std::string upper = to_upper(trimmed);
        if (upper.rfind("IMPORT", 0) != 0) {
            continue;
        }
        std::string rest = trim(trimmed.substr(6));
        if (rest.empty()) {
            continue;
        }
        std::istringstream parts(rest);
        std::string name;
        parts >> name;
        if (!name.empty()) {
            out.insert(name);
        }
    }
}

static void collect_module_functions(const fs::path& cpp_path,
                                     std::vector<std::pair<std::string, std::string>>& out) {
    const std::string text = read_text(cpp_path);
    const std::string needle = "registerModuleFunction(\"";
    size_t pos = 0;
    while (true) {
        pos = text.find(needle, pos);
        if (pos == std::string::npos) {
            break;
        }
        size_t mod_start = pos + needle.size();
        size_t mod_end = text.find('"', mod_start);
        if (mod_end == std::string::npos) {
            break;
        }
        std::string module = text.substr(mod_start, mod_end - mod_start);

        size_t comma = text.find(',', mod_end + 1);
        if (comma == std::string::npos) {
            break;
        }
        size_t func_quote = text.find('"', comma);
        if (func_quote == std::string::npos) {
            break;
        }
        size_t func_end = text.find('"', func_quote + 1);
        if (func_end == std::string::npos) {
            break;
        }
        std::string func = text.substr(func_quote + 1, func_end - func_quote - 1);

        out.emplace_back(module, func);
        pos = func_end + 1;
    }
}

int main(int argc, char** argv) {
    const fs::path lib_dir = "lib";
    const fs::path out_path = fs::path(".vscode") / "valenscript.code-snippets";

    std::vector<fs::path> vs_files;
    if (argc > 1) {
        fs::path candidate = argv[1];
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            vs_files.push_back(candidate);
        }
    }
    if (vs_files.empty()) {
        for (const auto& entry : fs::recursive_directory_iterator(fs::path("."))) {
            if (entry.is_regular_file() && entry.path().extension() == ".vs") {
                vs_files.push_back(entry.path());
            }
        }
    }

    std::set<std::string> imported_modules;
    for (const auto& path : vs_files) {
        collect_imports_from_file(path, imported_modules);
    }

    std::vector<std::pair<std::string, std::string>> module_funcs;
    for (const auto& entry : fs::directory_iterator(lib_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
            collect_module_functions(entry.path(), module_funcs);
        }
    }

    std::map<std::string, std::pair<std::string, std::string>> snippets;
    for (const auto& pair : module_funcs) {
        const std::string& module = pair.first;
        const std::string& func = pair.second;
        if (!vs_files.empty() && imported_modules.find(module) == imported_modules.end()) {
            continue;
        }
        std::string key = module + "." + func;
        snippets[key] = {module, func};
    }

    fs::create_directories(out_path.parent_path());
    std::ofstream out(out_path);
    if (!out) {
        return 1;
    }

    out << "{\n";
    bool first = true;
    for (const auto& item : snippets) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        const std::string key = item.first;
        const std::string module = item.second.first;
        const std::string func = item.second.second;
        const std::string full = module + "." + func;
        out << "  \"" << json_escape(key) << "\": {\n";
        out << "    \"prefix\": \"" << json_escape(full) << "\",\n";
        out << "    \"body\": \"" << json_escape(full) << "\",\n";
        out << "    \"description\": \"ValenScript: " << json_escape(full) << "\"\n";
        out << "  }";
    }
    out << "\n}\n";
    return 0;
}
