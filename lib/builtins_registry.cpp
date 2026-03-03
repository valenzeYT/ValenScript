#include "../include/module_registry.h"
#include "../include/interpreter.h"

namespace {
struct BuiltinsRegistrar {
    BuiltinsRegistrar() {
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
        module_registry::registerModule("input", [](Interpreter& interp) {
            auto add0 = [&interp](const std::string& name, const std::function<void()>& fn) {
                interp.registerModuleFunction("input", name, [&interp, name, fn](const std::vector<Value>& args) -> Value {
                    interp.expectArity(args, 0, "input." + name);
                    fn();
                    interp.fireEvent("input.event." + name);
                    return Value::fromNumber(0.0);
                });
            };
            auto add1Num = [&interp](const std::string& name, const std::function<void(double)>& fn) {
                interp.registerModuleFunction("input", name, [&interp, name, fn](const std::vector<Value>& args) -> Value {
                    interp.expectArity(args, 1, "input." + name);
                    double value = interp.expectNumber(args[0], "input." + name + " expects number");
                    fn(value);
                    interp.fireEvent("input.event." + name, {args[0]});
                    return Value::fromNumber(0.0);
                });
            };
            auto add2Num = [&interp](const std::string& name, const std::function<void(double, double)>& fn) {
                interp.registerModuleFunction("input", name, [&interp, name, fn](const std::vector<Value>& args) -> Value {
                    interp.expectArity(args, 2, "input." + name);
                    double a = interp.expectNumber(args[0], "input." + name + " expects number args");
                    double b = interp.expectNumber(args[1], "input." + name + " expects number args");
                    fn(a, b);
                    interp.fireEvent("input.event." + name, {args[0], args[1]});
                    return Value::fromNumber(0.0);
                });
            };
            auto add1Str = [&interp](const std::string& name, const std::function<void(const std::string&)>& fn) {
                interp.registerModuleFunction("input", name, [&interp, name, fn](const std::vector<Value>& args) -> Value {
                    interp.expectArity(args, 1, "input." + name);
                    std::string text = interp.expectString(args[0], "input." + name + " expects string");
                    fn(text);
                    interp.fireEvent("input.event." + name, {args[0]});
                    return Value::fromNumber(0.0);
                });
            };
            auto add0Num = [&interp](const std::string& name, const std::function<double()>& fn) {
                interp.registerModuleFunction("input", name, [&interp, name, fn](const std::vector<Value>& args) -> Value {
                    interp.expectArity(args, 0, "input." + name);
                    Value out = Value::fromNumber(fn());
                    interp.fireEvent("input.event." + name, {out});
                    return out;
                });
            };

            add0("left_down", []() { input_lib::left_down(); });
            add0("left_up", []() { input_lib::left_up(); });
            add0("left_click", []() { input_lib::left_click(); });
            add0("right_down", []() { input_lib::right_down(); });
            add0("right_up", []() { input_lib::right_up(); });
            add0("right_click", []() { input_lib::right_click(); });
            add0("middle_down", []() { input_lib::middle_down(); });
            add0("middle_up", []() { input_lib::middle_up(); });
            add0("middle_click", []() { input_lib::middle_click(); });

            add2Num("move", [](double x, double y) { input_lib::move(static_cast<int>(x), static_cast<int>(y)); });
            add2Num("move_rel", [](double dx, double dy) { input_lib::move_rel(static_cast<int>(dx), static_cast<int>(dy)); });
            add1Num("scroll", [](double amount) { input_lib::scroll(static_cast<int>(amount)); });
            add1Num("hscroll", [](double amount) { input_lib::hscroll(static_cast<int>(amount)); });
            add0Num("cursor_x", []() { return static_cast<double>(input_lib::cursor_x()); });
            add0Num("cursor_y", []() { return static_cast<double>(input_lib::cursor_y()); });

            add1Num("key_down", [](double code) { input_lib::key_code_down(static_cast<int>(code)); });
            add1Num("key_up", [](double code) { input_lib::key_code_up(static_cast<int>(code)); });
            add1Num("key_press", [](double code) { input_lib::key_code_press(static_cast<int>(code)); });
            add1Str("type", [](const std::string& text) { input_lib::type(text); });

            for (char c = 'a'; c <= 'z'; ++c) {
                std::string base(1, c);
                WORD vk = static_cast<WORD>('A' + (c - 'a'));

                add0(base + "_down", [vk]() { input_lib::key_down(vk); });
                add0(base + "_up", [vk]() { input_lib::key_up(vk); });
                add0(base + "_press", [vk]() { input_lib::key_press(vk); });
            }

            struct NamedVk {
                const char* name;
                WORD vk;
            };
            const NamedVk namedVks[] = {
                {"enter", VK_RETURN}, {"space", VK_SPACE}, {"tab", VK_TAB}, {"esc", VK_ESCAPE},
                {"shift", VK_SHIFT}, {"ctrl", VK_CONTROL}, {"alt", VK_MENU},
                {"lshift", VK_LSHIFT}, {"rshift", VK_RSHIFT},
                {"lctrl", VK_LCONTROL}, {"rctrl", VK_RCONTROL},
                {"lalt", VK_LMENU}, {"ralt", VK_RMENU},
                {"backspace", VK_BACK}, {"del", VK_DELETE}, {"insert", VK_INSERT},
                {"arrow_up", VK_UP}, {"arrow_down", VK_DOWN}, {"arrow_left", VK_LEFT}, {"arrow_right", VK_RIGHT},
                {"home", VK_HOME}, {"end", VK_END}, {"page_up", VK_PRIOR}, {"page_down", VK_NEXT},
                {"caps_lock", VK_CAPITAL}, {"num_lock", VK_NUMLOCK}, {"scroll_lock", VK_SCROLL},
                {"print_screen", VK_SNAPSHOT}, {"pause", VK_PAUSE},
                {"lwin", VK_LWIN}, {"rwin", VK_RWIN}, {"apps", VK_APPS},
                {"digit0", '0'}, {"digit1", '1'}, {"digit2", '2'}, {"digit3", '3'}, {"digit4", '4'},
                {"digit5", '5'}, {"digit6", '6'}, {"digit7", '7'}, {"digit8", '8'}, {"digit9", '9'},
                {"num0", VK_NUMPAD0}, {"num1", VK_NUMPAD1}, {"num2", VK_NUMPAD2}, {"num3", VK_NUMPAD3}, {"num4", VK_NUMPAD4},
                {"num5", VK_NUMPAD5}, {"num6", VK_NUMPAD6}, {"num7", VK_NUMPAD7}, {"num8", VK_NUMPAD8}, {"num9", VK_NUMPAD9},
                {"num_mul", VK_MULTIPLY}, {"num_add", VK_ADD}, {"num_sub", VK_SUBTRACT}, {"num_dec", VK_DECIMAL}, {"num_div", VK_DIVIDE},
                {"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f4", VK_F4}, {"f5", VK_F5}, {"f6", VK_F6},
                {"f7", VK_F7}, {"f8", VK_F8}, {"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12},
                {"f13", VK_F13}, {"f14", VK_F14}, {"f15", VK_F15}, {"f16", VK_F16}, {"f17", VK_F17}, {"f18", VK_F18},
                {"f19", VK_F19}, {"f20", VK_F20}, {"f21", VK_F21}, {"f22", VK_F22}, {"f23", VK_F23}, {"f24", VK_F24},
                {"semicolon", VK_OEM_1}, {"plus", VK_OEM_PLUS}, {"comma", VK_OEM_COMMA}, {"minus", VK_OEM_MINUS},
                {"period", VK_OEM_PERIOD}, {"slash", VK_OEM_2}, {"backtick", VK_OEM_3},
                {"lbracket", VK_OEM_4}, {"backslash", VK_OEM_5}, {"rbracket", VK_OEM_6}, {"quote", VK_OEM_7}
            };
            for (const auto& key : namedVks) {
                std::string base = key.name;
                add0(base + "_down", [vk = key.vk]() { input_lib::key_down(vk); });
                add0(base + "_up", [vk = key.vk]() { input_lib::key_up(vk); });
                add0(base + "_press", [vk = key.vk]() { input_lib::key_press(vk); });
            }
        });
        module_registry::registerModule("clipboard", [](Interpreter& interp) {
            interp.registerModuleFunction("clipboard", "get", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "clipboard.get");
                return Value::fromString(clipboard_lib::get());
            });
            interp.registerModuleFunction("clipboard", "set", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "clipboard.set");
                clipboard_lib::set(interp.expectString(args[0], "clipboard.set expects string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("clipboard", "clear", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "clipboard.clear");
                clipboard_lib::clear();
                return Value::fromNumber(0.0);
            });
        });
        module_registry::registerModule("random", [](Interpreter& interp) {
            interp.registerModuleFunction("random", "seed", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "random.seed");
                random_lib::seed(static_cast<std::uint64_t>(interp.expectNumber(args[0], "random.seed expects number")));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("random", "int", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "random.int");
                int minVal = static_cast<int>(interp.expectNumber(args[0], "random.int expects number args"));
                int maxVal = static_cast<int>(interp.expectNumber(args[1], "random.int expects number args"));
                return Value::fromNumber(static_cast<double>(random_lib::randint(minVal, maxVal)));
            });
            interp.registerModuleFunction("random", "float", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "random.float");
                double minVal = interp.expectNumber(args[0], "random.float expects number args");
                double maxVal = interp.expectNumber(args[1], "random.float expects number args");
                return Value::fromNumber(random_lib::randfloat(minVal, maxVal));
            });
            interp.registerModuleFunction("random", "choice", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "random.choice");
                if (args[0].type != ValueType::LIST) {
                    throw std::runtime_error("random.choice expects list");
                }
                int idx = random_lib::randindex(static_cast<int>(args[0].list.size()));
                return args[0].list[static_cast<size_t>(idx)];
            });
        });
        module_registry::registerModule("math", [](Interpreter& interp) {
            interp.registerModuleFunction("math", "factorial", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.factorial");
                return Value::fromNumber(math_lib::factorial(interp.expectNumber(args[0], "math.factorial expects number")));
            });
            interp.registerModuleFunction("math", "abs", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.abs");
                return Value::fromNumber(math_lib::abs_val(interp.expectNumber(args[0], "math.abs expects number")));
            });
            interp.registerModuleFunction("math", "sqrt", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.sqrt");
                return Value::fromNumber(math_lib::sqrt_val(interp.expectNumber(args[0], "math.sqrt expects number")));
            });
            interp.registerModuleFunction("math", "pow", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "math.pow");
                return Value::fromNumber(math_lib::pow_val(
                    interp.expectNumber(args[0], "math.pow expects number args"),
                    interp.expectNumber(args[1], "math.pow expects number args")));
            });
            interp.registerModuleFunction("math", "floor", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.floor");
                return Value::fromNumber(math_lib::floor_val(interp.expectNumber(args[0], "math.floor expects number")));
            });
            interp.registerModuleFunction("math", "ceil", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.ceil");
                return Value::fromNumber(math_lib::ceil_val(interp.expectNumber(args[0], "math.ceil expects number")));
            });
            interp.registerModuleFunction("math", "round", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.round");
                return Value::fromNumber(math_lib::round_val(interp.expectNumber(args[0], "math.round expects number")));
            });
            interp.registerModuleFunction("math", "mod", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "math.mod");
                return Value::fromNumber(math_lib::mod_val(
                    interp.expectNumber(args[0], "math.mod expects number args"),
                    interp.expectNumber(args[1], "math.mod expects number args")));
            });
            interp.registerModuleFunction("math", "is_even", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.is_even");
                return Value::fromBool(math_lib::is_even_val(interp.expectNumber(args[0], "math.is_even expects number")));
            });
            interp.registerModuleFunction("math", "is_odd", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.is_odd");
                return Value::fromBool(math_lib::is_odd_val(interp.expectNumber(args[0], "math.is_odd expects number")));
            });
            interp.registerModuleFunction("math", "min", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "math.min");
                return Value::fromNumber(math_lib::min_val(
                    interp.expectNumber(args[0], "math.min expects numbers"),
                    interp.expectNumber(args[1], "math.min expects numbers")));
            });
            interp.registerModuleFunction("math", "max", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "math.max");
                return Value::fromNumber(math_lib::max_val(
                    interp.expectNumber(args[0], "math.max expects numbers"),
                    interp.expectNumber(args[1], "math.max expects numbers")));
            });
            interp.registerModuleFunction("math", "clamp", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "math.clamp");
                return Value::fromNumber(math_lib::clamp_val(
                    interp.expectNumber(args[0], "math.clamp expects numbers"),
                    interp.expectNumber(args[1], "math.clamp expects numbers"),
                    interp.expectNumber(args[2], "math.clamp expects numbers")));
            });
            interp.registerModuleFunction("math", "lerp", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "math.lerp");
                return Value::fromNumber(math_lib::lerp_val(
                    interp.expectNumber(args[0], "math.lerp expects numbers"),
                    interp.expectNumber(args[1], "math.lerp expects numbers"),
                    interp.expectNumber(args[2], "math.lerp expects numbers")));
            });
            interp.registerModuleFunction("math", "sin", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.sin");
                return Value::fromNumber(math_lib::sin_val(interp.expectNumber(args[0], "math.sin expects number")));
            });
            interp.registerModuleFunction("math", "cos", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.cos");
                return Value::fromNumber(math_lib::cos_val(interp.expectNumber(args[0], "math.cos expects number")));
            });
            interp.registerModuleFunction("math", "tan", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.tan");
                return Value::fromNumber(math_lib::tan_val(interp.expectNumber(args[0], "math.tan expects number")));
            });
            interp.registerModuleFunction("math", "asin", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.asin");
                return Value::fromNumber(math_lib::asin_val(interp.expectNumber(args[0], "math.asin expects number")));
            });
            interp.registerModuleFunction("math", "acos", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.acos");
                return Value::fromNumber(math_lib::acos_val(interp.expectNumber(args[0], "math.acos expects number")));
            });
            interp.registerModuleFunction("math", "atan", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.atan");
                return Value::fromNumber(math_lib::atan_val(interp.expectNumber(args[0], "math.atan expects number")));
            });
            interp.registerModuleFunction("math", "atan2", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "math.atan2");
                return Value::fromNumber(math_lib::atan2_val(
                    interp.expectNumber(args[0], "math.atan2 expects numbers"),
                    interp.expectNumber(args[1], "math.atan2 expects numbers")));
            });
            interp.registerModuleFunction("math", "log", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.log");
                return Value::fromNumber(math_lib::log_val(interp.expectNumber(args[0], "math.log expects number")));
            });
            interp.registerModuleFunction("math", "log10", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.log10");
                return Value::fromNumber(math_lib::log10_val(interp.expectNumber(args[0], "math.log10 expects number")));
            });
            interp.registerModuleFunction("math", "exp", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.exp");
                return Value::fromNumber(math_lib::exp_val(interp.expectNumber(args[0], "math.exp expects number")));
            });
            interp.registerModuleFunction("math", "cbrt", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.cbrt");
                return Value::fromNumber(math_lib::cbrt_val(interp.expectNumber(args[0], "math.cbrt expects number")));
            });
            interp.registerModuleFunction("math", "hypot", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "math.hypot");
                return Value::fromNumber(math_lib::hypot_val(
                    interp.expectNumber(args[0], "math.hypot expects numbers"),
                    interp.expectNumber(args[1], "math.hypot expects numbers")));
            });
            interp.registerModuleFunction("math", "sign", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.sign");
                return Value::fromNumber(math_lib::sign_val(interp.expectNumber(args[0], "math.sign expects number")));
            });
            interp.registerModuleFunction("math", "trunc", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.trunc");
                return Value::fromNumber(math_lib::trunc_val(interp.expectNumber(args[0], "math.trunc expects number")));
            });
            interp.registerModuleFunction("math", "frac", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.frac");
                return Value::fromNumber(math_lib::frac_val(interp.expectNumber(args[0], "math.frac expects number")));
            });
            interp.registerModuleFunction("math", "gcd", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "math.gcd");
                return Value::fromNumber(math_lib::gcd_val(
                    interp.expectNumber(args[0], "math.gcd expects numbers"),
                    interp.expectNumber(args[1], "math.gcd expects numbers")));
            });
            interp.registerModuleFunction("math", "lcm", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "math.lcm");
                return Value::fromNumber(math_lib::lcm_val(
                    interp.expectNumber(args[0], "math.lcm expects numbers"),
                    interp.expectNumber(args[1], "math.lcm expects numbers")));
            });
            interp.registerModuleFunction("math", "deg2rad", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.deg2rad");
                return Value::fromNumber(math_lib::deg2rad_val(interp.expectNumber(args[0], "math.deg2rad expects number")));
            });
            interp.registerModuleFunction("math", "rad2deg", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "math.rad2deg");
                return Value::fromNumber(math_lib::rad2deg_val(interp.expectNumber(args[0], "math.rad2deg expects number")));
            });
            interp.registerModuleFunction("math", "pi", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "math.pi");
                return Value::fromNumber(math_lib::pi_val());
            });
        });
        module_registry::registerModule("string", [](Interpreter& interp) {
            interp.registerModuleFunction("string", "upper", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.upper");
                return Value::fromString(string_lib::upper(interp.expectString(args[0], "string.upper expects string")));
            });
            interp.registerModuleFunction("string", "lower", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.lower");
                return Value::fromString(string_lib::lower(interp.expectString(args[0], "string.lower expects string")));
            });
            interp.registerModuleFunction("string", "trim", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.trim");
                return Value::fromString(string_lib::trim(interp.expectString(args[0], "string.trim expects string")));
            });
            interp.registerModuleFunction("string", "contains", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.contains");
                return Value::fromBool(string_lib::contains(
                    interp.expectString(args[0], "string.contains expects string"),
                    interp.expectString(args[1], "string.contains expects string")));
            });
            interp.registerModuleFunction("string", "starts_with", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.starts_with");
                return Value::fromBool(string_lib::starts_with(
                    interp.expectString(args[0], "string.starts_with expects string"),
                    interp.expectString(args[1], "string.starts_with expects string")));
            });
            interp.registerModuleFunction("string", "ends_with", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.ends_with");
                return Value::fromBool(string_lib::ends_with(
                    interp.expectString(args[0], "string.ends_with expects string"),
                    interp.expectString(args[1], "string.ends_with expects string")));
            });
            interp.registerModuleFunction("string", "replace", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "string.replace");
                return Value::fromString(string_lib::replace_all(
                    interp.expectString(args[0], "string.replace expects string"),
                    interp.expectString(args[1], "string.replace expects string"),
                    interp.expectString(args[2], "string.replace expects string")));
            });
            interp.registerModuleFunction("string", "length", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.length");
                return Value::fromNumber(string_lib::length(interp.expectString(args[0], "string.length expects string")));
            });
            interp.registerModuleFunction("string", "split", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.split");
                auto parts = string_lib::split(
                    interp.expectString(args[0], "string.split expects string"),
                    interp.expectString(args[1], "string.split expects string"));
                std::vector<Value> out;
                out.reserve(parts.size());
                for (const auto& p : parts) out.push_back(Value::fromString(p));
                return Value::fromList(std::move(out));
            });
            interp.registerModuleFunction("string", "join", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.join");
                if (args[0].type != ValueType::LIST) {
                    throw std::runtime_error("string.join expects list as first arg");
                }
                std::vector<std::string> items;
                items.reserve(args[0].list.size());
                for (const auto& v : args[0].list) items.push_back(interp.valueToStringPublic(v));
                return Value::fromString(string_lib::join(items, interp.expectString(args[1], "string.join expects separator string")));
            });
            interp.registerModuleFunction("string", "substring", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "string.substring");
                return Value::fromString(string_lib::substring(
                    interp.expectString(args[0], "string.substring expects string"),
                    static_cast<int>(interp.expectNumber(args[1], "string.substring expects number")),
                    static_cast<int>(interp.expectNumber(args[2], "string.substring expects number"))));
            });
            interp.registerModuleFunction("string", "left", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.left");
                return Value::fromString(string_lib::left(
                    interp.expectString(args[0], "string.left expects string"),
                    static_cast<int>(interp.expectNumber(args[1], "string.left expects number"))));
            });
            interp.registerModuleFunction("string", "right", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.right");
                return Value::fromString(string_lib::right(
                    interp.expectString(args[0], "string.right expects string"),
                    static_cast<int>(interp.expectNumber(args[1], "string.right expects number"))));
            });
            interp.registerModuleFunction("string", "repeat", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.repeat");
                return Value::fromString(string_lib::repeat(
                    interp.expectString(args[0], "string.repeat expects string"),
                    static_cast<int>(interp.expectNumber(args[1], "string.repeat expects number"))));
            });
            interp.registerModuleFunction("string", "reverse", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.reverse");
                return Value::fromString(string_lib::reverse(interp.expectString(args[0], "string.reverse expects string")));
            });
            interp.registerModuleFunction("string", "index_of", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.index_of");
                return Value::fromNumber(string_lib::index_of(
                    interp.expectString(args[0], "string.index_of expects string"),
                    interp.expectString(args[1], "string.index_of expects string")));
            });
            interp.registerModuleFunction("string", "last_index_of", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.last_index_of");
                return Value::fromNumber(string_lib::last_index_of(
                    interp.expectString(args[0], "string.last_index_of expects string"),
                    interp.expectString(args[1], "string.last_index_of expects string")));
            });
            interp.registerModuleFunction("string", "pad_left", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "string.pad_left");
                return Value::fromString(string_lib::pad_left(
                    interp.expectString(args[0], "string.pad_left expects string"),
                    static_cast<int>(interp.expectNumber(args[1], "string.pad_left expects number")),
                    interp.expectString(args[2], "string.pad_left expects string")));
            });
            interp.registerModuleFunction("string", "pad_right", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "string.pad_right");
                return Value::fromString(string_lib::pad_right(
                    interp.expectString(args[0], "string.pad_right expects string"),
                    static_cast<int>(interp.expectNumber(args[1], "string.pad_right expects number")),
                    interp.expectString(args[2], "string.pad_right expects string")));
            });
            interp.registerModuleFunction("string", "remove", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.remove");
                return Value::fromString(string_lib::remove_all(
                    interp.expectString(args[0], "string.remove expects string"),
                    interp.expectString(args[1], "string.remove expects string")));
            });
            interp.registerModuleFunction("string", "count", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.count");
                return Value::fromNumber(string_lib::count(
                    interp.expectString(args[0], "string.count expects string"),
                    interp.expectString(args[1], "string.count expects string")));
            });
            interp.registerModuleFunction("string", "capitalize", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.capitalize");
                return Value::fromString(string_lib::capitalize(interp.expectString(args[0], "string.capitalize expects string")));
            });
            interp.registerModuleFunction("string", "title", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.title");
                return Value::fromString(string_lib::title(interp.expectString(args[0], "string.title expects string")));
            });
            interp.registerModuleFunction("string", "is_alpha", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.is_alpha");
                return Value::fromBool(string_lib::is_alpha(interp.expectString(args[0], "string.is_alpha expects string")));
            });
            interp.registerModuleFunction("string", "is_digit", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.is_digit");
                return Value::fromBool(string_lib::is_digit(interp.expectString(args[0], "string.is_digit expects string")));
            });
            interp.registerModuleFunction("string", "is_alnum", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.is_alnum");
                return Value::fromBool(string_lib::is_alnum(interp.expectString(args[0], "string.is_alnum expects string")));
            });
            interp.registerModuleFunction("string", "is_space", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.is_space");
                return Value::fromBool(string_lib::is_space(interp.expectString(args[0], "string.is_space expects string")));
            });
            interp.registerModuleFunction("string", "char_at", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.char_at");
                return Value::fromString(string_lib::char_at(
                    interp.expectString(args[0], "string.char_at expects string"),
                    static_cast<int>(interp.expectNumber(args[1], "string.char_at expects number"))));
            });
            interp.registerModuleFunction("string", "from_char", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.from_char");
                return Value::fromString(string_lib::from_char(static_cast<int>(interp.expectNumber(args[0], "string.from_char expects number"))));
            });
            interp.registerModuleFunction("string", "to_char", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "string.to_char");
                return Value::fromNumber(string_lib::to_char(interp.expectString(args[0], "string.to_char expects string")));
            });
            interp.registerModuleFunction("string", "remove_prefix", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.remove_prefix");
                return Value::fromString(string_lib::remove_prefix(
                    interp.expectString(args[0], "string.remove_prefix expects string"),
                    interp.expectString(args[1], "string.remove_prefix expects string")));
            });
            interp.registerModuleFunction("string", "remove_suffix", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "string.remove_suffix");
                return Value::fromString(string_lib::remove_suffix(
                    interp.expectString(args[0], "string.remove_suffix expects string"),
                    interp.expectString(args[1], "string.remove_suffix expects string")));
            });
        });
        module_registry::registerModule("gui", [](Interpreter& interp) {
            interp.registerModuleFunction("gui", "msgbox", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.msgbox");
                gui_lib::msgbox(interp.expectString(args[0], "gui.msgbox expects string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "confirm", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.confirm");
                bool ok = gui_lib::confirm(interp.expectString(args[0], "gui.confirm expects string"));
                return Value::fromBool(ok);
            });
            interp.registerModuleFunction("gui", "prompt", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.prompt");
                return Value::fromString(gui_lib::prompt(interp.expectString(args[0], "gui.prompt expects string")));
            });
            interp.registerModuleFunction("gui", "beep", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.beep");
                gui_lib::beep();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "info", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.info");
                gui_lib::info(interp.expectString(args[0], "gui.info expects string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "warning", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.warning");
                gui_lib::warning(interp.expectString(args[0], "gui.warning expects string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "error", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.error");
                gui_lib::error(interp.expectString(args[0], "gui.error expects string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "yesnocancel", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.yesnocancel");
                return Value::fromNumber(static_cast<double>(gui_lib::yesnocancel(interp.expectString(args[0], "gui.yesnocancel expects string"))));
            });
            interp.registerModuleFunction("gui", "prompt_default", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "gui.prompt_default");
                return Value::fromString(gui_lib::prompt_default(
                    interp.expectString(args[0], "gui.prompt_default expects prompt string"),
                    interp.expectString(args[1], "gui.prompt_default expects default string")));
            });
            interp.registerModuleFunction("gui", "open_file", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.open_file");
                return Value::fromString(gui_lib::open_file());
            });
            interp.registerModuleFunction("gui", "save_file", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.save_file");
                return Value::fromString(gui_lib::save_file());
            });
            interp.registerModuleFunction("gui", "pick_folder", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.pick_folder");
                return Value::fromString(gui_lib::pick_folder());
            });
            interp.registerModuleFunction("gui", "notify", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "gui.notify");
                gui_lib::notify(
                    interp.expectString(args[0], "gui.notify expects title string"),
                    interp.expectString(args[1], "gui.notify expects text string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "set_title", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.set_title");
                gui_lib::set_title(interp.expectString(args[0], "gui.set_title expects string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "get_title", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.get_title");
                return Value::fromString(gui_lib::get_title());
            });
            interp.registerModuleFunction("gui", "show_console", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.show_console");
                gui_lib::show_console();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "hide_console", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.hide_console");
                gui_lib::hide_console();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "minimize_console", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.minimize_console");
                gui_lib::minimize_console();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "maximize_console", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.maximize_console");
                gui_lib::maximize_console();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "restore_console", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.restore_console");
                gui_lib::restore_console();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "set_console_pos", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "gui.set_console_pos");
                gui_lib::set_console_pos(
                    static_cast<int>(interp.expectNumber(args[0], "gui.set_console_pos expects x number")),
                    static_cast<int>(interp.expectNumber(args[1], "gui.set_console_pos expects y number")));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "set_console_size", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "gui.set_console_size");
                gui_lib::set_console_size(
                    static_cast<int>(interp.expectNumber(args[0], "gui.set_console_size expects width number")),
                    static_cast<int>(interp.expectNumber(args[1], "gui.set_console_size expects height number")));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "screen_width", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.screen_width");
                return Value::fromNumber(static_cast<double>(gui_lib::screen_width()));
            });
            interp.registerModuleFunction("gui", "screen_height", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.screen_height");
                return Value::fromNumber(static_cast<double>(gui_lib::screen_height()));
            });
            interp.registerModuleFunction("gui", "topmost_console", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.topmost_console");
                gui_lib::topmost_console(interp.isTruthyPublic(args[0]));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "create_window", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "gui.create_window");
                gui_lib::create_window(
                    interp.expectString(args[0], "gui.create_window expects title string"),
                    static_cast<int>(interp.expectNumber(args[1], "gui.create_window expects width number")),
                    static_cast<int>(interp.expectNumber(args[2], "gui.create_window expects height number")));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "clear_window", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.clear_window");
                gui_lib::clear_window();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "add_text", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 5, "gui.add_text");
                return Value::fromNumber(static_cast<double>(gui_lib::add_text(
                    interp.expectString(args[0], "gui.add_text expects text string"),
                    static_cast<int>(interp.expectNumber(args[1], "gui.add_text expects x number")),
                    static_cast<int>(interp.expectNumber(args[2], "gui.add_text expects y number")),
                    static_cast<int>(interp.expectNumber(args[3], "gui.add_text expects width number")),
                    static_cast<int>(interp.expectNumber(args[4], "gui.add_text expects height number")))));
            });
            interp.registerModuleFunction("gui", "add_section", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 5, "gui.add_section");
                return Value::fromNumber(static_cast<double>(gui_lib::add_section(
                    interp.expectString(args[0], "gui.add_section expects title string"),
                    static_cast<int>(interp.expectNumber(args[1], "gui.add_section expects x number")),
                    static_cast<int>(interp.expectNumber(args[2], "gui.add_section expects y number")),
                    static_cast<int>(interp.expectNumber(args[3], "gui.add_section expects width number")),
                    static_cast<int>(interp.expectNumber(args[4], "gui.add_section expects height number")))));
            });
            interp.registerModuleFunction("gui", "add_button", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 5, "gui.add_button");
                return Value::fromNumber(static_cast<double>(gui_lib::add_button(
                    interp.expectString(args[0], "gui.add_button expects label string"),
                    static_cast<int>(interp.expectNumber(args[1], "gui.add_button expects x number")),
                    static_cast<int>(interp.expectNumber(args[2], "gui.add_button expects y number")),
                    static_cast<int>(interp.expectNumber(args[3], "gui.add_button expects width number")),
                    static_cast<int>(interp.expectNumber(args[4], "gui.add_button expects height number")))));
            });
            interp.registerModuleFunction("gui", "button_clicked", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.button_clicked");
                return Value::fromBool(gui_lib::button_clicked(
                    static_cast<int>(interp.expectNumber(args[0], "gui.button_clicked expects id number"))));
            });
            interp.registerModuleFunction("gui", "add_input", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 5, "gui.add_input");
                return Value::fromNumber(static_cast<double>(gui_lib::add_input(
                    interp.expectString(args[0], "gui.add_input expects placeholder string"),
                    static_cast<int>(interp.expectNumber(args[1], "gui.add_input expects x number")),
                    static_cast<int>(interp.expectNumber(args[2], "gui.add_input expects y number")),
                    static_cast<int>(interp.expectNumber(args[3], "gui.add_input expects width number")),
                    static_cast<int>(interp.expectNumber(args[4], "gui.add_input expects height number")))));
            });
            interp.registerModuleFunction("gui", "add_editor", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 5, "gui.add_editor");
                return Value::fromNumber(static_cast<double>(gui_lib::add_editor(
                    interp.expectString(args[0], "gui.add_editor expects text string"),
                    static_cast<int>(interp.expectNumber(args[1], "gui.add_editor expects x number")),
                    static_cast<int>(interp.expectNumber(args[2], "gui.add_editor expects y number")),
                    static_cast<int>(interp.expectNumber(args[3], "gui.add_editor expects width number")),
                    static_cast<int>(interp.expectNumber(args[4], "gui.add_editor expects height number")))));
            });
            interp.registerModuleFunction("gui", "input_text", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.input_text");
                return Value::fromString(gui_lib::input_text(
                    static_cast<int>(interp.expectNumber(args[0], "gui.input_text expects id number"))));
            });
            interp.registerModuleFunction("gui", "set_input", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "gui.set_input");
                gui_lib::set_input(
                    static_cast<int>(interp.expectNumber(args[0], "gui.set_input expects id number")),
                    interp.expectString(args[1], "gui.set_input expects text string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "add_link", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 6, "gui.add_link");
                return Value::fromNumber(static_cast<double>(gui_lib::add_link(
                    interp.expectString(args[0], "gui.add_link expects label string"),
                    interp.expectString(args[1], "gui.add_link expects url string"),
                    static_cast<int>(interp.expectNumber(args[2], "gui.add_link expects x number")),
                    static_cast<int>(interp.expectNumber(args[3], "gui.add_link expects y number")),
                    static_cast<int>(interp.expectNumber(args[4], "gui.add_link expects width number")),
                    static_cast<int>(interp.expectNumber(args[5], "gui.add_link expects height number")))));
            });
            interp.registerModuleFunction("gui", "set_icon", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.set_icon");
                gui_lib::set_icon(interp.expectString(args[0], "gui.set_icon expects path string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "open_link", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.open_link");
                gui_lib::open_link(interp.expectString(args[0], "gui.open_link expects url string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "close_window", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.close_window");
                gui_lib::close_window();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "window_open", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.window_open");
                return Value::fromBool(gui_lib::window_open());
            });
            interp.registerModuleFunction("gui", "show_window", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.show_window");
                gui_lib::show_window();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "hide_window", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.hide_window");
                gui_lib::hide_window();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "focus_window", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.focus_window");
                gui_lib::focus_window();
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "set_window_title", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.set_window_title");
                gui_lib::set_window_title(interp.expectString(args[0], "gui.set_window_title expects string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "get_window_title", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.get_window_title");
                return Value::fromString(gui_lib::get_window_title());
            });
            interp.registerModuleFunction("gui", "set_window_pos", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "gui.set_window_pos");
                gui_lib::set_window_pos(
                    static_cast<int>(interp.expectNumber(args[0], "gui.set_window_pos expects x number")),
                    static_cast<int>(interp.expectNumber(args[1], "gui.set_window_pos expects y number")));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "set_window_size", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "gui.set_window_size");
                gui_lib::set_window_size(
                    static_cast<int>(interp.expectNumber(args[0], "gui.set_window_size expects width number")),
                    static_cast<int>(interp.expectNumber(args[1], "gui.set_window_size expects height number")));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "window_width", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.window_width");
                return Value::fromNumber(static_cast<double>(gui_lib::window_width()));
            });
            interp.registerModuleFunction("gui", "window_height", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.window_height");
                return Value::fromNumber(static_cast<double>(gui_lib::window_height()));
            });
            interp.registerModuleFunction("gui", "topmost_window", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gui.topmost_window");
                gui_lib::topmost_window(interp.isTruthyPublic(args[0]));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("gui", "media_menu", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gui.media_menu");
                gui_lib::media_menu();
                return Value::fromNumber(0.0);
            });
        });
        module_registry::registerModule("requests", [](Interpreter& interp) {
            interp.registerModuleFunction("requests", "get", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "requests.get");
                std::string out = requests_lib::get(interp.expectString(args[0], "requests.get expects url string"));
                return Value::fromString(out);
            });
            interp.registerModuleFunction("requests", "delete", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "requests.delete");
                std::string out = requests_lib::del(interp.expectString(args[0], "requests.delete expects url string"));
                return Value::fromString(out);
            });
            interp.registerModuleFunction("requests", "post", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "requests.post");
                std::string out = requests_lib::post(
                    interp.expectString(args[0], "requests.post expects url string"),
                    interp.expectString(args[1], "requests.post expects body string"));
                return Value::fromString(out);
            });
            interp.registerModuleFunction("requests", "put", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "requests.put");
                std::string out = requests_lib::put(
                    interp.expectString(args[0], "requests.put expects url string"),
                    interp.expectString(args[1], "requests.put expects body string"));
                return Value::fromString(out);
            });
            interp.registerModuleFunction("requests", "patch", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "requests.patch");
                std::string out = requests_lib::patch(
                    interp.expectString(args[0], "requests.patch expects url string"),
                    interp.expectString(args[1], "requests.patch expects body string"));
                return Value::fromString(out);
            });
            interp.registerModuleFunction("requests", "download", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "requests.download");
                requests_lib::download(
                    interp.expectString(args[0], "requests.download expects url string"),
                    interp.expectString(args[1], "requests.download expects out path string"));
                return Value::fromNumber(0.0);
            });
        });
        module_registry::registerModule("json", [](Interpreter& interp) {
            interp.registerModuleFunction("json", "parse", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "json.parse");
                return json_lib::parse(interp.expectString(args[0], "json.parse expects string"));
            });
            interp.registerModuleFunction("json", "stringify", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "json.stringify");
                return Value::fromString(json_lib::stringify(args[0]));
            });
            interp.registerModuleFunction("json", "valid", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "json.valid");
                return Value::fromBool(json_lib::valid(interp.expectString(args[0], "json.valid expects string")));
            });
            interp.registerModuleFunction("json", "pretty", [&interp](const std::vector<Value>& args) -> Value {
                if (args.empty() || args.size() > 2) {
                    throw std::runtime_error("json.pretty expects 1 or 2 argument(s)");
                }
                int indent = 2;
                if (args.size() == 2) {
                    indent = static_cast<int>(interp.expectNumber(args[1], "json.pretty indent expects number"));
                }
                if (args[0].type == ValueType::STRING) {
                    Value parsed = json_lib::parse(args[0].str);
                    return Value::fromString(json_lib::pretty(parsed, indent));
                }
                return Value::fromString(json_lib::pretty(args[0], indent));
            });
        });
        module_registry::registerModule("zip", [](Interpreter& interp) {
            interp.registerModuleFunction("zip", "compress", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "zip.compress");
                return Value::fromBool(zip_lib::compress(
                    interp.expectString(args[0], "zip.compress expects source path string"),
                    interp.expectString(args[1], "zip.compress expects zip path string")));
            });
            interp.registerModuleFunction("zip", "create", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "zip.create");
                return Value::fromBool(zip_lib::compress(
                    interp.expectString(args[0], "zip.create expects source path string"),
                    interp.expectString(args[1], "zip.create expects zip path string")));
            });
            interp.registerModuleFunction("zip", "extract", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "zip.extract");
                return Value::fromBool(zip_lib::extract(
                    interp.expectString(args[0], "zip.extract expects zip path string"),
                    interp.expectString(args[1], "zip.extract expects output dir string")));
            });
            interp.registerModuleFunction("zip", "list", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "zip.list");
                return Value::fromString(zip_lib::list(
                    interp.expectString(args[0], "zip.list expects zip path string")));
            });
            interp.registerModuleFunction("zip", "exists", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "zip.exists");
                return Value::fromBool(zip_lib::exists(
                    interp.expectString(args[0], "zip.exists expects zip path string")));
            });
        });
        module_registry::registerModule("conversions", [](Interpreter& interp) {
            interp.registerModuleFunction("conversions", "convert", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 4, "conversions.convert");
                return Value::fromNumber(conversions_lib::convert(
                    interp.expectString(args[0], "conversions.convert expects category string"),
                    interp.expectString(args[1], "conversions.convert expects from unit string"),
                    interp.expectString(args[2], "conversions.convert expects to unit string"),
                    interp.expectNumber(args[3], "conversions.convert expects number value")));
            });
            interp.registerModuleFunction("conversions", "temperature", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "conversions.temperature");
                return Value::fromNumber(conversions_lib::temperature(
                    interp.expectString(args[0], "conversions.temperature expects from unit string"),
                    interp.expectString(args[1], "conversions.temperature expects to unit string"),
                    interp.expectNumber(args[2], "conversions.temperature expects number value")));
            });
            interp.registerModuleFunction("conversions", "length", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "conversions.length");
                return Value::fromNumber(conversions_lib::length(
                    interp.expectString(args[0], "conversions.length expects from unit string"),
                    interp.expectString(args[1], "conversions.length expects to unit string"),
                    interp.expectNumber(args[2], "conversions.length expects number value")));
            });
            interp.registerModuleFunction("conversions", "weight", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "conversions.weight");
                return Value::fromNumber(conversions_lib::weight(
                    interp.expectString(args[0], "conversions.weight expects from unit string"),
                    interp.expectString(args[1], "conversions.weight expects to unit string"),
                    interp.expectNumber(args[2], "conversions.weight expects number value")));
            });
            interp.registerModuleFunction("conversions", "time", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "conversions.time");
                return Value::fromNumber(conversions_lib::time(
                    interp.expectString(args[0], "conversions.time expects from unit string"),
                    interp.expectString(args[1], "conversions.time expects to unit string"),
                    interp.expectNumber(args[2], "conversions.time expects number value")));
            });
            interp.registerModuleFunction("conversions", "area", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "conversions.area");
                return Value::fromNumber(conversions_lib::area(
                    interp.expectString(args[0], "conversions.area expects from unit string"),
                    interp.expectString(args[1], "conversions.area expects to unit string"),
                    interp.expectNumber(args[2], "conversions.area expects number value")));
            });
            interp.registerModuleFunction("conversions", "volume", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "conversions.volume");
                return Value::fromNumber(conversions_lib::volume(
                    interp.expectString(args[0], "conversions.volume expects from unit string"),
                    interp.expectString(args[1], "conversions.volume expects to unit string"),
                    interp.expectNumber(args[2], "conversions.volume expects number value")));
            });
            interp.registerModuleFunction("conversions", "speed", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "conversions.speed");
                return Value::fromNumber(conversions_lib::speed(
                    interp.expectString(args[0], "conversions.speed expects from unit string"),
                    interp.expectString(args[1], "conversions.speed expects to unit string"),
                    interp.expectNumber(args[2], "conversions.speed expects number value")));
            });
            interp.registerModuleFunction("conversions", "data", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "conversions.data");
                return Value::fromNumber(conversions_lib::data(
                    interp.expectString(args[0], "conversions.data expects from unit string"),
                    interp.expectString(args[1], "conversions.data expects to unit string"),
                    interp.expectNumber(args[2], "conversions.data expects number value")));
            });
        });
        module_registry::registerModule("project", [](Interpreter& interp) {
            interp.registerModuleFunction("project", "new", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "project.new");
                project_lib::new_project(interp.expectString(args[0], "project.new expects name string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("project", "name", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "project.name");
                return Value::fromString(project_lib::name());
            });
            interp.registerModuleFunction("project", "set", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "project.set");
                project_lib::set_value(
                    interp.expectString(args[0], "project.set expects key string"),
                    interp.expectString(args[1], "project.set expects value string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("project", "get", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "project.get");
                return Value::fromString(project_lib::get_value(interp.expectString(args[0], "project.get expects key string")));
            });
            interp.registerModuleFunction("project", "save", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "project.save");
                project_lib::save(interp.expectString(args[0], "project.save expects path string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("project", "load", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "project.load");
                return Value::fromString(project_lib::load(interp.expectString(args[0], "project.load expects path string")));
            });
        });
        module_registry::registerModule("assets", [](Interpreter& interp) {
            interp.registerModuleFunction("assets", "add", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "assets.add");
                assets_lib::add(interp.expectString(args[0], "assets.add expects path string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("assets", "remove", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "assets.remove");
                assets_lib::remove(interp.expectString(args[0], "assets.remove expects path string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("assets", "has", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "assets.has");
                return Value::fromBool(assets_lib::has(interp.expectString(args[0], "assets.has expects path string")));
            });
            interp.registerModuleFunction("assets", "count", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "assets.count");
                return Value::fromNumber(static_cast<double>(assets_lib::count()));
            });
            interp.registerModuleFunction("assets", "clear", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "assets.clear");
                assets_lib::clear();
                return Value::fromNumber(0.0);
            });
        });
        module_registry::registerModule("ui", [](Interpreter& interp) {
            interp.registerModuleFunction("ui", "set_status", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "ui.set_status");
                ui_lib::set_status(interp.expectString(args[0], "ui.set_status expects text string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("ui", "status", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "ui.status");
                return Value::fromString(ui_lib::status());
            });
            interp.registerModuleFunction("ui", "set_progress", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "ui.set_progress");
                ui_lib::set_progress(interp.expectNumber(args[0], "ui.set_progress expects number"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("ui", "progress", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "ui.progress");
                return Value::fromNumber(ui_lib::progress());
            });
        });
        module_registry::registerModule("undo", [](Interpreter& interp) {
            interp.registerModuleFunction("undo", "push", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "undo.push");
                undo_lib::push(interp.expectString(args[0], "undo.push expects action string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("undo", "undo", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "undo.undo");
                return Value::fromString(undo_lib::undo());
            });
            interp.registerModuleFunction("undo", "redo", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "undo.redo");
                return Value::fromString(undo_lib::redo());
            });
            interp.registerModuleFunction("undo", "can_undo", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "undo.can_undo");
                return Value::fromBool(undo_lib::can_undo());
            });
            interp.registerModuleFunction("undo", "can_redo", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "undo.can_redo");
                return Value::fromBool(undo_lib::can_redo());
            });
            interp.registerModuleFunction("undo", "clear", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "undo.clear");
                undo_lib::clear();
                return Value::fromNumber(0.0);
            });
        });
        module_registry::registerModule("jobs", [](Interpreter& interp) {
            interp.registerModuleFunction("jobs", "push", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "jobs.push");
                jobs_lib::push(interp.expectString(args[0], "jobs.push expects job string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("jobs", "next", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "jobs.next");
                return Value::fromString(jobs_lib::next());
            });
            interp.registerModuleFunction("jobs", "count", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "jobs.count");
                return Value::fromNumber(static_cast<double>(jobs_lib::count()));
            });
            interp.registerModuleFunction("jobs", "clear", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "jobs.clear");
                jobs_lib::clear();
                return Value::fromNumber(0.0);
            });
        });
        module_registry::registerModule("fs", [](Interpreter& interp) {
            interp.registerModuleFunction("fs", "list", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "fs.list");
                return Value::fromString(fs_lib::list(interp.expectString(args[0], "fs.list expects path string")));
            });
            interp.registerModuleFunction("fs", "mkdir", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "fs.mkdir");
                return Value::fromBool(fs_lib::mkdirs(interp.expectString(args[0], "fs.mkdir expects path string")));
            });
            interp.registerModuleFunction("fs", "rmdir", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "fs.rmdir");
                return Value::fromBool(fs_lib::rmdir(interp.expectString(args[0], "fs.rmdir expects path string")));
            });
            interp.registerModuleFunction("fs", "copy", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "fs.copy");
                return Value::fromBool(fs_lib::copy(
                    interp.expectString(args[0], "fs.copy expects source string"),
                    interp.expectString(args[1], "fs.copy expects target string")));
            });
            interp.registerModuleFunction("fs", "move", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "fs.move");
                return Value::fromBool(fs_lib::move(
                    interp.expectString(args[0], "fs.move expects source string"),
                    interp.expectString(args[1], "fs.move expects target string")));
            });
        });
        module_registry::registerModule("perf", [](Interpreter& interp) {
            interp.registerModuleFunction("perf", "start", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "perf.start");
                perf_lib::start(interp.expectString(args[0], "perf.start expects label string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("perf", "stop", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "perf.stop");
                return Value::fromNumber(perf_lib::stop(interp.expectString(args[0], "perf.stop expects label string")));
            });
            interp.registerModuleFunction("perf", "now_ms", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "perf.now_ms");
                return Value::fromNumber(perf_lib::now_ms());
            });
            interp.registerModuleFunction("perf", "sleep_ms", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "perf.sleep_ms");
                perf_lib::sleep_ms(interp.expectNumber(args[0], "perf.sleep_ms expects milliseconds number"));
                return Value::fromNumber(0.0);
            });
        });
        module_registry::registerModule("history", [](Interpreter& interp) {
            interp.registerModuleFunction("history", "push", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "history.push");
                history_lib::push(interp.expectString(args[0], "history.push expects action string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("history", "undo", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "history.undo");
                return Value::fromString(history_lib::undo());
            });
            interp.registerModuleFunction("history", "redo", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "history.redo");
                return Value::fromString(history_lib::redo());
            });
            interp.registerModuleFunction("history", "can_undo", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "history.can_undo");
                return Value::fromBool(history_lib::can_undo());
            });
            interp.registerModuleFunction("history", "can_redo", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "history.can_redo");
                return Value::fromBool(history_lib::can_redo());
            });
            interp.registerModuleFunction("history", "clear", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "history.clear");
                history_lib::clear();
                return Value::fromNumber(0.0);
            });
        });
        module_registry::registerModule("time", [](Interpreter& interp) {
            interp.registerModuleFunction("time", "unix", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "time.unix");
                Value out = Value::fromNumber(static_cast<double>(time_lib::get_unix()));
                interp.fireEvent("time.event.unix", {out});
                return out;
            });
            interp.registerModuleFunction("time", "unix_ms", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "time.unix_ms");
                Value out = Value::fromNumber(static_cast<double>(time_lib::get_unix_ms()));
                interp.fireEvent("time.event.unix_ms", {out});
                return out;
            });
            interp.registerModuleFunction("time", "unix_us", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "time.unix_us");
                Value out = Value::fromNumber(static_cast<double>(time_lib::get_unix_us()));
                interp.fireEvent("time.event.unix_us", {out});
                return out;
            });
        });
        module_registry::registerModule("color", [](Interpreter& interp) {
            interp.registerModuleFunction("color", "from_rgb", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "color.from_rgb");
                return Value::fromString(color_lib::from_rgb(
                    interp.expectNumber(args[0], "color.from_rgb expects red number"),
                    interp.expectNumber(args[1], "color.from_rgb expects green number"),
                    interp.expectNumber(args[2], "color.from_rgb expects blue number")
                ));
            });
            interp.registerModuleFunction("color", "from_hex", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "color.from_hex");
                return Value::fromString(color_lib::from_hex(
                    interp.expectString(args[0], "color.from_hex expects hex string")
                ));
            });
            interp.registerModuleFunction("color", "to_components", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "color.to_components");
                auto comps = color_lib::to_components(interp.expectString(args[0], "color.to_components expects hex string"));
                std::vector<Value> out;
                for (int c : comps) out.push_back(Value::fromNumber(static_cast<double>(c)));
                return Value::fromList(std::move(out));
            });
            interp.registerModuleFunction("color", "red", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "color.red");
                return Value::fromNumber(static_cast<double>(color_lib::red(interp.expectString(args[0], "color.red expects hex string"))));
            });
            interp.registerModuleFunction("color", "green", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "color.green");
                return Value::fromNumber(static_cast<double>(color_lib::green(interp.expectString(args[0], "color.green expects hex string"))));
            });
            interp.registerModuleFunction("color", "blue", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "color.blue");
                return Value::fromNumber(static_cast<double>(color_lib::blue(interp.expectString(args[0], "color.blue expects hex string"))));
            });
            interp.registerModuleFunction("color", "invert", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "color.invert");
                return Value::fromString(color_lib::invert(interp.expectString(args[0], "color.invert expects hex string")));
            });
            interp.registerModuleFunction("color", "brightness", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "color.brightness");
                return Value::fromNumber(color_lib::brightness(interp.expectString(args[0], "color.brightness expects hex string")));
            });
            interp.registerModuleFunction("color", "blend", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 3, "color.blend");
                return Value::fromString(color_lib::blend(
                    interp.expectString(args[0], "color.blend expects base hex string"),
                    interp.expectString(args[1], "color.blend expects target hex string"),
                    interp.expectNumber(args[2], "color.blend expects ratio number")
                ));
            });
            interp.registerModuleFunction("color", "lighten", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 2, "color.lighten");
                return Value::fromString(color_lib::lighten(
                    interp.expectString(args[0], "color.lighten expects hex string"),
                    interp.expectNumber(args[1], "color.lighten expects amount number")
                ));
            });
        });
        module_registry::registerModule("display", [](Interpreter& interp) {
            interp.registerModuleFunction("display", "monitor_count", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "display.monitor_count");
                return Value::fromNumber(static_cast<double>(display_lib::monitor_count()));
            });
            interp.registerModuleFunction("display", "monitor_list", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "display.monitor_list");
                return Value::fromString(display_lib::monitor_list());
            });
            interp.registerModuleFunction("display", "primary_width", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "display.primary_width");
                return Value::fromNumber(static_cast<double>(display_lib::primary_width()));
            });
            interp.registerModuleFunction("display", "primary_height", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "display.primary_height");
                return Value::fromNumber(static_cast<double>(display_lib::primary_height()));
            });
            interp.registerModuleFunction("display", "primary_refresh", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "display.primary_refresh");
                return Value::fromNumber(static_cast<double>(display_lib::primary_refresh_rate()));
            });
            interp.registerModuleFunction("display", "monitor_info", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "display.monitor_info");
                int idx = static_cast<int>(interp.expectNumber(args[0], "display.monitor_info expects index"));
                return Value::fromString(display_lib::monitor_info(idx));
            });
        });
        module_registry::registerModule("net", [](Interpreter& interp) {
            interp.registerModuleFunction("net", "local_ip", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "net.local_ip");
                return Value::fromString(net_lib::local_ip());
            });
            interp.registerModuleFunction("net", "interfaces", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "net.interfaces");
                return Value::fromString(net_lib::interfaces());
            });
            interp.registerModuleFunction("net", "resolve", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "net.resolve");
                return Value::fromString(net_lib::resolve(interp.expectString(args[0], "net.resolve expects host string")));
            });
        });
        module_registry::registerModule("crypto", [](Interpreter& interp) {
            interp.registerModuleFunction("crypto", "sha256", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "crypto.sha256");
                return Value::fromString(crypto_lib::sha256(interp.expectString(args[0], "crypto.sha256 expects string")));
            });
            interp.registerModuleFunction("crypto", "random_bytes", [&interp](const std::vector<Value>& args) -> Value {
                if (args.size() != 1) {
                    throw std::runtime_error("crypto.random_bytes expects 1 argument(s)");
                }
                int count = static_cast<int>(interp.expectNumber(args[0], "crypto.random_bytes expects byte count"));
                if (count < 0) throw std::runtime_error("crypto.random_bytes expects non-negative length");
                return Value::fromString(crypto_lib::random_bytes(static_cast<size_t>(count)));
            });
        });
        module_registry::registerModule("gpu", [](Interpreter& interp) {
            auto parse_index = [&interp](const Value& value, const std::string& name) -> int {
                double raw = interp.expectNumber(value, name + " expects adapter index number");
                if (std::floor(raw) != raw) {
                    throw std::runtime_error(name + " expects integer index");
                }
                return static_cast<int>(raw);
            };
            interp.registerModuleFunction("gpu", "adapter_count", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gpu.adapter_count");
                return Value::fromNumber(static_cast<double>(gpu_lib::adapter_count()));
            });
            interp.registerModuleFunction("gpu", "adapter_names", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gpu.adapter_names");
                return Value::fromString(gpu_lib::adapter_names());
            });
            interp.registerModuleFunction("gpu", "primary_adapter", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "gpu.primary_adapter");
                return Value::fromString(gpu_lib::primary_adapter());
            });
            interp.registerModuleFunction("gpu", "adapter_name", [&interp, parse_index](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gpu.adapter_name");
                int idx = parse_index(args[0], "gpu.adapter_name");
                return Value::fromString(gpu_lib::adapter_name(idx));
            });
            interp.registerModuleFunction("gpu", "adapter_vendor", [&interp, parse_index](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gpu.adapter_vendor");
                int idx = parse_index(args[0], "gpu.adapter_vendor");
                return Value::fromString(gpu_lib::adapter_vendor(idx));
            });
            interp.registerModuleFunction("gpu", "adapter_memory_mb", [&interp, parse_index](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gpu.adapter_memory_mb");
                int idx = parse_index(args[0], "gpu.adapter_memory_mb");
                return Value::fromNumber(gpu_lib::adapter_memory_mb(idx));
            });
            interp.registerModuleFunction("gpu", "adapter_device_id", [&interp, parse_index](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gpu.adapter_device_id");
                int idx = parse_index(args[0], "gpu.adapter_device_id");
                return Value::fromString(gpu_lib::adapter_device_id(idx));
            });
            interp.registerModuleFunction("gpu", "adapter_flags", [&interp, parse_index](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "gpu.adapter_flags");
                int idx = parse_index(args[0], "gpu.adapter_flags");
                return Value::fromString(gpu_lib::adapter_flags(idx));
            });
        });
        module_registry::registerModule("wifi", [](Interpreter& interp) {
            interp.registerModuleFunction("wifi", "available", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.available");
                return Value::fromBool(wifi_lib::available());
            });
            interp.registerModuleFunction("wifi", "interface_count", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.interface_count");
                return Value::fromNumber(static_cast<double>(wifi_lib::interface_count()));
            });
            interp.registerModuleFunction("wifi", "interfaces", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.interfaces");
                return Value::fromString(wifi_lib::interface_names());
            });
            interp.registerModuleFunction("wifi", "state", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.state");
                return Value::fromString(wifi_lib::state());
            });
            interp.registerModuleFunction("wifi", "connected", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.connected");
                return Value::fromBool(wifi_lib::is_connected());
            });
            interp.registerModuleFunction("wifi", "ssid", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.ssid");
                return Value::fromString(wifi_lib::connected_ssid());
            });
            interp.registerModuleFunction("wifi", "bssid", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.bssid");
                return Value::fromString(wifi_lib::connected_bssid());
            });
            interp.registerModuleFunction("wifi", "signal_percent", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.signal_percent");
                return Value::fromNumber(wifi_lib::signal_percent());
            });
            interp.registerModuleFunction("wifi", "signal_bars", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.signal_bars");
                return Value::fromNumber(wifi_lib::signal_bars());
            });
            interp.registerModuleFunction("wifi", "radio", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.radio");
                return Value::fromString(wifi_lib::radio_type());
            });
            interp.registerModuleFunction("wifi", "auth", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.auth");
                return Value::fromString(wifi_lib::authentication());
            });
            interp.registerModuleFunction("wifi", "cipher", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.cipher");
                return Value::fromString(wifi_lib::cipher());
            });
            interp.registerModuleFunction("wifi", "profile", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.profile");
                return Value::fromString(wifi_lib::profile());
            });
            interp.registerModuleFunction("wifi", "profiles", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.profiles");
                return Value::fromString(wifi_lib::profiles());
            });
            interp.registerModuleFunction("wifi", "connect", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "wifi.connect");
                return Value::fromBool(wifi_lib::connect(interp.expectString(args[0], "wifi.connect expects profile string")));
            });
            interp.registerModuleFunction("wifi", "disconnect", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "wifi.disconnect");
                return Value::fromBool(wifi_lib::disconnect());
            });
            interp.registerModuleFunction("wifi", "can_ping", [&interp](const std::vector<Value>& args) -> Value {
                if (args.size() < 1 || args.size() > 2) {
                    throw std::runtime_error("wifi.can_ping expects 1 or 2 argument(s)");
                }
                int timeout = 1000;
                if (args.size() == 2) {
                    timeout = static_cast<int>(interp.expectNumber(args[1], "wifi.can_ping expects timeout milliseconds number"));
                }
                return Value::fromBool(wifi_lib::can_ping(interp.expectString(args[0], "wifi.can_ping expects host string"), timeout));
            });
            interp.registerModuleFunction("wifi", "ping_ms", [&interp](const std::vector<Value>& args) -> Value {
                if (args.size() < 1 || args.size() > 2) {
                    throw std::runtime_error("wifi.ping_ms expects 1 or 2 argument(s)");
                }
                int timeout = 1000;
                if (args.size() == 2) {
                    timeout = static_cast<int>(interp.expectNumber(args[1], "wifi.ping_ms expects timeout milliseconds number"));
                }
                return Value::fromNumber(wifi_lib::ping_ms(interp.expectString(args[0], "wifi.ping_ms expects host string"), timeout));
            });
            interp.registerModuleFunction("wifi", "password", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "wifi.password");
                return Value::fromString(wifi_lib::password(interp.expectString(args[0], "wifi.password expects profile string")));
            });
        });
        module_registry::registerModule("cpu", [](Interpreter& interp) {
            interp.registerModuleFunction("cpu", "logical_cores", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.logical_cores");
                return Value::fromNumber(static_cast<double>(cpu_lib::logical_cores()));
            });
            interp.registerModuleFunction("cpu", "arch", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.arch");
                return Value::fromString(cpu_lib::architecture());
            });
            interp.registerModuleFunction("cpu", "page_size", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.page_size");
                return Value::fromNumber(cpu_lib::page_size());
            });
            interp.registerModuleFunction("cpu", "allocation_granularity", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.allocation_granularity");
                return Value::fromNumber(cpu_lib::allocation_granularity());
            });
            interp.registerModuleFunction("cpu", "pid", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.pid");
                return Value::fromNumber(cpu_lib::process_id());
            });
            interp.registerModuleFunction("cpu", "thread_count", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.thread_count");
                return Value::fromNumber(cpu_lib::process_thread_count());
            });
            interp.registerModuleFunction("cpu", "usage_percent", [&interp](const std::vector<Value>& args) -> Value {
                if (args.size() > 1) {
                    throw std::runtime_error("cpu.usage_percent expects 0 or 1 argument(s)");
                }
                int sampleMs = 250;
                if (args.size() == 1) {
                    sampleMs = static_cast<int>(interp.expectNumber(args[0], "cpu.usage_percent expects sample milliseconds number"));
                }
                return Value::fromNumber(cpu_lib::usage_percent(sampleMs));
            });
            interp.registerModuleFunction("cpu", "process_cpu_ms", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.process_cpu_ms");
                return Value::fromNumber(cpu_lib::process_cpu_time_ms());
            });
            interp.registerModuleFunction("cpu", "vendor", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.vendor");
                return Value::fromString(cpu_lib::vendor());
            });
            interp.registerModuleFunction("cpu", "brand", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.brand");
                return Value::fromString(cpu_lib::brand());
            });
            interp.registerModuleFunction("cpu", "base_mhz", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "cpu.base_mhz");
                return Value::fromNumber(cpu_lib::base_mhz());
            });
        });
        module_registry::registerModule("memory", [](Interpreter& interp) {
            interp.registerModuleFunction("memory", "load_percent", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.load_percent");
                return Value::fromNumber(memory_lib::load_percent());
            });
            interp.registerModuleFunction("memory", "total_physical_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.total_physical_bytes");
                return Value::fromNumber(memory_lib::total_physical_bytes());
            });
            interp.registerModuleFunction("memory", "available_physical_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.available_physical_bytes");
                return Value::fromNumber(memory_lib::available_physical_bytes());
            });
            interp.registerModuleFunction("memory", "used_physical_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.used_physical_bytes");
                return Value::fromNumber(memory_lib::used_physical_bytes());
            });
            interp.registerModuleFunction("memory", "total_virtual_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.total_virtual_bytes");
                return Value::fromNumber(memory_lib::total_virtual_bytes());
            });
            interp.registerModuleFunction("memory", "available_virtual_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.available_virtual_bytes");
                return Value::fromNumber(memory_lib::available_virtual_bytes());
            });
            interp.registerModuleFunction("memory", "used_virtual_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.used_virtual_bytes");
                return Value::fromNumber(memory_lib::used_virtual_bytes());
            });
            interp.registerModuleFunction("memory", "total_page_file_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.total_page_file_bytes");
                return Value::fromNumber(memory_lib::total_page_file_bytes());
            });
            interp.registerModuleFunction("memory", "available_page_file_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.available_page_file_bytes");
                return Value::fromNumber(memory_lib::available_page_file_bytes());
            });
            interp.registerModuleFunction("memory", "used_page_file_bytes", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "memory.used_page_file_bytes");
                return Value::fromNumber(memory_lib::used_page_file_bytes());
            });
        });
        module_registry::registerModule("system", [](Interpreter& interp) {
            interp.registerModuleFunction("system", "computer_name", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.computer_name");
                return Value::fromString(system_lib::computer_name());
            });
            interp.registerModuleFunction("system", "user_name", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.user_name");
                return Value::fromString(system_lib::user_name());
            });
            interp.registerModuleFunction("system", "os_name", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.os_name");
                return Value::fromString(system_lib::os_name());
            });
            interp.registerModuleFunction("system", "os_display_version", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.os_display_version");
                return Value::fromString(system_lib::os_display_version());
            });
            interp.registerModuleFunction("system", "os_build_number", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.os_build_number");
                return Value::fromString(system_lib::os_build_number());
            });
            interp.registerModuleFunction("system", "is_64bit", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.is_64bit");
                return Value::fromBool(system_lib::is_64bit());
            });
            interp.registerModuleFunction("system", "uptime_seconds", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.uptime_seconds");
                return Value::fromNumber(system_lib::uptime_seconds());
            });
            interp.registerModuleFunction("system", "temp_dir", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.temp_dir");
                return Value::fromString(system_lib::temp_dir());
            });
            interp.registerModuleFunction("system", "current_dir", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.current_dir");
                return Value::fromString(system_lib::current_dir());
            });
            interp.registerModuleFunction("system", "windows_dir", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.windows_dir");
                return Value::fromString(system_lib::windows_dir());
            });
            interp.registerModuleFunction("system", "system_dir", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.system_dir");
                return Value::fromString(system_lib::system_dir());
            });
            interp.registerModuleFunction("system", "locale", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.locale");
                return Value::fromString(system_lib::locale());
            });
            interp.registerModuleFunction("system", "timezone", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.timezone");
                return Value::fromString(system_lib::timezone());
            });
            interp.registerModuleFunction("system", "machine_guid", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "system.machine_guid");
                return Value::fromString(system_lib::machine_guid());
            });
        });
        module_registry::registerModule("motherboard", [](Interpreter& interp) {
            interp.registerModuleFunction("motherboard", "manufacturer", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.manufacturer");
                return Value::fromString(motherboard_lib::baseboard_manufacturer());
            });
            interp.registerModuleFunction("motherboard", "product", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.product");
                return Value::fromString(motherboard_lib::baseboard_product());
            });
            interp.registerModuleFunction("motherboard", "version", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.version");
                return Value::fromString(motherboard_lib::baseboard_version());
            });
            interp.registerModuleFunction("motherboard", "serial", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.serial");
                return Value::fromString(motherboard_lib::baseboard_serial());
            });
            interp.registerModuleFunction("motherboard", "bios_vendor", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.bios_vendor");
                return Value::fromString(motherboard_lib::bios_vendor());
            });
            interp.registerModuleFunction("motherboard", "bios_version", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.bios_version");
                return Value::fromString(motherboard_lib::bios_version());
            });
            interp.registerModuleFunction("motherboard", "bios_release_date", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.bios_release_date");
                return Value::fromString(motherboard_lib::bios_release_date());
            });
            interp.registerModuleFunction("motherboard", "system_manufacturer", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.system_manufacturer");
                return Value::fromString(motherboard_lib::system_manufacturer());
            });
            interp.registerModuleFunction("motherboard", "system_product", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.system_product");
                return Value::fromString(motherboard_lib::system_product_name());
            });
            interp.registerModuleFunction("motherboard", "system_sku", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.system_sku");
                return Value::fromString(motherboard_lib::system_sku());
            });
            interp.registerModuleFunction("motherboard", "system_family", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.system_family");
                return Value::fromString(motherboard_lib::system_family());
            });
            interp.registerModuleFunction("motherboard", "has_data", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "motherboard.has_data");
                return Value::fromBool(motherboard_lib::has_data());
            });
        });
        module_registry::registerModule("vscode", [](Interpreter& interp) {
            interp.registerModuleFunction("vscode", "available", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "vscode.available");
                return Value::fromBool(vscode_lib::available());
            });
            interp.registerModuleFunction("vscode", "open", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "vscode.open");
                vscode_lib::open(interp.expectString(args[0], "vscode.open expects path string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("vscode", "open_file", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "vscode.open_file");
                vscode_lib::open_file(interp.expectString(args[0], "vscode.open_file expects path string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("vscode", "open_folder", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "vscode.open_folder");
                vscode_lib::open_folder(interp.expectString(args[0], "vscode.open_folder expects path string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("vscode", "new_window", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "vscode.new_window");
                vscode_lib::new_window(interp.expectString(args[0], "vscode.new_window expects path string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("vscode", "goto", [&interp](const std::vector<Value>& args) -> Value {
                if (args.size() < 2 || args.size() > 3) {
                    throw std::runtime_error("vscode.goto expects 2 or 3 argument(s)");
                }
                int line = static_cast<int>(interp.expectNumber(args[1], "vscode.goto expects line number"));
                int col = 1;
                if (args.size() == 3) {
                    col = static_cast<int>(interp.expectNumber(args[2], "vscode.goto expects column number"));
                }
                vscode_lib::go_to(interp.expectString(args[0], "vscode.goto expects file path string"), line, col);
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("vscode", "list_extensions", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "vscode.list_extensions");
                return Value::fromString(vscode_lib::list_extensions());
            });
            interp.registerModuleFunction("vscode", "install_extension", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "vscode.install_extension");
                vscode_lib::install_extension(interp.expectString(args[0], "vscode.install_extension expects extension id string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("vscode", "uninstall_extension", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "vscode.uninstall_extension");
                vscode_lib::uninstall_extension(interp.expectString(args[0], "vscode.uninstall_extension expects extension id string"));
                return Value::fromNumber(0.0);
            });
            interp.registerModuleFunction("vscode", "version", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 0, "vscode.version");
                return Value::fromString(vscode_lib::version());
            });
            interp.registerModuleFunction("vscode", "run", [&interp](const std::vector<Value>& args) -> Value {
                interp.expectArity(args, 1, "vscode.run");
                return Value::fromString(vscode_lib::run_script(interp.expectString(args[0], "vscode.run expects file path string")));
            });
        });
    }
};

static BuiltinsRegistrar builtinsRegistrar;
} // namespace
