#include "../include/interpreter.h"
#include "../include/module_registry.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace convert_lib {
namespace {

struct ExecResult {
    int exitCode = -1;
    std::string stdoutText;
    std::string stderrText;
    std::string error;
};

static std::string trimNulls(std::string s) {
    while (!s.empty() && s.back() == '\0') {
        s.pop_back();
    }
    return s;
}

static bool toolAvailable(const std::string& toolExeOrName) {
    // `SearchPathA` searches PATH (and app dir) for an executable.
    char buffer[MAX_PATH] = {0};
    DWORD len = SearchPathA(nullptr, toolExeOrName.c_str(), ".exe", MAX_PATH, buffer, nullptr);
    if (len == 0 || len >= MAX_PATH) {
        // If caller passed "ffmpeg.exe" already, try without ".exe" too.
        if (toolExeOrName.size() > 4 && toolExeOrName.substr(toolExeOrName.size() - 4) == ".exe") {
            std::string noExt = toolExeOrName.substr(0, toolExeOrName.size() - 4);
            len = SearchPathA(nullptr, noExt.c_str(), ".exe", MAX_PATH, buffer, nullptr);
            return len > 0 && len < MAX_PATH;
        }
        return false;
    }
    return true;
}

static std::string quoteArg(const std::string& arg) {
    // Minimal Windows command line quoting.
    if (arg.empty()) {
        return "\"\"";
    }
    bool needsQuotes = false;
    for (char c : arg) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\"') {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes) {
        return arg;
    }

    std::string out;
    out.push_back('"');
    size_t backslashes = 0;
    for (char c : arg) {
        if (c == '\\') {
            ++backslashes;
            out.push_back('\\');
            continue;
        }
        if (c == '"') {
            // Escape all backslashes and the quote.
            out.append(backslashes, '\\');
            out.push_back('\\');
            out.push_back('"');
            backslashes = 0;
            continue;
        }
        backslashes = 0;
        out.push_back(c);
    }
    // Escape trailing backslashes before the closing quote.
    out.append(backslashes, '\\');
    out.push_back('"');
    return out;
}

static ExecResult runProcess(const std::string& exe, const std::vector<std::string>& args, DWORD timeoutMs) {
    ExecResult res;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE outRead = nullptr, outWrite = nullptr;
    HANDLE errRead = nullptr, errWrite = nullptr;

    if (!CreatePipe(&outRead, &outWrite, &sa, 0)) {
        res.error = "convert: CreatePipe(stdout) failed: " + std::to_string(GetLastError());
        return res;
    }
    if (!SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0)) {
        res.error = "convert: SetHandleInformation(stdout) failed: " + std::to_string(GetLastError());
        CloseHandle(outRead);
        CloseHandle(outWrite);
        return res;
    }

    if (!CreatePipe(&errRead, &errWrite, &sa, 0)) {
        res.error = "convert: CreatePipe(stderr) failed: " + std::to_string(GetLastError());
        CloseHandle(outRead);
        CloseHandle(outWrite);
        return res;
    }
    if (!SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0)) {
        res.error = "convert: SetHandleInformation(stderr) failed: " + std::to_string(GetLastError());
        CloseHandle(outRead);
        CloseHandle(outWrite);
        CloseHandle(errRead);
        CloseHandle(errWrite);
        return res;
    }

    std::string cmdline = quoteArg(exe);
    for (const auto& a : args) {
        cmdline.push_back(' ');
        cmdline += quoteArg(a);
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = outWrite;
    si.hStdError = errWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};
    // CreateProcess mutates the buffer.
    std::vector<char> cmdBuf(cmdline.begin(), cmdline.end());
    cmdBuf.push_back('\0');

    const BOOL ok = CreateProcessA(nullptr,
                                   cmdBuf.data(),
                                   nullptr,
                                   nullptr,
                                   TRUE,
                                   CREATE_NO_WINDOW,
                                   nullptr,
                                   nullptr,
                                   &si,
                                   &pi);

    CloseHandle(outWrite);
    CloseHandle(errWrite);

    if (!ok) {
        res.error = "convert: CreateProcess failed: " + std::to_string(GetLastError());
        CloseHandle(outRead);
        CloseHandle(errRead);
        return res;
    }

    auto drain = [](HANDLE h) -> std::string {
        std::string out;
        char buf[4096];
        DWORD read = 0;
        while (true) {
            BOOL okRead = ReadFile(h, buf, sizeof(buf), &read, nullptr);
            if (!okRead || read == 0) {
                break;
            }
            out.append(buf, buf + read);
        }
        return out;
    };

    DWORD wait = WaitForSingleObject(pi.hProcess, timeoutMs == 0 ? INFINITE : timeoutMs);
    if (wait == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 124);
        WaitForSingleObject(pi.hProcess, 5000);
        res.error = "convert: process timeout";
    }

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        res.exitCode = -1;
    } else {
        res.exitCode = static_cast<int>(exitCode);
    }

    // Drain output after process exit.
    res.stdoutText = drain(outRead);
    res.stderrText = drain(errRead);

    CloseHandle(outRead);
    CloseHandle(errRead);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return res;
}

