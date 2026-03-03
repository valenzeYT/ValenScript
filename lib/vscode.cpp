#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace {
std::string escapeForQuotes(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        if (c == '"') {
            out += "\\\"";
        } else {
            out += c;
        }
    }
    return out;
}

std::string quoteArg(const std::string& arg) {
    return "\"" + escapeForQuotes(arg) + "\"";
}

std::string runCapture(const std::string& command) {
    FILE* pipe = _popen((command + " 2>&1").c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("vscode: failed to start process");
    }

    std::array<char, 512> buffer{};
    std::string output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

    int code = _pclose(pipe);
    if (code != 0) {
        if (output.empty()) {
            throw std::runtime_error("vscode command failed");
        }
        throw std::runtime_error(output);
    }
    return output;
}

void runNoCapture(const std::string& command) {
    int code = std::system((command + " >nul 2>&1").c_str());
    if (code != 0) {
        throw std::runtime_error("vscode command failed: " + command);
    }
}
} // namespace

namespace vscode_lib {

bool available() { return std::system("where code >nul 2>&1") == 0; }

void open(const std::string& path) { runNoCapture("code --reuse-window " + quoteArg(path)); }

void open_file(const std::string& path) { open(path); }

void open_folder(const std::string& path) { runNoCapture("code --reuse-window " + quoteArg(path)); }

void new_window(const std::string& path) { runNoCapture("code --new-window " + quoteArg(path)); }

void go_to(const std::string& filePath, int line, int column) {
    runNoCapture("code --reuse-window --goto " + quoteArg(filePath + ":" + std::to_string(line) + ":" + std::to_string(column)));
}

std::string list_extensions() { return runCapture("code --list-extensions"); }

void install_extension(const std::string& extensionId) {
    runNoCapture("code --install-extension " + quoteArg(extensionId) + " --force");
}

void uninstall_extension(const std::string& extensionId) {
    runNoCapture("code --uninstall-extension " + quoteArg(extensionId));
}

std::string version() { return runCapture("code --version"); }

std::string run_script(const std::string& filePath) {
    return runCapture("c:\\valenscript\\valen.exe init " + quoteArg(filePath));
}

} // namespace vscode_lib
