#include <filesystem>
#include <sstream>
#include <string>

namespace fs_lib {

std::string list(const std::string& path) {
    std::error_code ec;
    std::ostringstream out;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        if (ec) break;
        out << entry.path().filename().string() << "\n";
    }
    return out.str();
}

bool mkdirs(const std::string& path) {
    std::error_code ec;
    return std::filesystem::create_directories(path, ec) || std::filesystem::exists(path, ec);
}

bool rmdir(const std::string& path) {
    std::error_code ec;
    return std::filesystem::remove_all(path, ec) > 0 && !ec;
}

bool copy(const std::string& from, const std::string& to) {
    std::error_code ec;
    return std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
}

bool move(const std::string& from, const std::string& to) {
    std::error_code ec;
    std::filesystem::rename(from, to, ec);
    return !ec;
}

} // namespace fs_lib
