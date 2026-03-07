#include "../include/interpreter.h"
#include "../include/module_registry.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace audio_lib {
namespace {

static Value okMap(const std::string& action) {
    std::unordered_map<std::string, Value> out;
    out["ok"] = Value::fromBool(true);
    out["action"] = Value::fromString(action);
    return Value::fromMap(std::move(out));
}

static Value errMap(const std::string& action, const std::string& error) {
    std::unordered_map<std::string, Value> out;
    out["ok"] = Value::fromBool(false);
    out["action"] = Value::fromString(action);
    out["error"] = Value::fromString(error);
    return Value::fromMap(std::move(out));
}

static Value beep(double freq, double durationMs) {
    const std::string action = "beep";
    const int f = static_cast<int>(freq);
    const int d = static_cast<int>(durationMs);

    if (f < 37 || f > 32767) {
        return errMap(action, "audio.beep freq must be 37..32767");
    }
    if (d < 0) {
        return errMap(action, "audio.beep duration_ms must be >= 0");
    }

    const BOOL ok = Beep(static_cast<DWORD>(f), static_cast<DWORD>(d));
    if (!ok) {
        return errMap(action, "audio.beep failed: " + std::to_string(GetLastError()));
    }
    return okMap(action);
}

static Value stop() {
    const std::string action = "stop";
    const BOOL ok = PlaySoundA(nullptr, nullptr, 0);
    if (!ok) {
        return errMap(action, "audio.stop failed: " + std::to_string(GetLastError()));
    }
    return okMap(action);
}

static Value playWav(const std::string& path, bool async, bool loop) {
    const std::string action = "play_wav";
    if (path.empty()) {
        return errMap(action, "audio.play_wav path must be non-empty");
    }

    DWORD flags = SND_FILENAME | SND_NODEFAULT;
    if (async) {
        flags |= SND_ASYNC;
    } else {
        flags |= SND_SYNC;
    }
    if (loop) {
        flags |= SND_LOOP;
        flags |= SND_ASYNC; // loop must be async
    }

    const BOOL ok = PlaySoundA(path.c_str(), nullptr, flags);
    if (!ok) {
        return errMap(action, "audio.play_wav failed: " + std::to_string(GetLastError()));
    }

    std::unordered_map<std::string, Value> out;
    out["ok"] = Value::fromBool(true);
    out["action"] = Value::fromString(action);
    out["path"] = Value::fromString(path);
    out["async"] = Value::fromBool(async || loop);
    out["loop"] = Value::fromBool(loop);
    return Value::fromMap(std::move(out));
}

static Value messageBeep(int type) {
    const std::string action = "message_beep";

    UINT beepType = MB_OK;
    switch (type) {
        case 0: beepType = MB_OK; break;
        case 1: beepType = MB_ICONHAND; break;
        case 2: beepType = MB_ICONQUESTION; break;
        case 3: beepType = MB_ICONEXCLAMATION; break;
        case 4: beepType = MB_ICONASTERISK; break;
        default: beepType = MB_OK; break;
    }

    const BOOL ok = MessageBeep(beepType);
    if (!ok) {
        return errMap(action, "audio.message_beep failed: " + std::to_string(GetLastError()));
    }

    std::unordered_map<std::string, Value> out;
    out["ok"] = Value::fromBool(true);
    out["action"] = Value::fromString(action);
    out["type"] = Value::fromNumber(static_cast<double>(type));
    return Value::fromMap(std::move(out));
}

} // namespace

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("audio", [](Interpreter& interp) {
        interp.registerModuleFunction("audio", "beep", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 2, "audio.beep");
            const double freq = interp.expectNumber(args[0], "audio.beep freq must be a number");
            const double durationMs = interp.expectNumber(args[1], "audio.beep duration_ms must be a number");
            return beep(freq, durationMs);
        });

        interp.registerModuleFunction("audio", "play_wav", [&interp](const std::vector<Value>& args) -> Value {
            if (args.empty() || args.size() > 3) {
                throw std::runtime_error("audio.play_wav expects 1-3 argument(s): path, async?, loop?");
            }
            const std::string path = interp.expectString(args[0], "audio.play_wav path must be a string");
            bool async = true;
            bool loop = false;
            if (args.size() >= 2) {
                async = interp.isTruthyPublic(args[1]);
            }
            if (args.size() == 3) {
                loop = interp.isTruthyPublic(args[2]);
            }
            return playWav(path, async, loop);
        });

        interp.registerModuleFunction("audio", "stop", [&interp](const std::vector<Value>& args) -> Value {
            interp.expectArity(args, 0, "audio.stop");
            return stop();
        });

        interp.registerModuleFunction("audio", "message_beep", [&interp](const std::vector<Value>& args) -> Value {
            if (args.size() > 1) {
                throw std::runtime_error("audio.message_beep expects 0 or 1 argument(s): type?");
            }
            int type = 0;
            if (args.size() == 1) {
                type = static_cast<int>(interp.expectNumber(args[0], "audio.message_beep type must be a number"));
            }
            return messageBeep(type);
        });
    });
}

} // namespace audio_lib