static Value toValue(const ExecResult& r, const std::string& tool) {
    std::unordered_map<std::string, Value> out;
    out["ok"] = Value::fromBool(r.error.empty() && r.exitCode == 0);
    out["tool"] = Value::fromString(tool);
    out["exit_code"] = Value::fromNumber(static_cast<double>(r.exitCode));
    out["stdout"] = Value::fromString(r.stdoutText);
    out["stderr"] = Value::fromString(r.stderrText);
    if (!r.error.empty()) {
        out["error"] = Value::fromString(r.error);
    }
    return Value::fromMap(std::move(out));
}

static Value toTextResult(bool ok, const std::string& action, const std::string& inPath, const std::string& outPath, const std::string& err) {
    std::unordered_map<std::string, Value> out;
    out["ok"] = Value::fromBool(ok);
    out["action"] = Value::fromString(action);
    out["in"] = Value::fromString(inPath);
    out["out"] = Value::fromString(outPath);
    if (!err.empty()) {
        out["error"] = Value::fromString(err);
    }
    return Value::fromMap(std::move(out));
}

static bool readAllBytes(const std::string& path, std::string& data, std::string& err) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        err = "Failed to open input: " + path;
        return false;
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    data = oss.str();
    return true;
}

static bool writeAllBytes(const std::string& path, const std::string& data, std::string& err) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        err = "Failed to open output: " + path;
        return false;
    }
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!out) {
        err = "Failed to write output: " + path;
        return false;
    }
    return true;
}

static Value crlfToLf(const std::string& inPath, const std::string& outPath) {
    const std::string action = "text_crlf_to_lf";
    std::string data;
    std::string err;
    if (!readAllBytes(inPath, data, err)) {
        return toTextResult(false, action, inPath, outPath, err);
    }
    std::string out;
    out.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        char c = data[i];
        if (c == '\r') {
            if (i + 1 < data.size() && data[i + 1] == '\n') {
                continue;
            }
            // Lone CR -> drop.
            continue;
        }
        out.push_back(c);
    }
    if (!writeAllBytes(outPath, out, err)) {
        return toTextResult(false, action, inPath, outPath, err);
    }
    return toTextResult(true, action, inPath, outPath, "");
}

static Value lfToCrlf(const std::string& inPath, const std::string& outPath) {
    const std::string action = "text_lf_to_crlf";
    std::string data;
    std::string err;
    if (!readAllBytes(inPath, data, err)) {
        return toTextResult(false, action, inPath, outPath, err);
    }
    std::string out;
    out.reserve(data.size() + data.size() / 16);
    for (size_t i = 0; i < data.size(); ++i) {
        char c = data[i];
        if (c == '\n') {
            if (i > 0 && data[i - 1] == '\r') {
                out.push_back('\n');
            } else {
                out.push_back('\r');
                out.push_back('\n');
            }
        } else {
            out.push_back(c);
        }
    }
    if (!writeAllBytes(outPath, out, err)) {
        return toTextResult(false, action, inPath, outPath, err);
    }
    return toTextResult(true, action, inPath, outPath, "");
}

static Value csvToTsv(const std::string& inPath, const std::string& outPath) {
    // Very simple CSV -> TSV: not RFC4180-complete (no quoted commas handling).
    const std::string action = "csv_to_tsv";
    std::string data;
    std::string err;
    if (!readAllBytes(inPath, data, err)) {
        return toTextResult(false, action, inPath, outPath, err);
    }
    for (char& c : data) {
        if (c == ',') {
            c = '\t';
        }
    }
    if (!writeAllBytes(outPath, data, err)) {
        return toTextResult(false, action, inPath, outPath, err);
    }
    return toTextResult(true, action, inPath, outPath, "");
}

