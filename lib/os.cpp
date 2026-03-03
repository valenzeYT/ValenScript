#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <thread>
#include <chrono>
#include <random>

using namespace std;

namespace os_lib
{
    void wait(double seconds)
    {
        if (seconds < 0)
            return;
        this_thread::sleep_for(chrono::duration<double>(seconds));
    }

    string read(string filename)
    {
        ifstream file(filename);
        if (!file.is_open())
            throw runtime_error("os.read[] cannot open file: " + filename);

        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void write(string filename, string content)
    {
        ofstream file(filename);
        if (!file.is_open())
            throw runtime_error("os.write[] cannot open file: " + filename);
        file << content;
    }

    void clear(string filename)
    {
        os_lib::write(filename, "");
    }

    void destroy(string filename)
    {
        if (remove(filename.c_str()) != 0)
        {
            throw runtime_error("os.destroy[] cannot destroy file: " + filename);
        }
    }

    void rename(string filename, string name)
    {
        if (std::rename(filename.c_str(), name.c_str()) != 0)
        {
            throw runtime_error("os.rename[] cannot rename file: " + filename);
        }
    }

    bool exists(string filename)
    {
        return filesystem::exists(filename);
    }

    int size(string filename)
    {
        if (!exists(filename))
        {
            throw runtime_error("os.size[] file does not exist: " + filename);
        }

        return filesystem::file_size(filename);
    }

    void append(string filename, string content)
    {
        ofstream file(filename, ios::app);
        if (!file.is_open())
            throw runtime_error("os.append[] cannot open file: " + filename);

        file << content;
    }

    int creation_time(string filename)
    {
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (!GetFileAttributesExA(filename.c_str(), GetFileExInfoStandard, &fileInfo))
        {
            throw runtime_error("Failed to get file info: " + filename);
        }

        FILETIME ft = fileInfo.ftCreationTime;

        SYSTEMTIME stUTC;
        if (!FileTimeToSystemTime(&ft, &stUTC))
        {
            throw runtime_error("Failed to convert FILETIME to SYSTEMTIME");
        }

        tm tmTime = {};
        tmTime.tm_year = stUTC.wYear - 1900;
        tmTime.tm_mon = stUTC.wMonth - 1;
        tmTime.tm_mday = stUTC.wDay;
        tmTime.tm_hour = stUTC.wHour;
        tmTime.tm_min = stUTC.wMinute;
        tmTime.tm_sec = stUTC.wSecond;

        return _mkgmtime(&tmTime);
    }

    int modified(string filename)
    {
        if (!exists(filename))
        {
            throw runtime_error("os.modified[] file does not exist: " + filename);
        }

        auto ftime = filesystem::last_write_time(filename);
        auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
            ftime - filesystem::file_time_type::clock::now() + chrono::system_clock::now());
        int cftime = chrono::system_clock::to_time_t(sctp);
        return cftime;
    }

    void abort()
    {
        std::abort();
    }

    string extension(string filename)
    {
        size_t dotPos = filename.rfind('.');
        if (dotPos == string::npos)
            return "";
        return filename.substr(dotPos + 1);
    }

    string name(string filepath)
    {
        size_t slashPos = filepath.find_last_of("/\\");
        size_t start = (slashPos == std::string::npos) ? 0 : slashPos + 1;

        size_t dotPos = filepath.rfind('.');
        size_t end = (dotPos == std::string::npos || dotPos < start) ? filepath.size() : dotPos;

        return filepath.substr(start, end - start);
    }

    string getenv(string environment)
    {
        const char *val = std::getenv(environment.c_str());
        if (val == nullptr)
        {
            return "";
        }
        return string(val);
    }

    void setenv(string environment, string value)
    {
    _putenv_s(environment.c_str(), value.c_str());
    }
}
