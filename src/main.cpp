#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/interpreter.h"
#include <iomanip>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace fs = std::filesystem;

static fs::path exeDir() {
    char buf[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return fs::current_path();
    }
    return fs::path(buf).parent_path();
}

static fs::path baseDir() {
    // Keep installs stable even if valen.exe is invoked from another directory.
    // If valen.exe lives in `bin/`, prefer the parent directory (repo-style layout).
    const fs::path exe = exeDir();
    const fs::path parent = exe.parent_path();

    // Heuristic: pick the directory that looks like it contains the runtime layout.
    if (fs::exists(parent / "lib_registry.txt") || fs::exists(parent / "lib") || fs::exists(parent / "downloads")) {
        return parent;
    }
    return exe;
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Cannot open file: " + filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        --end;
    }
    return s.substr(start, end - start);
}

fs::path downloadsDir() {
    return baseDir() / "downloads";
}

fs::path libLogPath() {
    return downloadsDir() / "valen_lib.log";
}

void appendLog(const std::string& message) {
    fs::create_directories(downloadsDir());
    std::ofstream out(libLogPath(), std::ios::app);
    if (!out.is_open()) {
        return;
    }
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &now);
    out << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << " " << message << "\n";
}

bool isValidLibName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    for (char ch : name) {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '_' && ch != '-') {
            return false;
        }
    }
    return true;
}

std::string toClassSuffix(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (char ch : name) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            out.push_back(ch);
        } else {
            out.push_back('_');
        }
    }
    if (!out.empty() && std::isdigit(static_cast<unsigned char>(out[0])) != 0) {
        out.insert(out.begin(), 'L');
    }
    return out;
}

std::string defaultLibUrl(const std::string& name) {
    return "https://raw.githubusercontent.com/valenzeYT/" + name + "/main/" + name + ".cpp";
}

std::optional<std::string> lookupLibUrl(const std::string& name) {
    std::ifstream in((baseDir() / "lib_registry.txt").string());
    if (!in.is_open()) {
        return std::nullopt;
    }
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (key == name && !value.empty()) {
            return value;
        }
    }
    return std::nullopt;
}

bool downloadLib(const std::string& url, const fs::path& outPath) {
    const std::string out = outPath.string();
    std::string cmd = "powershell -NoProfile -Command \"try { ";
    cmd += "Invoke-WebRequest -UseBasicParsing -Uri '";
    cmd += url;
    cmd += "' -OutFile '";
    cmd += out;
    cmd += "'; exit 0 } catch { exit 1 }\"";
    const int code = std::system(cmd.c_str());
    return code == 0 && fs::exists(outPath);
}

bool buildPlugin(const std::string& name, const fs::path& sourcePath) {
    const fs::path outPath = baseDir() / "lib" / (name + ".dll");
    const fs::path importLib = baseDir() / "bin" / "valen.dll.a";

    if (!fs::exists(importLib)) {
        appendLog("missing import lib: " + importLib.string());
        return false;
    }

    const auto start = std::chrono::steady_clock::now();
    std::string cmd = "g++ -std=c++17 -O2 -shared -o \"";
    cmd += outPath.string();
    cmd += "\" \"";
    cmd += sourcePath.string();
    cmd += "\" \"";
    cmd += importLib.string();
    cmd += "\"";
    if (name == "gpu") {
        cmd += " -include cmath -ldxgi";
    }
    if (name == "gui") {
        cmd += " -lgdi32 -luser32 -lshell32 -lole32 -loleaut32 -luxtheme -lcomdlg32 -lcomctl32 -lriched20";
    }
    if (name == "space") {
        cmd += " -lwinhttp";
    }
    if (name == "discord") {
        cmd += " -lwinhttp";
    }
    if (name == "audio") {
        cmd += " -lwinmm";
    }

    appendLog("build plugin: " + cmd);
    const int code = std::system(cmd.c_str());
    const auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    appendLog("build time ms: " + std::to_string(ms));
    std::cout << "Build time: " << ms << " ms\n";
    return code == 0 && fs::exists(outPath);
}

void writeLibTemplate(const fs::path& outPath, const std::string& name) {
    const std::string suffix = toClassSuffix(name);
    std::ofstream out(outPath);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot create lib file: " + outPath.string());
    }
    out << "#include \"../include/module_registry.h\"\n";
    out << "#include \"../include/interpreter.h\"\n\n";
    out << "namespace {\n";
    out << "struct UserLib_" << suffix << "Registrar {\n";
    out << "    UserLib_" << suffix << "Registrar() {\n";
    out << "        module_registry::registerModule(\"" << name << "\", [](Interpreter& interp) {\n";
    out << "            interp.registerModuleFunction(\"" << name << "\", \"hello\",\n";
    out << "                                          [&interp](const std::vector<Value>& args) -> Value {\n";
    out << "                                              interp.expectArity(args, 0, \"" << name << ".hello\");\n";
    out << "                                              return Value::fromString(\"" << name << " says hello\");\n";
    out << "                                          });\n";
    out << "        });\n";
    out << "    }\n";
    out << "};\n\n";
    out << "static UserLib_" << suffix << "Registrar userLib_" << suffix << "Registrar;\n";
    out << "} // namespace\n";
}

