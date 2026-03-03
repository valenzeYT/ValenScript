#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace zip_lib {

static std::string psEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\'') out += "''";
        else out.push_back(c);
    }
    return out;
}

static int runPs(const std::string& script) {
    std::string cmd = "powershell -NoProfile -NonInteractive -Command \"" + script + "\"";
    return std::system(cmd.c_str());
}

bool compress(const std::string& sourcePath, const std::string& zipPath) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path src(sourcePath);
    fs::path dst(zipPath);
    if (!fs::exists(src, ec)) return false;

    fs::create_directories(dst.parent_path(), ec);
    fs::remove(dst, ec);

    std::string srcEsc = psEscape(fs::absolute(src, ec).string());
    std::string dstEsc = psEscape(fs::absolute(dst, ec).string());
    std::string script =
        "$ErrorActionPreference='Stop';"
        "Compress-Archive -Path '" + srcEsc + "' -DestinationPath '" + dstEsc + "' -Force";
    return runPs(script) == 0;
}

bool extract(const std::string& zipPath, const std::string& outDir) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path src(zipPath);
    fs::path dst(outDir);
    if (!fs::exists(src, ec)) return false;

    fs::create_directories(dst, ec);
    std::string srcEsc = psEscape(fs::absolute(src, ec).string());
    std::string dstEsc = psEscape(fs::absolute(dst, ec).string());
    std::string script =
        "$ErrorActionPreference='Stop';"
        "Expand-Archive -Path '" + srcEsc + "' -DestinationPath '" + dstEsc + "' -Force";
    return runPs(script) == 0;
}

std::string list(const std::string& zipPath) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path src(zipPath);
    if (!fs::exists(src, ec)) return "";

    fs::path tmp = fs::temp_directory_path(ec) / "valen_zip_list.txt";
    std::string srcEsc = psEscape(fs::absolute(src, ec).string());
    std::string tmpEsc = psEscape(fs::absolute(tmp, ec).string());
    std::string script =
        "$ErrorActionPreference='Stop';"
        "Add-Type -AssemblyName System.IO.Compression.FileSystem;"
        "$z=[IO.Compression.ZipFile]::OpenRead('" + srcEsc + "');"
        "try { $z.Entries | ForEach-Object { $_.FullName } | Out-File -Encoding utf8 '" + tmpEsc + "' } "
        "finally { $z.Dispose() }";
    if (runPs(script) != 0) return "";

    std::ifstream f(tmp.string());
    std::stringstream ss;
    ss << f.rdbuf();
    fs::remove(tmp, ec);
    return ss.str();
}

bool exists(const std::string& zipPath) {
    std::error_code ec;
    return std::filesystem::exists(zipPath, ec);
}

} // namespace zip_lib
