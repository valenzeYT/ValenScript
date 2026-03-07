#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <thread>
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

static bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::toupper(static_cast<unsigned char>(a[i])) != std::toupper(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
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

struct ImportIndex {
    std::set<std::string> modules;
    std::map<std::string, std::set<std::string>> exposed_names; // module -> {module, alias?}
};

static void collect_imports_from_file(const fs::path& path, ImportIndex& out) {
    std::istringstream ss(read_text(path));
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // Strip UTF-8 BOM if present at start of a line (common when files are saved as UTF-8 with BOM).
        if (line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3);
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
        std::string module;
        parts >> module;
        if (module.empty()) {
            continue;
        }

        std::string maybe_as;
        std::string alias;
        parts >> maybe_as;
        if (!maybe_as.empty() && (iequals(maybe_as, "AS"))) {
            parts >> alias;
        }

        out.modules.insert(module);
        out.exposed_names[module].insert(module);
        if (!alias.empty()) {
            out.exposed_names[module].insert(alias);
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

static std::vector<fs::path> find_vs_files(const std::vector<fs::path>& explicit_files) {
    std::vector<fs::path> out;
    if (!explicit_files.empty()) {
        for (const auto& p : explicit_files) {
            if (fs::exists(p) && fs::is_regular_file(p)) {
                out.push_back(p);
            }
        }
        return out;
    }

    return out;
}

static std::vector<fs::path> find_all_vs_files() {
    std::vector<fs::path> out;
    for (const auto& entry : fs::recursive_directory_iterator(fs::path("."))) {
        if (entry.is_regular_file() && entry.path().extension() == ".vs") {
            out.push_back(entry.path());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

static std::vector<fs::path> list_cpp_files(const fs::path& dir) {
    std::vector<fs::path> out;
    if (!fs::exists(dir)) {
        return out;
    }
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
            out.push_back(entry.path());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

static std::string signature_for_files(const std::vector<fs::path>& paths) {
    std::string sig;
    std::error_code ec;
    for (const auto& p : paths) {
        const auto s = fs::file_size(p, ec);
        const auto t = fs::last_write_time(p, ec).time_since_epoch().count();
        sig += p.string();
        sig += "|";
        sig += std::to_string(static_cast<unsigned long long>(s));
        sig += "|";
        sig += std::to_string(static_cast<long long>(t));
        sig += "\n";
    }
    return sig;
}

struct Snippet {
    std::string module;
    std::string func;
    std::string exposed_module; // module name in code (module or alias)
    bool is_alias = false;
};

static int generate_snippets(const std::vector<fs::path>& explicit_vs_files, bool workspace_mode, bool include_all) {
    const fs::path lib_dir = "lib";
    const fs::path downloads_dir = "downloads";
    const fs::path out_path = fs::path(".vscode") / "valenscript.code-snippets";

    std::vector<fs::path> vs_files = find_vs_files(explicit_vs_files);
    if (workspace_mode && vs_files.empty()) {
        vs_files = find_all_vs_files();
    }

    ImportIndex imports;
    for (const auto& path : vs_files) {
        collect_imports_from_file(path, imports);
    }

    std::vector<std::pair<std::string, std::string>> module_funcs;
    auto collect_from_dir = [&](const fs::path& dir) {
        for (const auto& p : list_cpp_files(dir)) {
            collect_module_functions(p, module_funcs);
        }
    };
    collect_from_dir(lib_dir);
    collect_from_dir(downloads_dir);

    std::map<std::string, Snippet> snippets; // key -> snippet
    for (const auto& pair : module_funcs) {
        const std::string& module = pair.first;
        const std::string& func = pair.second;

        if (!include_all && imports.modules.find(module) == imports.modules.end()) {
            continue;
        }

        std::set<std::string> exposed;
        auto it = imports.exposed_names.find(module);
        if (it != imports.exposed_names.end()) {
            exposed = it->second;
        } else {
            exposed.insert(module);
        }

        for (const auto& exposed_module : exposed) {
            const bool is_alias = (exposed_module != module);
            const std::string full = exposed_module + "." + func;
            const std::string key = full;

            auto existing = snippets.find(key);
            if (existing == snippets.end()) {
                snippets[key] = Snippet{module, func, exposed_module, is_alias};
                continue;
            }
            // Prefer non-alias over alias for collisions.
            if (existing->second.is_alias && !is_alias) {
                existing->second = Snippet{module, func, exposed_module, is_alias};
            }
        }
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
        const std::string full = item.first;
        out << "  \"" << json_escape(full) << "\": {\n";
        out << "    \"prefix\": \"" << json_escape(full) << "\",\n";
        out << "    \"body\": \"" << json_escape(full) << "\",\n";
        out << "    \"description\": \"ValenScript: " << json_escape(full) << "\"\n";
        out << "  }";
    }
    out << "\n}\n";
    return 0;
}

int main(int argc, char** argv) {
    bool include_all = false;
    bool watch = false;
    bool workspace_mode = false;
    int interval_ms = 250;
    std::vector<fs::path> explicit_vs_files;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--all") {
            include_all = true;
            continue;
        }
        if (arg == "--workspace") {
            workspace_mode = true;
            continue;
        }
        if (arg == "--watch") {
            watch = true;
            continue;
        }
        if (arg == "--interval-ms" && i + 1 < argc) {
            interval_ms = std::max(25, std::atoi(argv[++i]));
            continue;
        }
        fs::path candidate = arg;
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            explicit_vs_files.push_back(candidate);
        }
    }

    if (!watch) {
        return generate_snippets(explicit_vs_files, workspace_mode, include_all);
    }

    if (!workspace_mode && explicit_vs_files.empty()) {
        // Prevent surprising "everything in the repo" behavior. For per-file IntelliSense, pass a file path
        // (VS Code task uses "${file}"). For workspace behavior, pass --workspace.
        std::cerr << "gen_valenscript_snippets: --watch requires a .vs file argument (or pass --workspace)\n";
        return 2;
    }

    std::string last_sig;
    while (true) {
        std::vector<fs::path> vs_files = find_vs_files(explicit_vs_files);
        if (workspace_mode && vs_files.empty()) {
            vs_files = find_all_vs_files();
        }
        std::vector<fs::path> cpp_files = list_cpp_files("lib");
        const auto downloads_cpp = list_cpp_files("downloads");
        cpp_files.insert(cpp_files.end(), downloads_cpp.begin(), downloads_cpp.end());
        std::sort(cpp_files.begin(), cpp_files.end());

        std::string sig = signature_for_files(vs_files) + "----\n" + signature_for_files(cpp_files);
        if (sig != last_sig) {
            last_sig = std::move(sig);
            (void)generate_snippets(explicit_vs_files, workspace_mode, include_all);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}