static Value tsvToCsv(const std::string& inPath, const std::string& outPath) {
    const std::string action = "tsv_to_csv";
    std::string data;
    std::string err;
    if (!readAllBytes(inPath, data, err)) {
        return toTextResult(false, action, inPath, outPath, err);
    }
    for (char& c : data) {
        if (c == '\t') {
            c = ',';
        }
    }
    if (!writeAllBytes(outPath, data, err)) {
        return toTextResult(false, action, inPath, outPath, err);
    }
    return toTextResult(true, action, inPath, outPath, "");
}

} // namespace

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("convert", [](Interpreter& interp) {
        interp.registerModuleFunction("convert", "available", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 0, "convert.available");
            std::unordered_map<std::string, Value> out;
            out["ffmpeg"] = Value::fromBool(toolAvailable("ffmpeg"));
            out["magick"] = Value::fromBool(toolAvailable("magick"));
            out["pandoc"] = Value::fromBool(toolAvailable("pandoc"));
            out["7z"] = Value::fromBool(toolAvailable("7z"));
            return Value::fromMap(std::move(out));
        });

        interp.registerModuleFunction("convert", "image", [&interp](const std::vector<Value>& args) -> Value {
            // Uses ImageMagick: `magick input output`
            interp.expectArity(args, 2, "convert.image");
            const std::string inPath = interp.expectString(args[0], "convert.image input must be a string");
            const std::string outPath = interp.expectString(args[1], "convert.image output must be a string");
            if (!toolAvailable("magick")) {
                ExecResult r;
                r.exitCode = 127;
                r.error = "convert.image requires ImageMagick (`magick`) on PATH";
                return toValue(r, "magick");
            }
            ExecResult r = runProcess("magick", {inPath, outPath}, 0);
            return toValue(r, "magick");
        });

        interp.registerModuleFunction("convert", "audio", [&interp](const std::vector<Value>& args) -> Value {
            // Uses ffmpeg: `ffmpeg -y -i input output`
            interp.expectArity(args, 2, "convert.audio");
            const std::string inPath = interp.expectString(args[0], "convert.audio input must be a string");
            const std::string outPath = interp.expectString(args[1], "convert.audio output must be a string");
            if (!toolAvailable("ffmpeg")) {
                ExecResult r;
                r.exitCode = 127;
                r.error = "convert.audio requires ffmpeg on PATH";
                return toValue(r, "ffmpeg");
            }
            ExecResult r = runProcess("ffmpeg", {"-y", "-i", inPath, outPath}, 0);
            return toValue(r, "ffmpeg");
        });

        interp.registerModuleFunction("convert", "video", [&interp](const std::vector<Value>& args) -> Value {
            // Uses ffmpeg: `ffmpeg -y -i input output`
            interp.expectArity(args, 2, "convert.video");
            const std::string inPath = interp.expectString(args[0], "convert.video input must be a string");
            const std::string outPath = interp.expectString(args[1], "convert.video output must be a string");
            if (!toolAvailable("ffmpeg")) {
                ExecResult r;
                r.exitCode = 127;
                r.error = "convert.video requires ffmpeg on PATH";
                return toValue(r, "ffmpeg");
            }
            ExecResult r = runProcess("ffmpeg", {"-y", "-i", inPath, outPath}, 0);
            return toValue(r, "ffmpeg");
        });

        interp.registerModuleFunction("convert", "document", [&interp](const std::vector<Value>& args) -> Value {
            // Uses pandoc: `pandoc input -o output`
            interp.expectArity(args, 2, "convert.document");
            const std::string inPath = interp.expectString(args[0], "convert.document input must be a string");
            const std::string outPath = interp.expectString(args[1], "convert.document output must be a string");
            if (!toolAvailable("pandoc")) {
                ExecResult r;
                r.exitCode = 127;
                r.error = "convert.document requires pandoc on PATH";
                return toValue(r, "pandoc");
            }
            ExecResult r = runProcess("pandoc", {inPath, "-o", outPath}, 0);
            return toValue(r, "pandoc");
        });

        interp.registerModuleFunction("convert", "archive_extract", [&interp](const std::vector<Value>& args) -> Value {
            // Uses 7-Zip: `7z x -y -oOUTDIR ARCHIVE`
            interp.expectArity(args, 2, "convert.archive_extract");
            const std::string archivePath = interp.expectString(args[0], "convert.archive_extract archive must be a string");
            const std::string outDir = interp.expectString(args[1], "convert.archive_extract out_dir must be a string");
            if (!toolAvailable("7z")) {
                ExecResult r;
                r.exitCode = 127;
                r.error = "convert.archive_extract requires 7z on PATH";
                return toValue(r, "7z");
            }
            const std::string outFlag = std::string("-o") + outDir;
            ExecResult r = runProcess("7z", {"x", "-y", outFlag, archivePath}, 0);
            return toValue(r, "7z");
        });

        interp.registerModuleFunction("convert", "text_crlf_to_lf", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 2, "convert.text_crlf_to_lf");
            const std::string inPath = interp.expectString(args[0], "convert.text_crlf_to_lf input must be a string");
            const std::string outPath = interp.expectString(args[1], "convert.text_crlf_to_lf output must be a string");
            return crlfToLf(inPath, outPath);
        });

        interp.registerModuleFunction("convert", "text_lf_to_crlf", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 2, "convert.text_lf_to_crlf");
            const std::string inPath = interp.expectString(args[0], "convert.text_lf_to_crlf input must be a string");
            const std::string outPath = interp.expectString(args[1], "convert.text_lf_to_crlf output must be a string");
            return lfToCrlf(inPath, outPath);
        });

        interp.registerModuleFunction("convert", "csv_to_tsv", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 2, "convert.csv_to_tsv");
            const std::string inPath = interp.expectString(args[0], "convert.csv_to_tsv input must be a string");
            const std::string outPath = interp.expectString(args[1], "convert.csv_to_tsv output must be a string");
            return csvToTsv(inPath, outPath);
        });

        interp.registerModuleFunction("convert", "tsv_to_csv", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 2, "convert.tsv_to_csv");
            const std::string inPath = interp.expectString(args[0], "convert.tsv_to_csv input must be a string");
            const std::string outPath = interp.expectString(args[1], "convert.tsv_to_csv output must be a string");
            return tsvToCsv(inPath, outPath);
        });
    });
}

} // namespace convert_lib

