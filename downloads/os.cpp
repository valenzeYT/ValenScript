#include "../include/interpreter.h"
#include <vector>
#include "../include/module_registry.h"
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

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("os", [](Interpreter& interp) {
                    interp.registerModuleFunction("os", "read", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.read");
                        Value out = Value::fromString(os_lib::read(interp.expectString(args[0], "os.read expects string")));
                        interp.fireEvent("os.event.read", {args[0], out});
                        return out;
                    });
                    interp.registerModuleFunction("os", "write", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "os.write");
                        os_lib::write(interp.expectString(args[0], "os.write expects filename string"),
                                      interp.expectString(args[1], "os.write expects content string"));
                        interp.fireEvent("os.event.write", {args[0], args[1]});
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("os", "clear", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.clear");
                        os_lib::clear(interp.expectString(args[0], "os.clear expects string"));
                        interp.fireEvent("os.event.clear", {args[0]});
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("os", "destroy", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.destroy");
                        os_lib::destroy(interp.expectString(args[0], "os.destroy expects string"));
                        interp.fireEvent("os.event.destroy", {args[0]});
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("os", "rename", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "os.rename");
                        os_lib::rename(interp.expectString(args[0], "os.rename expects source string"),
                                       interp.expectString(args[1], "os.rename expects target string"));
                        interp.fireEvent("os.event.rename", {args[0], args[1]});
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("os", "exists", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.exists");
                        Value out = Value::fromBool(os_lib::exists(interp.expectString(args[0], "os.exists expects string")));
                        interp.fireEvent("os.event.exists", {args[0], out});
                        return out;
                    });
                    interp.registerModuleFunction("os", "size", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.size");
                        Value out = Value::fromNumber(static_cast<double>(os_lib::size(interp.expectString(args[0], "os.size expects string"))));
                        interp.fireEvent("os.event.size", {args[0], out});
                        return out;
                    });
                    interp.registerModuleFunction("os", "append", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "os.append");
                        os_lib::append(interp.expectString(args[0], "os.append expects filename string"),
                                       interp.expectString(args[1], "os.append expects content string"));
                        interp.fireEvent("os.event.append", {args[0], args[1]});
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("os", "creation_time", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.creation_time");
                        Value out = Value::fromNumber(static_cast<double>(os_lib::creation_time(interp.expectString(args[0], "os.creation_time expects string"))));
                        interp.fireEvent("os.event.creation_time", {args[0], out});
                        return out;
                    });
                    interp.registerModuleFunction("os", "modified", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.modified");
                        Value out = Value::fromNumber(static_cast<double>(os_lib::modified(interp.expectString(args[0], "os.modified expects string"))));
                        interp.fireEvent("os.event.modified", {args[0], out});
                        return out;
                    });
                    interp.registerModuleFunction("os", "abort", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "os.abort");
                        interp.fireEvent("os.event.abort");
                        os_lib::abort();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("os", "extension", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.extension");
                        Value out = Value::fromString(os_lib::extension(interp.expectString(args[0], "os.extension expects string")));
                        interp.fireEvent("os.event.extension", {args[0], out});
                        return out;
                    });
                    interp.registerModuleFunction("os", "name", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.name");
                        Value out = Value::fromString(os_lib::name(interp.expectString(args[0], "os.name expects string")));
                        interp.fireEvent("os.event.name", {args[0], out});
                        return out;
                    });
                    interp.registerModuleFunction("os", "getenv", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.getenv");
                        Value out = Value::fromString(os_lib::getenv(interp.expectString(args[0], "os.getenv expects string")));
                        interp.fireEvent("os.event.getenv", {args[0], out});
                        return out;
                    });
                    interp.registerModuleFunction("os", "setenv", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "os.setenv");
                        os_lib::setenv(interp.expectString(args[0], "os.setenv expects key string"),
                                       interp.expectString(args[1], "os.setenv expects value string"));
                        interp.fireEvent("os.event.setenv", {args[0], args[1]});
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("os", "wait", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "os.wait");
                        os_lib::wait(interp.expectNumber(args[0], "os.wait expects number"));
                        interp.fireEvent("os.event.wait", {args[0]});
                        return Value::fromNumber(0.0);
                    });

    });
}