void installLib(const std::string& name) {
    if (!isValidLibName(name)) {
        throw std::runtime_error("Invalid lib name. Use letters, numbers, '-' or '_'.");
    }

    const fs::path libDir = baseDir() / "lib";
    const fs::path downloads = downloadsDir();
    fs::create_directories(libDir);
    fs::create_directories(downloads);
    const fs::path downloadPath = downloads / (name + ".cpp");
    const fs::path dllPath = libDir / (name + ".dll");

    if (fs::exists(dllPath)) {
        std::cout << "Lib already installed: " << name << "\n";
        appendLog("lib installed: " + name);
        return;
    }

    if (!fs::exists(downloadPath)) {
        std::string url;
        if (const auto registryUrl = lookupLibUrl(name)) {
            url = *registryUrl;
        } else {
            url = defaultLibUrl(name);
        }

        std::cout << "Downloading " << name << " from " << url << "...\n";
        appendLog("downloading: " + name + " from " + url);
        const bool downloaded = downloadLib(url, downloadPath);
        if (!downloaded) {
            throw std::runtime_error("Download failed for " + name);
        }
        std::cout << "Downloaded to " << downloadPath.string() << "\n";
        appendLog("downloaded: " + name + " to " + downloadPath.string());
    } else {
        std::cout << "Already downloaded: " << name << "\n";
        appendLog("already downloaded: " + name);
    }

    if (!buildPlugin(name, downloadPath)) {
        std::cerr << "Failed to build plugin. Ensure g++ is installed and valen.exe was built with an import lib.\n";
        appendLog("build failed: " + name);
        return;
    }

    // Snippets are intentionally not regenerated here, because VS Code snippets are workspace-global.
    // Use `.vscode` tasks (watch/generate) to drive per-file import IntelliSense.
}

void printUsage() {
    std::cerr << "Usage:\n";
    std::cerr << "  valen init <file>\n";
    std::cerr << "  valen lib <name>\n";
    std::cerr << "  valen lib install <name>\n";
    std::cerr << "  valen lib remove <name>\n";
    std::cerr << "  valen lib uninstall <name>\n";
}

int main(int argc, char* argv[]) {
    std::cout << std::fixed << std::setprecision(17);

    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    try {
        if (command == "init") {
            if (argc < 3) {
                printUsage();
                return 1;
            }
            std::string filename = argv[2];

            std::string input = readFile(filename);

            Lexer lexer(input);
            Parser parser(lexer);
            auto statements = parser.parse();

            Interpreter interpreter;
            interpreter.interpret(statements);

        } else if (command == "lib") {
            if (argc < 3) {
                printUsage();
                return 1;
            }
            std::string subcommand = argv[2];
            std::string name;
            if (subcommand == "install") {
                if (argc < 4) {
                    printUsage();
                    return 1;
                }
                name = argv[3];
                installLib(name);
            } else if (subcommand == "remove" || subcommand == "uninstall") {
                if (argc < 4) {
                    printUsage();
                    return 1;
                }
                name = argv[3];
                if (!isValidLibName(name)) {
                    throw std::runtime_error("Invalid lib name. Use letters, numbers, '-' or '_'.");
                }
                const fs::path dllPath = baseDir() / "lib" / (name + ".dll");
                const fs::path srcPath = downloadsDir() / (name + ".cpp");
                bool removed = false;
                if (fs::exists(dllPath)) {
                    fs::remove(dllPath);
                    removed = true;
                    appendLog("removed dll: " + name);
                }
                if (fs::exists(srcPath)) {
                    fs::remove(srcPath);
                    removed = true;
                    appendLog("removed source: " + name);
                }
                if (removed) {
                    std::cout << "Removed lib: " << name << "\n";
                } else {
                    std::cout << "Lib not found: " << name << "\n";
                }
            } else {
                name = subcommand;
                installLib(name);
            }
        } else if (command == "exec") {
            if (argc < 3) {
                printUsage();
                return 1;
            }
            std::string filename = argv[2];

            std::string input = readFile(filename);

            Lexer lexer(input);
            Parser parser(lexer);
            auto statements = parser.parse();

            Interpreter interpreter;
            interpreter.interpret(statements);

        } else {
            std::cerr << "Unknown command: " << command << "\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
