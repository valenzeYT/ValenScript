#include "interpreter.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "lib/all_libs.h"

namespace {
struct ReturnSignal {
    Value value;
};

struct BreakSignal {};
struct ContinueSignal {};

bool collectAssignmentPath(ASTNode* node, std::string& rootName, std::vector<ASTNode*>& indices) {
    if (auto v = dynamic_cast<VarNode*>(node)) {
        rootName = v->name;
        return true;
    }
    if (auto idx = dynamic_cast<IndexNode*>(node)) {
        if (!collectAssignmentPath(idx->target.get(), rootName, indices)) {
            return false;
        }
        indices.push_back(idx->index.get());
        return true;
    }
    return false;
}
} // namespace

Interpreter::Interpreter() {
    modules.clear();
    functions.clear();
    events.clear();
    scopes.clear();
    task_lib::reset_state(nextTaskId, nextChannelId, tasks, channels);
}

void Interpreter::interpret(const std::vector<std::unique_ptr<ASTNode>>& statements) {
    out_lib::configure_stdout();
    scopes.push_back({});
    for (const auto& stmt : statements) {
        visit(stmt.get());
    }
}

void Interpreter::executeBlock(const std::vector<std::unique_ptr<ASTNode>>& statements, bool newScope) {
    if (newScope) {
        scopes.push_back({});
    }
    try {
        for (const auto& stmt : statements) {
            visit(stmt.get());
        }
    } catch (...) {
        if (newScope) {
            scopes.pop_back();
        }
        throw;
    }
    if (newScope) {
        scopes.pop_back();
    }
}

double Interpreter::expectNumber(const Value& value, const std::string& errorMessage) const {
    if (value.type == ValueType::NUMBER) {
        return value.number;
    }
    if (value.type == ValueType::BOOL) {
        return value.boolean ? 1.0 : 0.0;
    }
    throw std::runtime_error(errorMessage);
}

std::string Interpreter::expectString(const Value& value, const std::string& errorMessage) const {
    if (value.type != ValueType::STRING) {
        throw std::runtime_error(errorMessage);
    }
    return value.str;
}

void Interpreter::expectArity(const std::vector<Value>& args, size_t expected, const std::string& name) const {
    if (args.size() != expected) {
        throw std::runtime_error(name + " expects " + std::to_string(expected) + " argument(s)");
    }
}

Value Interpreter::getVariable(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    throw std::runtime_error("Undefined variable: " + name);
}

void Interpreter::setVariable(const std::string& name, const Value& value) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            found->second = value;
            return;
        }
    }
    scopes.back()[name] = value;
}

void Interpreter::fireEvent(const std::string& name, const std::vector<Value>& args) {
    auto it = events.find(name);
    if (it == events.end()) {
        return;
    }
    for (const auto& def : it->second) {
        if (def.body != nullptr) {
            scopes.push_back({});
            for (size_t i = 0; i < def.params.size(); ++i) {
                if (i < args.size()) {
                    scopes.back()[def.params[i]] = args[i];
                } else {
                    scopes.back()[def.params[i]] = Value::fromNumber(0.0);
                }
            }
            try {
                executeBlock(*def.body, false);
            } catch (...) {
                scopes.pop_back();
                throw;
            }
            scopes.pop_back();
        }
    }
}

bool Interpreter::isTruthy(const Value& value) const {
    if (value.type == ValueType::NUMBER) {
        return value.number != 0.0;
    }
    if (value.type == ValueType::BOOL) {
        return value.boolean;
    }
    if (value.type == ValueType::STRING) {
        return !value.str.empty();
    }
    if (value.type == ValueType::LIST) {
        return !value.list.empty();
    }
    return !value.map.empty();
}

bool Interpreter::valuesEqual(const Value& a, const Value& b) const {
    if (a.type == ValueType::BOOL && b.type == ValueType::NUMBER) {
        return (a.boolean ? 1.0 : 0.0) == b.number;
    }
    if (a.type == ValueType::NUMBER && b.type == ValueType::BOOL) {
        return a.number == (b.boolean ? 1.0 : 0.0);
    }
    if (a.type != b.type) {
        return false;
    }
    if (a.type == ValueType::NUMBER) {
        return a.number == b.number;
    }
    if (a.type == ValueType::BOOL) {
        return a.boolean == b.boolean;
    }
    if (a.type == ValueType::STRING) {
        return a.str == b.str;
    }
    if (a.type == ValueType::LIST) {
        if (a.list.size() != b.list.size()) {
            return false;
        }
        for (size_t i = 0; i < a.list.size(); ++i) {
            if (!valuesEqual(a.list[i], b.list[i])) {
                return false;
            }
        }
        return true;
    }
    if (a.map.size() != b.map.size()) {
        return false;
    }
    for (const auto& kv : a.map) {
        auto it = b.map.find(kv.first);
        if (it == b.map.end() || !valuesEqual(kv.second, it->second)) {
            return false;
        }
    }
    return true;
}

std::string Interpreter::valueToString(const Value& value) const {
    if (value.type == ValueType::NUMBER) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << value.number;
        std::string out = oss.str();
        while (!out.empty() && out.back() == '0') {
            out.pop_back();
        }
        if (!out.empty() && out.back() == '.') {
            out.pop_back();
        }
        if (out == "-0") out = "0";
        if (out.empty()) out = "0";
        return out;
    }
    if (value.type == ValueType::BOOL) {
        return value.boolean ? "true" : "false";
    }
    if (value.type == ValueType::STRING) {
        return value.str;
    }
    if (value.type == ValueType::LIST) {
        std::string out = "[";
        for (size_t i = 0; i < value.list.size(); ++i) {
            if (i) out += "|";
            out += valueToString(value.list[i]);
        }
        out += "]";
        return out;
    }
    std::string out = "{";
    bool first = true;
    for (const auto& kv : value.map) {
        if (!first) out += "|";
        first = false;
        out += kv.first + ":" + valueToString(kv.second);
    }
    out += "}";
    return out;
}

std::string Interpreter::valueKeyString(const Value& value) const {
    if (value.type == ValueType::STRING) {
        return value.str;
    }
    if (value.type == ValueType::NUMBER || value.type == ValueType::BOOL) {
        return valueToString(value);
    }
    throw std::runtime_error("Map keys must be string, number, or bool");
}

void Interpreter::visitPrint(PrintNode* node) {
    std::vector<std::string> output;
    output.reserve(node->args.size());
    for (size_t i = 0; i < node->args.size(); ++i) {
        Value v = visitExpr(node->args[i].get());
        output.push_back(valueToString(v));
    }
    out_lib::print_line(output);
}

Value Interpreter::visit(ASTNode* node) {
    if (auto p = dynamic_cast<PrintNode*>(node)) {
        visitPrint(p);
        return Value::fromNumber(0.0);
    }

    if (auto e = dynamic_cast<ExprStmtNode*>(node)) {
        visitExpr(e->expr.get());
        return Value::fromNumber(0.0);
    }

    if (auto a = dynamic_cast<AssignNode*>(node)) {
        Value assigned = visitExpr(a->value.get());
        if (auto v = dynamic_cast<VarNode*>(a->target.get())) {
            setVariable(v->name, assigned);
            return Value::fromNumber(0.0);
        }

        std::string rootName;
        std::vector<ASTNode*> indices;
        if (!collectAssignmentPath(a->target.get(), rootName, indices) || indices.empty()) {
            throw std::runtime_error("Invalid assignment target");
        }

        Value rootValue = getVariable(rootName);
        Value* cursor = &rootValue;

        auto toIndex = [this](const Value& indexValue, const std::string& msg) -> int {
            double raw = expectNumber(indexValue, msg);
            if (std::floor(raw) != raw) {
                throw std::runtime_error(msg);
            }
            return static_cast<int>(raw);
        };

        for (size_t i = 0; i < indices.size(); ++i) {
            Value indexValue = visitExpr(indices[i]);
            const bool isLast = (i + 1 == indices.size());

            if (cursor->type == ValueType::LIST) {
                int idx = toIndex(indexValue, "List index must be integer");
                if (idx < 0 || idx >= static_cast<int>(cursor->list.size())) {
                    throw std::runtime_error("List index out of range");
                }
                if (isLast) {
                    cursor->list[static_cast<size_t>(idx)] = assigned;
                } else {
                    cursor = &cursor->list[static_cast<size_t>(idx)];
                }
                continue;
            }

            if (cursor->type == ValueType::MAP) {
                std::string key = valueKeyString(indexValue);
                if (isLast) {
                    cursor->map[key] = assigned;
                } else {
                    auto it = cursor->map.find(key);
                    if (it == cursor->map.end()) {
                        throw std::runtime_error("Map key not found: " + key);
                    }
                    cursor = &it->second;
                }
                continue;
            }

            if (cursor->type == ValueType::STRING) {
                int idx = toIndex(indexValue, "String index must be integer");
                if (idx < 0 || idx >= static_cast<int>(cursor->str.size())) {
                    throw std::runtime_error("String index out of range");
                }
                if (!isLast) {
                    throw std::runtime_error("Cannot index into string character");
                }
                if (assigned.type != ValueType::STRING || assigned.str.size() != 1) {
                    throw std::runtime_error("String index assignment expects single-character string");
                }
                cursor->str[static_cast<size_t>(idx)] = assigned.str[0];
                continue;
            }

            throw std::runtime_error("Index assignment only supports list, map, and string targets");
        }

        setVariable(rootName, rootValue);
        return Value::fromNumber(0.0);
    }

    if (auto i = dynamic_cast<IfNode*>(node)) {
        if (isTruthy(visitExpr(i->condition.get()))) {
            executeBlock(i->thenBody, true);
        } else {
            executeBlock(i->elseBody, true);
        }
        return Value::fromNumber(0.0);
    }

    if (auto w = dynamic_cast<WhileNode*>(node)) {
        while (isTruthy(visitExpr(w->condition.get()))) {
            try {
                executeBlock(w->body, true);
            } catch (const ContinueSignal&) {
                continue;
            } catch (const BreakSignal&) {
                break;
            }
        }
        return Value::fromNumber(0.0);
    }

    if (auto r = dynamic_cast<RepeatNode*>(node)) {
        double countNum = expectNumber(visitExpr(r->countExpr.get()), "repeat times expects number");
        if (countNum < 0 || std::floor(countNum) != countNum) {
            throw std::runtime_error("repeat times expects a non-negative integer");
        }
        int count = static_cast<int>(countNum);
        for (int i = 0; i < count; ++i) {
            try {
                executeBlock(r->body, true);
            } catch (const ContinueSignal&) {
                continue;
            } catch (const BreakSignal&) {
                break;
            }
        }
        return Value::fromNumber(0.0);
    }

    if (auto f = dynamic_cast<ForNode*>(node)) {
        double startNum = expectNumber(visitExpr(f->startExpr.get()), "for start expects number");
        double endNum = expectNumber(visitExpr(f->endExpr.get()), "for end expects number");
        if (std::floor(startNum) != startNum || std::floor(endNum) != endNum) {
            throw std::runtime_error("for bounds must be integers");
        }

        int start = static_cast<int>(startNum);
        int end = static_cast<int>(endNum);
        scopes.push_back({});
        for (int i = start; (start <= end) ? (i <= end) : (i >= end); (start <= end) ? ++i : --i) {
            setVariable(f->variable, Value::fromNumber(i));
            try {
                executeBlock(f->body, true);
            } catch (const ContinueSignal&) {
                continue;
            } catch (const BreakSignal&) {
                break;
            }
        }
        scopes.pop_back();
        return Value::fromNumber(0.0);
    }

    if (dynamic_cast<BreakNode*>(node)) {
        throw BreakSignal{};
    }

    if (dynamic_cast<ContinueNode*>(node)) {
        throw ContinueSignal{};
    }

    if (auto ret = dynamic_cast<ReturnNode*>(node)) {
        throw ReturnSignal{visitExpr(ret->expr.get())};
    }

    if (auto fn = dynamic_cast<FunctionDefNode*>(node)) {
        functions[fn->name] = FunctionDef{fn->params, &fn->body};
        return Value::fromNumber(0.0);
    }

    if (auto ev = dynamic_cast<EventDefNode*>(node)) {
        events[ev->name].push_back(EventDef{ev->params, &ev->body});
        return Value::fromNumber(0.0);
    }

    if (auto wt = dynamic_cast<WaitNode*>(node)) {
        double seconds = expectNumber(visitExpr(wt->seconds.get()), "wait expects number");
        if (seconds > 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
        }
        return Value::fromNumber(0.0);
    }

    if (auto imp = dynamic_cast<ImportNode*>(node)) {
        const std::string importName = imp->moduleName;
        if (importName == "os") {
            modules["os"]["read"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.read");
                Value out = Value::fromString(os_lib::read(expectString(args[0], "os.read expects string")));
                fireEvent("os.event.read", {args[0], out});
                return out;
            };
            modules["os"]["write"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "os.write");
                os_lib::write(expectString(args[0], "os.write expects filename string"),
                              expectString(args[1], "os.write expects content string"));
                fireEvent("os.event.write", {args[0], args[1]});
                return Value::fromNumber(0.0);
            };
            modules["os"]["clear"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.clear");
                os_lib::clear(expectString(args[0], "os.clear expects string"));
                fireEvent("os.event.clear", {args[0]});
                return Value::fromNumber(0.0);
            };
            modules["os"]["destroy"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.destroy");
                os_lib::destroy(expectString(args[0], "os.destroy expects string"));
                fireEvent("os.event.destroy", {args[0]});
                return Value::fromNumber(0.0);
            };
            modules["os"]["rename"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "os.rename");
                os_lib::rename(expectString(args[0], "os.rename expects source string"),
                               expectString(args[1], "os.rename expects target string"));
                fireEvent("os.event.rename", {args[0], args[1]});
                return Value::fromNumber(0.0);
            };
            modules["os"]["exists"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.exists");
                Value out = Value::fromBool(os_lib::exists(expectString(args[0], "os.exists expects string")));
                fireEvent("os.event.exists", {args[0], out});
                return out;
            };
            modules["os"]["size"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.size");
                Value out = Value::fromNumber(static_cast<double>(os_lib::size(expectString(args[0], "os.size expects string"))));
                fireEvent("os.event.size", {args[0], out});
                return out;
            };
            modules["os"]["append"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "os.append");
                os_lib::append(expectString(args[0], "os.append expects filename string"),
                               expectString(args[1], "os.append expects content string"));
                fireEvent("os.event.append", {args[0], args[1]});
                return Value::fromNumber(0.0);
            };
            modules["os"]["creation_time"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.creation_time");
                Value out = Value::fromNumber(static_cast<double>(os_lib::creation_time(expectString(args[0], "os.creation_time expects string"))));
                fireEvent("os.event.creation_time", {args[0], out});
                return out;
            };
            modules["os"]["modified"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.modified");
                Value out = Value::fromNumber(static_cast<double>(os_lib::modified(expectString(args[0], "os.modified expects string"))));
                fireEvent("os.event.modified", {args[0], out});
                return out;
            };
            modules["os"]["abort"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "os.abort");
                fireEvent("os.event.abort");
                os_lib::abort();
                return Value::fromNumber(0.0);
            };
            modules["os"]["extension"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.extension");
                Value out = Value::fromString(os_lib::extension(expectString(args[0], "os.extension expects string")));
                fireEvent("os.event.extension", {args[0], out});
                return out;
            };
            modules["os"]["name"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.name");
                Value out = Value::fromString(os_lib::name(expectString(args[0], "os.name expects string")));
                fireEvent("os.event.name", {args[0], out});
                return out;
            };
            modules["os"]["getenv"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.getenv");
                Value out = Value::fromString(os_lib::getenv(expectString(args[0], "os.getenv expects string")));
                fireEvent("os.event.getenv", {args[0], out});
                return out;
            };
            modules["os"]["setenv"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "os.setenv");
                os_lib::setenv(expectString(args[0], "os.setenv expects key string"),
                               expectString(args[1], "os.setenv expects value string"));
                fireEvent("os.event.setenv", {args[0], args[1]});
                return Value::fromNumber(0.0);
            };
            modules["os"]["wait"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "os.wait");
                os_lib::wait(expectNumber(args[0], "os.wait expects number"));
                fireEvent("os.event.wait", {args[0]});
                return Value::fromNumber(0.0);
            };
        } else if (importName == "input") {
            auto add0 = [this](const std::string& name, const std::function<void()>& fn) {
                modules["input"][name] = [this, name, fn](const std::vector<Value>& args) -> Value {
                    expectArity(args, 0, "input." + name);
                    fn();
                    fireEvent("input.event." + name);
                    return Value::fromNumber(0.0);
                };
            };
            auto add1Num = [this](const std::string& name, const std::function<void(double)>& fn) {
                modules["input"][name] = [this, name, fn](const std::vector<Value>& args) -> Value {
                    expectArity(args, 1, "input." + name);
                    double value = expectNumber(args[0], "input." + name + " expects number");
                    fn(value);
                    fireEvent("input.event." + name, {args[0]});
                    return Value::fromNumber(0.0);
                };
            };
            auto add2Num = [this](const std::string& name, const std::function<void(double, double)>& fn) {
                modules["input"][name] = [this, name, fn](const std::vector<Value>& args) -> Value {
                    expectArity(args, 2, "input." + name);
                    double a = expectNumber(args[0], "input." + name + " expects number args");
                    double b = expectNumber(args[1], "input." + name + " expects number args");
                    fn(a, b);
                    fireEvent("input.event." + name, {args[0], args[1]});
                    return Value::fromNumber(0.0);
                };
            };
            auto add1Str = [this](const std::string& name, const std::function<void(const std::string&)>& fn) {
                modules["input"][name] = [this, name, fn](const std::vector<Value>& args) -> Value {
                    expectArity(args, 1, "input." + name);
                    std::string text = expectString(args[0], "input." + name + " expects string");
                    fn(text);
                    fireEvent("input.event." + name, {args[0]});
                    return Value::fromNumber(0.0);
                };
            };
            auto add0Num = [this](const std::string& name, const std::function<double()>& fn) {
                modules["input"][name] = [this, name, fn](const std::vector<Value>& args) -> Value {
                    expectArity(args, 0, "input." + name);
                    Value out = Value::fromNumber(fn());
                    fireEvent("input.event." + name, {out});
                    return out;
                };
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
        } else if (importName == "clipboard") {
            modules["clipboard"]["get"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "clipboard.get");
                return Value::fromString(clipboard_lib::get());
            };
            modules["clipboard"]["set"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "clipboard.set");
                clipboard_lib::set(expectString(args[0], "clipboard.set expects string"));
                return Value::fromNumber(0.0);
            };
            modules["clipboard"]["clear"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "clipboard.clear");
                clipboard_lib::clear();
                return Value::fromNumber(0.0);
            };
        } else if (importName == "random") {
            modules["random"]["seed"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "random.seed");
                random_lib::seed(static_cast<std::uint64_t>(expectNumber(args[0], "random.seed expects number")));
                return Value::fromNumber(0.0);
            };
            modules["random"]["int"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "random.int");
                int minVal = static_cast<int>(expectNumber(args[0], "random.int expects number args"));
                int maxVal = static_cast<int>(expectNumber(args[1], "random.int expects number args"));
                return Value::fromNumber(static_cast<double>(random_lib::randint(minVal, maxVal)));
            };
            modules["random"]["float"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "random.float");
                double minVal = expectNumber(args[0], "random.float expects number args");
                double maxVal = expectNumber(args[1], "random.float expects number args");
                return Value::fromNumber(random_lib::randfloat(minVal, maxVal));
            };
            modules["random"]["choice"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "random.choice");
                if (args[0].type != ValueType::LIST) {
                    throw std::runtime_error("random.choice expects list");
                }
                int idx = random_lib::randindex(static_cast<int>(args[0].list.size()));
                return args[0].list[static_cast<size_t>(idx)];
            };
        } else if (importName == "math") {
            modules["math"]["factorial"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.factorial");
                return Value::fromNumber(math_lib::factorial(expectNumber(args[0], "math.factorial expects number")));
            };
            modules["math"]["abs"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.abs");
                return Value::fromNumber(math_lib::abs_val(expectNumber(args[0], "math.abs expects number")));
            };
            modules["math"]["sqrt"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.sqrt");
                return Value::fromNumber(math_lib::sqrt_val(expectNumber(args[0], "math.sqrt expects number")));
            };
            modules["math"]["pow"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "math.pow");
                return Value::fromNumber(math_lib::pow_val(
                    expectNumber(args[0], "math.pow expects number args"),
                    expectNumber(args[1], "math.pow expects number args")));
            };
            modules["math"]["floor"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.floor");
                return Value::fromNumber(math_lib::floor_val(expectNumber(args[0], "math.floor expects number")));
            };
            modules["math"]["ceil"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.ceil");
                return Value::fromNumber(math_lib::ceil_val(expectNumber(args[0], "math.ceil expects number")));
            };
            modules["math"]["round"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.round");
                return Value::fromNumber(math_lib::round_val(expectNumber(args[0], "math.round expects number")));
            };
            modules["math"]["mod"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "math.mod");
                return Value::fromNumber(math_lib::mod_val(
                    expectNumber(args[0], "math.mod expects number args"),
                    expectNumber(args[1], "math.mod expects number args")));
            };
            modules["math"]["is_even"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.is_even");
                return Value::fromBool(math_lib::is_even_val(expectNumber(args[0], "math.is_even expects number")));
            };
            modules["math"]["is_odd"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.is_odd");
                return Value::fromBool(math_lib::is_odd_val(expectNumber(args[0], "math.is_odd expects number")));
            };
            modules["math"]["min"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "math.min");
                return Value::fromNumber(math_lib::min_val(
                    expectNumber(args[0], "math.min expects numbers"),
                    expectNumber(args[1], "math.min expects numbers")));
            };
            modules["math"]["max"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "math.max");
                return Value::fromNumber(math_lib::max_val(
                    expectNumber(args[0], "math.max expects numbers"),
                    expectNumber(args[1], "math.max expects numbers")));
            };
            modules["math"]["clamp"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "math.clamp");
                return Value::fromNumber(math_lib::clamp_val(
                    expectNumber(args[0], "math.clamp expects numbers"),
                    expectNumber(args[1], "math.clamp expects numbers"),
                    expectNumber(args[2], "math.clamp expects numbers")));
            };
            modules["math"]["lerp"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "math.lerp");
                return Value::fromNumber(math_lib::lerp_val(
                    expectNumber(args[0], "math.lerp expects numbers"),
                    expectNumber(args[1], "math.lerp expects numbers"),
                    expectNumber(args[2], "math.lerp expects numbers")));
            };
            modules["math"]["sin"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.sin");
                return Value::fromNumber(math_lib::sin_val(expectNumber(args[0], "math.sin expects number")));
            };
            modules["math"]["cos"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.cos");
                return Value::fromNumber(math_lib::cos_val(expectNumber(args[0], "math.cos expects number")));
            };
            modules["math"]["tan"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.tan");
                return Value::fromNumber(math_lib::tan_val(expectNumber(args[0], "math.tan expects number")));
            };
            modules["math"]["asin"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.asin");
                return Value::fromNumber(math_lib::asin_val(expectNumber(args[0], "math.asin expects number")));
            };
            modules["math"]["acos"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.acos");
                return Value::fromNumber(math_lib::acos_val(expectNumber(args[0], "math.acos expects number")));
            };
            modules["math"]["atan"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.atan");
                return Value::fromNumber(math_lib::atan_val(expectNumber(args[0], "math.atan expects number")));
            };
            modules["math"]["atan2"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "math.atan2");
                return Value::fromNumber(math_lib::atan2_val(
                    expectNumber(args[0], "math.atan2 expects numbers"),
                    expectNumber(args[1], "math.atan2 expects numbers")));
            };
            modules["math"]["log"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.log");
                return Value::fromNumber(math_lib::log_val(expectNumber(args[0], "math.log expects number")));
            };
            modules["math"]["log10"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.log10");
                return Value::fromNumber(math_lib::log10_val(expectNumber(args[0], "math.log10 expects number")));
            };
            modules["math"]["exp"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.exp");
                return Value::fromNumber(math_lib::exp_val(expectNumber(args[0], "math.exp expects number")));
            };
            modules["math"]["cbrt"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.cbrt");
                return Value::fromNumber(math_lib::cbrt_val(expectNumber(args[0], "math.cbrt expects number")));
            };
            modules["math"]["hypot"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "math.hypot");
                return Value::fromNumber(math_lib::hypot_val(
                    expectNumber(args[0], "math.hypot expects numbers"),
                    expectNumber(args[1], "math.hypot expects numbers")));
            };
            modules["math"]["sign"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.sign");
                return Value::fromNumber(math_lib::sign_val(expectNumber(args[0], "math.sign expects number")));
            };
            modules["math"]["trunc"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.trunc");
                return Value::fromNumber(math_lib::trunc_val(expectNumber(args[0], "math.trunc expects number")));
            };
            modules["math"]["frac"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.frac");
                return Value::fromNumber(math_lib::frac_val(expectNumber(args[0], "math.frac expects number")));
            };
            modules["math"]["gcd"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "math.gcd");
                return Value::fromNumber(math_lib::gcd_val(
                    expectNumber(args[0], "math.gcd expects numbers"),
                    expectNumber(args[1], "math.gcd expects numbers")));
            };
            modules["math"]["lcm"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "math.lcm");
                return Value::fromNumber(math_lib::lcm_val(
                    expectNumber(args[0], "math.lcm expects numbers"),
                    expectNumber(args[1], "math.lcm expects numbers")));
            };
            modules["math"]["deg2rad"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.deg2rad");
                return Value::fromNumber(math_lib::deg2rad_val(expectNumber(args[0], "math.deg2rad expects number")));
            };
            modules["math"]["rad2deg"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "math.rad2deg");
                return Value::fromNumber(math_lib::rad2deg_val(expectNumber(args[0], "math.rad2deg expects number")));
            };
            modules["math"]["pi"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "math.pi");
                return Value::fromNumber(math_lib::pi_val());
            };
        } else if (importName == "string") {
            modules["string"]["upper"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.upper");
                return Value::fromString(string_lib::upper(expectString(args[0], "string.upper expects string")));
            };
            modules["string"]["lower"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.lower");
                return Value::fromString(string_lib::lower(expectString(args[0], "string.lower expects string")));
            };
            modules["string"]["trim"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.trim");
                return Value::fromString(string_lib::trim(expectString(args[0], "string.trim expects string")));
            };
            modules["string"]["contains"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.contains");
                return Value::fromBool(string_lib::contains(
                    expectString(args[0], "string.contains expects string"),
                    expectString(args[1], "string.contains expects string")));
            };
            modules["string"]["starts_with"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.starts_with");
                return Value::fromBool(string_lib::starts_with(
                    expectString(args[0], "string.starts_with expects string"),
                    expectString(args[1], "string.starts_with expects string")));
            };
            modules["string"]["ends_with"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.ends_with");
                return Value::fromBool(string_lib::ends_with(
                    expectString(args[0], "string.ends_with expects string"),
                    expectString(args[1], "string.ends_with expects string")));
            };
            modules["string"]["replace"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "string.replace");
                return Value::fromString(string_lib::replace_all(
                    expectString(args[0], "string.replace expects string"),
                    expectString(args[1], "string.replace expects string"),
                    expectString(args[2], "string.replace expects string")));
            };
            modules["string"]["length"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.length");
                return Value::fromNumber(string_lib::length(expectString(args[0], "string.length expects string")));
            };
            modules["string"]["split"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.split");
                auto parts = string_lib::split(
                    expectString(args[0], "string.split expects string"),
                    expectString(args[1], "string.split expects string"));
                std::vector<Value> out;
                out.reserve(parts.size());
                for (const auto& p : parts) out.push_back(Value::fromString(p));
                return Value::fromList(std::move(out));
            };
            modules["string"]["join"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.join");
                if (args[0].type != ValueType::LIST) {
                    throw std::runtime_error("string.join expects list as first arg");
                }
                std::vector<std::string> items;
                items.reserve(args[0].list.size());
                for (const auto& v : args[0].list) items.push_back(valueToString(v));
                return Value::fromString(string_lib::join(items, expectString(args[1], "string.join expects separator string")));
            };
            modules["string"]["substring"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "string.substring");
                return Value::fromString(string_lib::substring(
                    expectString(args[0], "string.substring expects string"),
                    static_cast<int>(expectNumber(args[1], "string.substring expects number")),
                    static_cast<int>(expectNumber(args[2], "string.substring expects number"))));
            };
            modules["string"]["left"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.left");
                return Value::fromString(string_lib::left(
                    expectString(args[0], "string.left expects string"),
                    static_cast<int>(expectNumber(args[1], "string.left expects number"))));
            };
            modules["string"]["right"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.right");
                return Value::fromString(string_lib::right(
                    expectString(args[0], "string.right expects string"),
                    static_cast<int>(expectNumber(args[1], "string.right expects number"))));
            };
            modules["string"]["repeat"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.repeat");
                return Value::fromString(string_lib::repeat(
                    expectString(args[0], "string.repeat expects string"),
                    static_cast<int>(expectNumber(args[1], "string.repeat expects number"))));
            };
            modules["string"]["reverse"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.reverse");
                return Value::fromString(string_lib::reverse(expectString(args[0], "string.reverse expects string")));
            };
            modules["string"]["index_of"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.index_of");
                return Value::fromNumber(string_lib::index_of(
                    expectString(args[0], "string.index_of expects string"),
                    expectString(args[1], "string.index_of expects string")));
            };
            modules["string"]["last_index_of"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.last_index_of");
                return Value::fromNumber(string_lib::last_index_of(
                    expectString(args[0], "string.last_index_of expects string"),
                    expectString(args[1], "string.last_index_of expects string")));
            };
            modules["string"]["pad_left"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "string.pad_left");
                return Value::fromString(string_lib::pad_left(
                    expectString(args[0], "string.pad_left expects string"),
                    static_cast<int>(expectNumber(args[1], "string.pad_left expects number")),
                    expectString(args[2], "string.pad_left expects string")));
            };
            modules["string"]["pad_right"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "string.pad_right");
                return Value::fromString(string_lib::pad_right(
                    expectString(args[0], "string.pad_right expects string"),
                    static_cast<int>(expectNumber(args[1], "string.pad_right expects number")),
                    expectString(args[2], "string.pad_right expects string")));
            };
            modules["string"]["remove"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.remove");
                return Value::fromString(string_lib::remove_all(
                    expectString(args[0], "string.remove expects string"),
                    expectString(args[1], "string.remove expects string")));
            };
            modules["string"]["count"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.count");
                return Value::fromNumber(string_lib::count(
                    expectString(args[0], "string.count expects string"),
                    expectString(args[1], "string.count expects string")));
            };
            modules["string"]["capitalize"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.capitalize");
                return Value::fromString(string_lib::capitalize(expectString(args[0], "string.capitalize expects string")));
            };
            modules["string"]["title"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.title");
                return Value::fromString(string_lib::title(expectString(args[0], "string.title expects string")));
            };
            modules["string"]["is_alpha"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.is_alpha");
                return Value::fromBool(string_lib::is_alpha(expectString(args[0], "string.is_alpha expects string")));
            };
            modules["string"]["is_digit"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.is_digit");
                return Value::fromBool(string_lib::is_digit(expectString(args[0], "string.is_digit expects string")));
            };
            modules["string"]["is_alnum"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.is_alnum");
                return Value::fromBool(string_lib::is_alnum(expectString(args[0], "string.is_alnum expects string")));
            };
            modules["string"]["is_space"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.is_space");
                return Value::fromBool(string_lib::is_space(expectString(args[0], "string.is_space expects string")));
            };
            modules["string"]["char_at"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.char_at");
                return Value::fromString(string_lib::char_at(
                    expectString(args[0], "string.char_at expects string"),
                    static_cast<int>(expectNumber(args[1], "string.char_at expects number"))));
            };
            modules["string"]["from_char"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.from_char");
                return Value::fromString(string_lib::from_char(static_cast<int>(expectNumber(args[0], "string.from_char expects number"))));
            };
            modules["string"]["to_char"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "string.to_char");
                return Value::fromNumber(string_lib::to_char(expectString(args[0], "string.to_char expects string")));
            };
            modules["string"]["remove_prefix"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.remove_prefix");
                return Value::fromString(string_lib::remove_prefix(
                    expectString(args[0], "string.remove_prefix expects string"),
                    expectString(args[1], "string.remove_prefix expects string")));
            };
            modules["string"]["remove_suffix"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "string.remove_suffix");
                return Value::fromString(string_lib::remove_suffix(
                    expectString(args[0], "string.remove_suffix expects string"),
                    expectString(args[1], "string.remove_suffix expects string")));
            };
        } else if (importName == "gui") {
            modules["gui"]["msgbox"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.msgbox");
                gui_lib::msgbox(expectString(args[0], "gui.msgbox expects string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["confirm"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.confirm");
                bool ok = gui_lib::confirm(expectString(args[0], "gui.confirm expects string"));
                return Value::fromBool(ok);
            };
            modules["gui"]["prompt"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.prompt");
                return Value::fromString(gui_lib::prompt(expectString(args[0], "gui.prompt expects string")));
            };
            modules["gui"]["beep"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.beep");
                gui_lib::beep();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["info"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.info");
                gui_lib::info(expectString(args[0], "gui.info expects string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["warning"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.warning");
                gui_lib::warning(expectString(args[0], "gui.warning expects string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["error"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.error");
                gui_lib::error(expectString(args[0], "gui.error expects string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["yesnocancel"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.yesnocancel");
                return Value::fromNumber(static_cast<double>(gui_lib::yesnocancel(expectString(args[0], "gui.yesnocancel expects string"))));
            };
            modules["gui"]["prompt_default"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "gui.prompt_default");
                return Value::fromString(gui_lib::prompt_default(
                    expectString(args[0], "gui.prompt_default expects prompt string"),
                    expectString(args[1], "gui.prompt_default expects default string")));
            };
            modules["gui"]["open_file"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.open_file");
                return Value::fromString(gui_lib::open_file());
            };
            modules["gui"]["save_file"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.save_file");
                return Value::fromString(gui_lib::save_file());
            };
            modules["gui"]["pick_folder"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.pick_folder");
                return Value::fromString(gui_lib::pick_folder());
            };
            modules["gui"]["notify"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "gui.notify");
                gui_lib::notify(
                    expectString(args[0], "gui.notify expects title string"),
                    expectString(args[1], "gui.notify expects text string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["set_title"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.set_title");
                gui_lib::set_title(expectString(args[0], "gui.set_title expects string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["get_title"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.get_title");
                return Value::fromString(gui_lib::get_title());
            };
            modules["gui"]["show_console"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.show_console");
                gui_lib::show_console();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["hide_console"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.hide_console");
                gui_lib::hide_console();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["minimize_console"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.minimize_console");
                gui_lib::minimize_console();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["maximize_console"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.maximize_console");
                gui_lib::maximize_console();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["restore_console"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.restore_console");
                gui_lib::restore_console();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["set_console_pos"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "gui.set_console_pos");
                gui_lib::set_console_pos(
                    static_cast<int>(expectNumber(args[0], "gui.set_console_pos expects x number")),
                    static_cast<int>(expectNumber(args[1], "gui.set_console_pos expects y number")));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["set_console_size"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "gui.set_console_size");
                gui_lib::set_console_size(
                    static_cast<int>(expectNumber(args[0], "gui.set_console_size expects width number")),
                    static_cast<int>(expectNumber(args[1], "gui.set_console_size expects height number")));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["screen_width"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.screen_width");
                return Value::fromNumber(static_cast<double>(gui_lib::screen_width()));
            };
            modules["gui"]["screen_height"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.screen_height");
                return Value::fromNumber(static_cast<double>(gui_lib::screen_height()));
            };
            modules["gui"]["topmost_console"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.topmost_console");
                gui_lib::topmost_console(isTruthy(args[0]));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["create_window"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "gui.create_window");
                gui_lib::create_window(
                    expectString(args[0], "gui.create_window expects title string"),
                    static_cast<int>(expectNumber(args[1], "gui.create_window expects width number")),
                    static_cast<int>(expectNumber(args[2], "gui.create_window expects height number")));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["clear_window"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.clear_window");
                gui_lib::clear_window();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["add_text"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 5, "gui.add_text");
                return Value::fromNumber(static_cast<double>(gui_lib::add_text(
                    expectString(args[0], "gui.add_text expects text string"),
                    static_cast<int>(expectNumber(args[1], "gui.add_text expects x number")),
                    static_cast<int>(expectNumber(args[2], "gui.add_text expects y number")),
                    static_cast<int>(expectNumber(args[3], "gui.add_text expects width number")),
                    static_cast<int>(expectNumber(args[4], "gui.add_text expects height number")))));
            };
            modules["gui"]["add_section"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 5, "gui.add_section");
                return Value::fromNumber(static_cast<double>(gui_lib::add_section(
                    expectString(args[0], "gui.add_section expects title string"),
                    static_cast<int>(expectNumber(args[1], "gui.add_section expects x number")),
                    static_cast<int>(expectNumber(args[2], "gui.add_section expects y number")),
                    static_cast<int>(expectNumber(args[3], "gui.add_section expects width number")),
                    static_cast<int>(expectNumber(args[4], "gui.add_section expects height number")))));
            };
            modules["gui"]["add_button"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 5, "gui.add_button");
                return Value::fromNumber(static_cast<double>(gui_lib::add_button(
                    expectString(args[0], "gui.add_button expects label string"),
                    static_cast<int>(expectNumber(args[1], "gui.add_button expects x number")),
                    static_cast<int>(expectNumber(args[2], "gui.add_button expects y number")),
                    static_cast<int>(expectNumber(args[3], "gui.add_button expects width number")),
                    static_cast<int>(expectNumber(args[4], "gui.add_button expects height number")))));
            };
            modules["gui"]["button_clicked"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.button_clicked");
                return Value::fromBool(gui_lib::button_clicked(
                    static_cast<int>(expectNumber(args[0], "gui.button_clicked expects id number"))));
            };
            modules["gui"]["add_input"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 5, "gui.add_input");
                return Value::fromNumber(static_cast<double>(gui_lib::add_input(
                    expectString(args[0], "gui.add_input expects placeholder string"),
                    static_cast<int>(expectNumber(args[1], "gui.add_input expects x number")),
                    static_cast<int>(expectNumber(args[2], "gui.add_input expects y number")),
                    static_cast<int>(expectNumber(args[3], "gui.add_input expects width number")),
                    static_cast<int>(expectNumber(args[4], "gui.add_input expects height number")))));
            };
            modules["gui"]["add_editor"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 5, "gui.add_editor");
                return Value::fromNumber(static_cast<double>(gui_lib::add_editor(
                    expectString(args[0], "gui.add_editor expects text string"),
                    static_cast<int>(expectNumber(args[1], "gui.add_editor expects x number")),
                    static_cast<int>(expectNumber(args[2], "gui.add_editor expects y number")),
                    static_cast<int>(expectNumber(args[3], "gui.add_editor expects width number")),
                    static_cast<int>(expectNumber(args[4], "gui.add_editor expects height number")))));
            };
            modules["gui"]["input_text"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.input_text");
                return Value::fromString(gui_lib::input_text(
                    static_cast<int>(expectNumber(args[0], "gui.input_text expects id number"))));
            };
            modules["gui"]["set_input"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "gui.set_input");
                gui_lib::set_input(
                    static_cast<int>(expectNumber(args[0], "gui.set_input expects id number")),
                    expectString(args[1], "gui.set_input expects text string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["add_link"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 6, "gui.add_link");
                return Value::fromNumber(static_cast<double>(gui_lib::add_link(
                    expectString(args[0], "gui.add_link expects label string"),
                    expectString(args[1], "gui.add_link expects url string"),
                    static_cast<int>(expectNumber(args[2], "gui.add_link expects x number")),
                    static_cast<int>(expectNumber(args[3], "gui.add_link expects y number")),
                    static_cast<int>(expectNumber(args[4], "gui.add_link expects width number")),
                    static_cast<int>(expectNumber(args[5], "gui.add_link expects height number")))));
            };
            modules["gui"]["set_icon"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.set_icon");
                gui_lib::set_icon(expectString(args[0], "gui.set_icon expects path string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["open_link"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.open_link");
                gui_lib::open_link(expectString(args[0], "gui.open_link expects url string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["close_window"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.close_window");
                gui_lib::close_window();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["window_open"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.window_open");
                return Value::fromBool(gui_lib::window_open());
            };
            modules["gui"]["show_window"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.show_window");
                gui_lib::show_window();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["hide_window"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.hide_window");
                gui_lib::hide_window();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["focus_window"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.focus_window");
                gui_lib::focus_window();
                return Value::fromNumber(0.0);
            };
            modules["gui"]["set_window_title"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.set_window_title");
                gui_lib::set_window_title(expectString(args[0], "gui.set_window_title expects string"));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["get_window_title"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.get_window_title");
                return Value::fromString(gui_lib::get_window_title());
            };
            modules["gui"]["set_window_pos"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "gui.set_window_pos");
                gui_lib::set_window_pos(
                    static_cast<int>(expectNumber(args[0], "gui.set_window_pos expects x number")),
                    static_cast<int>(expectNumber(args[1], "gui.set_window_pos expects y number")));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["set_window_size"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "gui.set_window_size");
                gui_lib::set_window_size(
                    static_cast<int>(expectNumber(args[0], "gui.set_window_size expects width number")),
                    static_cast<int>(expectNumber(args[1], "gui.set_window_size expects height number")));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["window_width"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.window_width");
                return Value::fromNumber(static_cast<double>(gui_lib::window_width()));
            };
            modules["gui"]["window_height"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.window_height");
                return Value::fromNumber(static_cast<double>(gui_lib::window_height()));
            };
            modules["gui"]["topmost_window"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "gui.topmost_window");
                gui_lib::topmost_window(isTruthy(args[0]));
                return Value::fromNumber(0.0);
            };
            modules["gui"]["media_menu"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "gui.media_menu");
                gui_lib::media_menu();
                return Value::fromNumber(0.0);
            };
        } else if (importName == "requests") {
            modules["requests"]["get"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "requests.get");
                std::string out = requests_lib::get(expectString(args[0], "requests.get expects url string"));
                return Value::fromString(out);
            };
            modules["requests"]["delete"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "requests.delete");
                std::string out = requests_lib::del(expectString(args[0], "requests.delete expects url string"));
                return Value::fromString(out);
            };
            modules["requests"]["post"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "requests.post");
                std::string out = requests_lib::post(
                    expectString(args[0], "requests.post expects url string"),
                    expectString(args[1], "requests.post expects body string"));
                return Value::fromString(out);
            };
            modules["requests"]["put"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "requests.put");
                std::string out = requests_lib::put(
                    expectString(args[0], "requests.put expects url string"),
                    expectString(args[1], "requests.put expects body string"));
                return Value::fromString(out);
            };
            modules["requests"]["patch"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "requests.patch");
                std::string out = requests_lib::patch(
                    expectString(args[0], "requests.patch expects url string"),
                    expectString(args[1], "requests.patch expects body string"));
                return Value::fromString(out);
            };
            modules["requests"]["download"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "requests.download");
                requests_lib::download(
                    expectString(args[0], "requests.download expects url string"),
                    expectString(args[1], "requests.download expects out path string"));
                return Value::fromNumber(0.0);
            };
        } else if (importName == "json") {
            modules["json"]["parse"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "json.parse");
                return json_lib::parse(expectString(args[0], "json.parse expects string"));
            };
            modules["json"]["stringify"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "json.stringify");
                return Value::fromString(json_lib::stringify(args[0]));
            };
            modules["json"]["valid"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "json.valid");
                return Value::fromBool(json_lib::valid(expectString(args[0], "json.valid expects string")));
            };
            modules["json"]["pretty"] = [this](const std::vector<Value>& args) -> Value {
                if (args.empty() || args.size() > 2) {
                    throw std::runtime_error("json.pretty expects 1 or 2 argument(s)");
                }
                int indent = 2;
                if (args.size() == 2) {
                    indent = static_cast<int>(expectNumber(args[1], "json.pretty indent expects number"));
                }
                if (args[0].type == ValueType::STRING) {
                    Value parsed = json_lib::parse(args[0].str);
                    return Value::fromString(json_lib::pretty(parsed, indent));
                }
                return Value::fromString(json_lib::pretty(args[0], indent));
            };
        } else if (importName == "zip") {
            modules["zip"]["compress"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "zip.compress");
                return Value::fromBool(zip_lib::compress(
                    expectString(args[0], "zip.compress expects source path string"),
                    expectString(args[1], "zip.compress expects zip path string")));
            };
            modules["zip"]["create"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "zip.create");
                return Value::fromBool(zip_lib::compress(
                    expectString(args[0], "zip.create expects source path string"),
                    expectString(args[1], "zip.create expects zip path string")));
            };
            modules["zip"]["extract"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "zip.extract");
                return Value::fromBool(zip_lib::extract(
                    expectString(args[0], "zip.extract expects zip path string"),
                    expectString(args[1], "zip.extract expects output dir string")));
            };
            modules["zip"]["list"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "zip.list");
                return Value::fromString(zip_lib::list(
                    expectString(args[0], "zip.list expects zip path string")));
            };
            modules["zip"]["exists"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "zip.exists");
                return Value::fromBool(zip_lib::exists(
                    expectString(args[0], "zip.exists expects zip path string")));
            };
        } else if (importName == "conversions") {
            modules["conversions"]["convert"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 4, "conversions.convert");
                return Value::fromNumber(conversions_lib::convert(
                    expectString(args[0], "conversions.convert expects category string"),
                    expectString(args[1], "conversions.convert expects from unit string"),
                    expectString(args[2], "conversions.convert expects to unit string"),
                    expectNumber(args[3], "conversions.convert expects number value")));
            };
            modules["conversions"]["temperature"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "conversions.temperature");
                return Value::fromNumber(conversions_lib::temperature(
                    expectString(args[0], "conversions.temperature expects from unit string"),
                    expectString(args[1], "conversions.temperature expects to unit string"),
                    expectNumber(args[2], "conversions.temperature expects number value")));
            };
            modules["conversions"]["length"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "conversions.length");
                return Value::fromNumber(conversions_lib::length(
                    expectString(args[0], "conversions.length expects from unit string"),
                    expectString(args[1], "conversions.length expects to unit string"),
                    expectNumber(args[2], "conversions.length expects number value")));
            };
            modules["conversions"]["weight"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "conversions.weight");
                return Value::fromNumber(conversions_lib::weight(
                    expectString(args[0], "conversions.weight expects from unit string"),
                    expectString(args[1], "conversions.weight expects to unit string"),
                    expectNumber(args[2], "conversions.weight expects number value")));
            };
            modules["conversions"]["time"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "conversions.time");
                return Value::fromNumber(conversions_lib::time(
                    expectString(args[0], "conversions.time expects from unit string"),
                    expectString(args[1], "conversions.time expects to unit string"),
                    expectNumber(args[2], "conversions.time expects number value")));
            };
            modules["conversions"]["area"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "conversions.area");
                return Value::fromNumber(conversions_lib::area(
                    expectString(args[0], "conversions.area expects from unit string"),
                    expectString(args[1], "conversions.area expects to unit string"),
                    expectNumber(args[2], "conversions.area expects number value")));
            };
            modules["conversions"]["volume"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "conversions.volume");
                return Value::fromNumber(conversions_lib::volume(
                    expectString(args[0], "conversions.volume expects from unit string"),
                    expectString(args[1], "conversions.volume expects to unit string"),
                    expectNumber(args[2], "conversions.volume expects number value")));
            };
            modules["conversions"]["speed"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "conversions.speed");
                return Value::fromNumber(conversions_lib::speed(
                    expectString(args[0], "conversions.speed expects from unit string"),
                    expectString(args[1], "conversions.speed expects to unit string"),
                    expectNumber(args[2], "conversions.speed expects number value")));
            };
            modules["conversions"]["data"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 3, "conversions.data");
                return Value::fromNumber(conversions_lib::data(
                    expectString(args[0], "conversions.data expects from unit string"),
                    expectString(args[1], "conversions.data expects to unit string"),
                    expectNumber(args[2], "conversions.data expects number value")));
            };
        } else if (importName == "project") {
            modules["project"]["new"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "project.new");
                project_lib::new_project(expectString(args[0], "project.new expects name string"));
                return Value::fromNumber(0.0);
            };
            modules["project"]["name"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "project.name");
                return Value::fromString(project_lib::name());
            };
            modules["project"]["set"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "project.set");
                project_lib::set_value(
                    expectString(args[0], "project.set expects key string"),
                    expectString(args[1], "project.set expects value string"));
                return Value::fromNumber(0.0);
            };
            modules["project"]["get"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "project.get");
                return Value::fromString(project_lib::get_value(expectString(args[0], "project.get expects key string")));
            };
            modules["project"]["save"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "project.save");
                project_lib::save(expectString(args[0], "project.save expects path string"));
                return Value::fromNumber(0.0);
            };
            modules["project"]["load"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "project.load");
                return Value::fromString(project_lib::load(expectString(args[0], "project.load expects path string")));
            };
        } else if (importName == "assets") {
            modules["assets"]["add"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "assets.add");
                assets_lib::add(expectString(args[0], "assets.add expects path string"));
                return Value::fromNumber(0.0);
            };
            modules["assets"]["remove"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "assets.remove");
                assets_lib::remove(expectString(args[0], "assets.remove expects path string"));
                return Value::fromNumber(0.0);
            };
            modules["assets"]["has"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "assets.has");
                return Value::fromBool(assets_lib::has(expectString(args[0], "assets.has expects path string")));
            };
            modules["assets"]["count"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "assets.count");
                return Value::fromNumber(static_cast<double>(assets_lib::count()));
            };
            modules["assets"]["clear"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "assets.clear");
                assets_lib::clear();
                return Value::fromNumber(0.0);
            };
        } else if (importName == "ui") {
            modules["ui"]["set_status"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "ui.set_status");
                ui_lib::set_status(expectString(args[0], "ui.set_status expects text string"));
                return Value::fromNumber(0.0);
            };
            modules["ui"]["status"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "ui.status");
                return Value::fromString(ui_lib::status());
            };
            modules["ui"]["set_progress"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "ui.set_progress");
                ui_lib::set_progress(expectNumber(args[0], "ui.set_progress expects number"));
                return Value::fromNumber(0.0);
            };
            modules["ui"]["progress"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "ui.progress");
                return Value::fromNumber(ui_lib::progress());
            };
        } else if (importName == "undo") {
            modules["undo"]["push"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "undo.push");
                undo_lib::push(expectString(args[0], "undo.push expects action string"));
                return Value::fromNumber(0.0);
            };
            modules["undo"]["undo"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "undo.undo");
                return Value::fromString(undo_lib::undo());
            };
            modules["undo"]["redo"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "undo.redo");
                return Value::fromString(undo_lib::redo());
            };
            modules["undo"]["can_undo"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "undo.can_undo");
                return Value::fromBool(undo_lib::can_undo());
            };
            modules["undo"]["can_redo"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "undo.can_redo");
                return Value::fromBool(undo_lib::can_redo());
            };
            modules["undo"]["clear"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "undo.clear");
                undo_lib::clear();
                return Value::fromNumber(0.0);
            };
        } else if (importName == "jobs") {
            modules["jobs"]["push"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "jobs.push");
                jobs_lib::push(expectString(args[0], "jobs.push expects job string"));
                return Value::fromNumber(0.0);
            };
            modules["jobs"]["next"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "jobs.next");
                return Value::fromString(jobs_lib::next());
            };
            modules["jobs"]["count"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "jobs.count");
                return Value::fromNumber(static_cast<double>(jobs_lib::count()));
            };
            modules["jobs"]["clear"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "jobs.clear");
                jobs_lib::clear();
                return Value::fromNumber(0.0);
            };
        } else if (importName == "fs") {
            modules["fs"]["list"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "fs.list");
                return Value::fromString(fs_lib::list(expectString(args[0], "fs.list expects path string")));
            };
            modules["fs"]["mkdir"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "fs.mkdir");
                return Value::fromBool(fs_lib::mkdirs(expectString(args[0], "fs.mkdir expects path string")));
            };
            modules["fs"]["rmdir"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "fs.rmdir");
                return Value::fromBool(fs_lib::rmdir(expectString(args[0], "fs.rmdir expects path string")));
            };
            modules["fs"]["copy"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "fs.copy");
                return Value::fromBool(fs_lib::copy(
                    expectString(args[0], "fs.copy expects source string"),
                    expectString(args[1], "fs.copy expects target string")));
            };
            modules["fs"]["move"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 2, "fs.move");
                return Value::fromBool(fs_lib::move(
                    expectString(args[0], "fs.move expects source string"),
                    expectString(args[1], "fs.move expects target string")));
            };
        } else if (importName == "perf") {
            modules["perf"]["start"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "perf.start");
                perf_lib::start(expectString(args[0], "perf.start expects label string"));
                return Value::fromNumber(0.0);
            };
            modules["perf"]["stop"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "perf.stop");
                return Value::fromNumber(perf_lib::stop(expectString(args[0], "perf.stop expects label string")));
            };
            modules["perf"]["now_ms"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "perf.now_ms");
                return Value::fromNumber(perf_lib::now_ms());
            };
            modules["perf"]["sleep_ms"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "perf.sleep_ms");
                perf_lib::sleep_ms(expectNumber(args[0], "perf.sleep_ms expects milliseconds number"));
                return Value::fromNumber(0.0);
            };
        } else if (importName == "history") {
            modules["history"]["push"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "history.push");
                history_lib::push(expectString(args[0], "history.push expects action string"));
                return Value::fromNumber(0.0);
            };
            modules["history"]["undo"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "history.undo");
                return Value::fromString(history_lib::undo());
            };
            modules["history"]["redo"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "history.redo");
                return Value::fromString(history_lib::redo());
            };
            modules["history"]["can_undo"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "history.can_undo");
                return Value::fromBool(history_lib::can_undo());
            };
            modules["history"]["can_redo"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "history.can_redo");
                return Value::fromBool(history_lib::can_redo());
            };
            modules["history"]["clear"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "history.clear");
                history_lib::clear();
                return Value::fromNumber(0.0);
            };
        } else if (importName == "time") {
            modules["time"]["unix"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "time.unix");
                Value out = Value::fromNumber(static_cast<double>(time_lib::get_unix()));
                fireEvent("time.event.unix", {out});
                return out;
            };
            modules["time"]["unix_ms"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "time.unix_ms");
                Value out = Value::fromNumber(static_cast<double>(time_lib::get_unix_ms()));
                fireEvent("time.event.unix_ms", {out});
                return out;
            };
            modules["time"]["unix_us"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "time.unix_us");
                Value out = Value::fromNumber(static_cast<double>(time_lib::get_unix_us()));
                fireEvent("time.event.unix_us", {out});
                return out;
            };
        } else if (importName == "vscode") {
            modules["vscode"]["available"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "vscode.available");
                return Value::fromBool(vscode_lib::available());
            };
            modules["vscode"]["open"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "vscode.open");
                vscode_lib::open(expectString(args[0], "vscode.open expects path string"));
                return Value::fromNumber(0.0);
            };
            modules["vscode"]["open_file"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "vscode.open_file");
                vscode_lib::open_file(expectString(args[0], "vscode.open_file expects path string"));
                return Value::fromNumber(0.0);
            };
            modules["vscode"]["open_folder"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "vscode.open_folder");
                vscode_lib::open_folder(expectString(args[0], "vscode.open_folder expects path string"));
                return Value::fromNumber(0.0);
            };
            modules["vscode"]["new_window"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "vscode.new_window");
                vscode_lib::new_window(expectString(args[0], "vscode.new_window expects path string"));
                return Value::fromNumber(0.0);
            };
            modules["vscode"]["goto"] = [this](const std::vector<Value>& args) -> Value {
                if (args.size() < 2 || args.size() > 3) {
                    throw std::runtime_error("vscode.goto expects 2 or 3 argument(s)");
                }
                int line = static_cast<int>(expectNumber(args[1], "vscode.goto expects line number"));
                int col = 1;
                if (args.size() == 3) {
                    col = static_cast<int>(expectNumber(args[2], "vscode.goto expects column number"));
                }
                vscode_lib::go_to(expectString(args[0], "vscode.goto expects file path string"), line, col);
                return Value::fromNumber(0.0);
            };
            modules["vscode"]["list_extensions"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "vscode.list_extensions");
                return Value::fromString(vscode_lib::list_extensions());
            };
            modules["vscode"]["install_extension"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "vscode.install_extension");
                vscode_lib::install_extension(expectString(args[0], "vscode.install_extension expects extension id string"));
                return Value::fromNumber(0.0);
            };
            modules["vscode"]["uninstall_extension"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "vscode.uninstall_extension");
                vscode_lib::uninstall_extension(expectString(args[0], "vscode.uninstall_extension expects extension id string"));
                return Value::fromNumber(0.0);
            };
            modules["vscode"]["version"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 0, "vscode.version");
                return Value::fromString(vscode_lib::version());
            };
            modules["vscode"]["run"] = [this](const std::vector<Value>& args) -> Value {
                expectArity(args, 1, "vscode.run");
                return Value::fromString(vscode_lib::run_script(expectString(args[0], "vscode.run expects file path string")));
            };
        } else {
            throw std::runtime_error("Unknown module: " + imp->moduleName);
        }
        if (!imp->alias.empty()) {
            modules[imp->alias] = modules[importName];
        }
        return Value::fromNumber(0.0);
    }

    return visitExpr(node);
}

Value Interpreter::visitExpr(ASTNode* node) {
    if (auto n = dynamic_cast<NumberNode*>(node)) return Value::fromNumber(n->value);
    if (auto b = dynamic_cast<BoolNode*>(node)) return Value::fromBool(b->value);
    if (auto s = dynamic_cast<StringNode*>(node)) return Value::fromString(s->value);

    if (auto l = dynamic_cast<ListNode*>(node)) {
        std::vector<Value> items;
        items.reserve(l->items.size());
        for (const auto& it : l->items) {
            items.push_back(visitExpr(it.get()));
        }
        return Value::fromList(std::move(items));
    }

    if (auto m = dynamic_cast<MapNode*>(node)) {
        std::unordered_map<std::string, Value> out;
        for (const auto& kv : m->items) {
            Value key = visitExpr(kv.first.get());
            out[valueKeyString(key)] = visitExpr(kv.second.get());
        }
        return Value::fromMap(std::move(out));
    }

    if (auto idx = dynamic_cast<IndexNode*>(node)) {
        Value target = visitExpr(idx->target.get());
        Value index = visitExpr(idx->index.get());

        if (target.type == ValueType::LIST) {
            double raw = expectNumber(index, "List index must be number");
            if (std::floor(raw) != raw) {
                throw std::runtime_error("List index must be integer");
            }
            int i = static_cast<int>(raw);
            if (i < 0 || i >= static_cast<int>(target.list.size())) {
                throw std::runtime_error("List index out of range");
            }
            return target.list[i];
        }

        if (target.type == ValueType::STRING) {
            double raw = expectNumber(index, "String index must be number");
            if (std::floor(raw) != raw) {
                throw std::runtime_error("String index must be integer");
            }
            int i = static_cast<int>(raw);
            if (i < 0 || i >= static_cast<int>(target.str.size())) {
                throw std::runtime_error("String index out of range");
            }
            return Value::fromString(std::string(1, target.str[static_cast<size_t>(i)]));
        }

        if (target.type == ValueType::MAP) {
            std::string key = valueKeyString(index);
            auto it = target.map.find(key);
            if (it == target.map.end()) {
                throw std::runtime_error("Map key not found: " + key);
            }
            return it->second;
        }

        throw std::runtime_error("Indexing only supports list, string, and map");
    }

    if (auto v = dynamic_cast<VarNode*>(node)) return getVariable(v->name);

    if (auto u = dynamic_cast<UnaryOpNode*>(node)) {
        Value val = visitExpr(u->operand.get());
        if (u->op.type == TokenType::NOT) {
            return Value::fromBool(!isTruthy(val));
        }
        double n = expectNumber(val, "Unary operation on non-number");
        if (u->op.type == TokenType::MINUS) return Value::fromNumber(-n);
        throw std::runtime_error("Unknown unary operator");
    }

    if (auto b = dynamic_cast<BinOpNode*>(node)) {
        Value lv = visitExpr(b->left.get());
        Value rv = visitExpr(b->right.get());

        if (b->op.type == TokenType::PLUS &&
            (lv.type == ValueType::STRING || rv.type == ValueType::STRING)) {
            return Value::fromString(valueToString(lv) + valueToString(rv));
        }

        if (b->op.type == TokenType::AND) return Value::fromBool(isTruthy(lv) && isTruthy(rv));
        if (b->op.type == TokenType::OR) return Value::fromBool(isTruthy(lv) || isTruthy(rv));
        if (b->op.type == TokenType::EQ) return Value::fromBool(valuesEqual(lv, rv));
        if (b->op.type == TokenType::NE) return Value::fromBool(!valuesEqual(lv, rv));

        if ((b->op.type == TokenType::LT || b->op.type == TokenType::GT || b->op.type == TokenType::LE || b->op.type == TokenType::GE) &&
            lv.type == ValueType::STRING && rv.type == ValueType::STRING) {
            if (b->op.type == TokenType::LT) return Value::fromBool(lv.str < rv.str);
            if (b->op.type == TokenType::GT) return Value::fromBool(lv.str > rv.str);
            if (b->op.type == TokenType::LE) return Value::fromBool(lv.str <= rv.str);
            return Value::fromBool(lv.str >= rv.str);
        }

        double left = expectNumber(lv, "Arithmetic/comparison on non-number");
        double right = expectNumber(rv, "Arithmetic/comparison on non-number");

        switch (b->op.type) {
            case TokenType::PLUS: return Value::fromNumber(left + right);
            case TokenType::MINUS: return Value::fromNumber(left - right);
            case TokenType::MUL: return Value::fromNumber(left * right);
            case TokenType::DIV:
                if (right == 0) throw std::runtime_error("Division by zero");
                return Value::fromNumber(left / right);
            case TokenType::MOD:
                if (right == 0) throw std::runtime_error("Modulo by zero");
                return Value::fromNumber(fmod(left, right));
            case TokenType::LT: return Value::fromBool(left < right);
            case TokenType::GT: return Value::fromBool(left > right);
            case TokenType::LE: return Value::fromBool(left <= right);
            case TokenType::GE: return Value::fromBool(left >= right);
            default: throw std::runtime_error("Unknown operator");
        }
    }

    if (auto mf = dynamic_cast<ModuleFuncNode*>(node)) {
        auto itModule = modules.find(mf->moduleName);
        if (itModule == modules.end()) {
            throw std::runtime_error("Module '" + mf->moduleName + "' not imported");
        }
        auto itFunc = itModule->second.find(mf->funcName);
        if (itFunc == itModule->second.end()) {
            throw std::runtime_error("Function not found: " + mf->funcName);
        }
        std::vector<Value> args;
        args.reserve(mf->args.size());
        for (const auto& arg : mf->args) {
            args.push_back(visitExpr(arg.get()));
        }
        Value out = itFunc->second(args);
        fireEvent(mf->moduleName + "." + mf->funcName, args);
        return out;
    }

    if (auto fc = dynamic_cast<FunctionCallNode*>(node)) {
        if (fc->name == "SPAWN") {
            if (fc->args.size() < 2) {
                throw std::runtime_error("spawn expects at least 2 argument(s): module, function, ...args");
            }
            std::string moduleName = expectString(visitExpr(fc->args[0].get()), "spawn expects module name string");
            std::string funcName = expectString(visitExpr(fc->args[1].get()), "spawn expects function name string");
            std::vector<Value> callArgs;
            callArgs.reserve(fc->args.size() - 2);
            for (size_t i = 2; i < fc->args.size(); ++i) {
                callArgs.push_back(visitExpr(fc->args[i].get()));
            }
            auto itModule = modules.find(moduleName);
            if (itModule == modules.end()) {
                throw std::runtime_error("spawn: module '" + moduleName + "' not imported");
            }
            auto itFunc = itModule->second.find(funcName);
            if (itFunc == itModule->second.end()) {
                throw std::runtime_error("spawn: function '" + funcName + "' not found in module '" + moduleName + "'");
            }
            auto fn = itFunc->second;
            int id = task_lib::spawn(nextTaskId, tasks, asyncMutex, [fn, callArgs]() mutable -> Value {
                return fn(callArgs);
            });
            return Value::fromNumber(static_cast<double>(id));
        }

        if (fc->name == "TASK_DONE") {
            if (fc->args.size() != 1) throw std::runtime_error("task_done expects 1 argument(s)");
            int id = static_cast<int>(expectNumber(visitExpr(fc->args[0].get()), "task_done expects task id number"));
            return Value::fromBool(task_lib::done(tasks, asyncMutex, id));
        }

        if (fc->name == "AWAIT") {
            if (fc->args.size() != 1) throw std::runtime_error("await expects 1 argument(s)");
            int id = static_cast<int>(expectNumber(visitExpr(fc->args[0].get()), "await expects task id number"));
            return task_lib::await(tasks, asyncMutex, id);
        }

        if (fc->name == "TASK_RESULT") {
            if (fc->args.size() != 1) throw std::runtime_error("task_result expects 1 argument(s)");
            int id = static_cast<int>(expectNumber(visitExpr(fc->args[0].get()), "task_result expects task id number"));
            return task_lib::result(tasks, asyncMutex, id);
        }

        if (fc->name == "CHANNEL_CREATE") {
            if (fc->args.size() != 0) throw std::runtime_error("channel_create expects 0 argument(s)");
            int id = task_lib::channel_create(nextChannelId, channels, asyncMutex);
            return Value::fromNumber(static_cast<double>(id));
        }

        if (fc->name == "CHANNEL_SEND") {
            if (fc->args.size() != 2) throw std::runtime_error("channel_send expects 2 argument(s)");
            int id = static_cast<int>(expectNumber(visitExpr(fc->args[0].get()), "channel_send expects channel id number"));
            Value v = visitExpr(fc->args[1].get());
            return Value::fromBool(task_lib::channel_send(channels, asyncMutex, id, v));
        }

        if (fc->name == "CHANNEL_RECV") {
            if (fc->args.size() != 1) throw std::runtime_error("channel_recv expects 1 argument(s)");
            int id = static_cast<int>(expectNumber(visitExpr(fc->args[0].get()), "channel_recv expects channel id number"));
            return task_lib::channel_recv(channels, asyncMutex, id);
        }

        if (fc->name == "SET_TIMEOUT") {
            if (fc->args.size() != 1) throw std::runtime_error("set_timeout expects 1 argument(s)");
            double ms = expectNumber(visitExpr(fc->args[0].get()), "set_timeout expects milliseconds number");
            int id = task_lib::set_timeout(nextTaskId, tasks, asyncMutex, ms);
            return Value::fromNumber(static_cast<double>(id));
        }

        if (fc->name == "LEN") {
            if (fc->args.size() != 1) throw std::runtime_error("len expects 1 argument(s)");
            Value v = visitExpr(fc->args[0].get());
            if (v.type == ValueType::STRING) return Value::fromNumber(static_cast<double>(v.str.size()));
            if (v.type == ValueType::LIST) return Value::fromNumber(static_cast<double>(v.list.size()));
            if (v.type == ValueType::MAP) return Value::fromNumber(static_cast<double>(v.map.size()));
            throw std::runtime_error("len expects string, list, or map");
        }

        if (fc->name == "TYPE") {
            if (fc->args.size() != 1) throw std::runtime_error("type expects 1 argument(s)");
            Value v = visitExpr(fc->args[0].get());
            if (v.type == ValueType::NUMBER) return Value::fromString("number");
            if (v.type == ValueType::BOOL) return Value::fromString("bool");
            if (v.type == ValueType::STRING) return Value::fromString("string");
            if (v.type == ValueType::LIST) return Value::fromString("list");
            return Value::fromString("map");
        }

        if (fc->name == "NUM") {
            if (fc->args.size() != 1) throw std::runtime_error("num expects 1 argument(s)");
            Value v = visitExpr(fc->args[0].get());
            if (v.type == ValueType::NUMBER) return v;
            if (v.type == ValueType::BOOL) return Value::fromNumber(v.boolean ? 1.0 : 0.0);
            if (v.type != ValueType::STRING) throw std::runtime_error("num expects number or string");
            size_t idx = 0;
            double parsed = 0.0;
            try {
                parsed = std::stod(v.str, &idx);
            } catch (...) {
                throw std::runtime_error("num failed to parse");
            }
            if (idx != v.str.size()) throw std::runtime_error("num failed to parse");
            return Value::fromNumber(parsed);
        }

        if (fc->name == "BOOL") {
            if (fc->args.size() != 1) throw std::runtime_error("bool expects 1 argument(s)");
            return Value::fromBool(isTruthy(visitExpr(fc->args[0].get())));
        }

        if (fc->name == "STRINGIFIED") {
            if (fc->args.size() != 1) throw std::runtime_error("stringified expects 1 argument(s)");
            return Value::fromString(valueToString(visitExpr(fc->args[0].get())));
        }

        if (fc->name == "NUMIFIED") {
            if (fc->args.size() != 1) throw std::runtime_error("numified expects 1 argument(s)");
            Value v = visitExpr(fc->args[0].get());
            if (v.type == ValueType::NUMBER) return v;
            if (v.type == ValueType::BOOL) return Value::fromNumber(v.boolean ? 1.0 : 0.0);
            if (v.type != ValueType::STRING) throw std::runtime_error("numified expects number, bool, or string");
            size_t idx = 0;
            double parsed = 0.0;
            try {
                parsed = std::stod(v.str, &idx);
            } catch (...) {
                throw std::runtime_error("numified failed to parse");
            }
            if (idx != v.str.size()) throw std::runtime_error("numified failed to parse");
            return Value::fromNumber(parsed);
        }

        if (fc->name == "BOOLIFIED") {
            if (fc->args.size() != 1) throw std::runtime_error("boolified expects 1 argument(s)");
            return Value::fromBool(isTruthy(visitExpr(fc->args[0].get())));
        }

        if (fc->name == "STDIN") {
            if (fc->args.size() > 1) {
                throw std::runtime_error("stdin expects 0 or 1 argument(s)");
            }
            if (fc->args.size() == 1) {
                Value prompt = visitExpr(fc->args[0].get());
                out_lib::write(valueToString(prompt));
                out_lib::flush();
            }
            std::string line;
            std::getline(std::cin, line);
            fireEvent("stdin.event.read", {Value::fromString(line)});
            return Value::fromString(line);
        }

        if (fc->name == "ASSERT") {
            if (fc->args.size() < 1 || fc->args.size() > 2) {
                throw std::runtime_error("assert expects 1 or 2 argument(s)");
            }
            Value cond = visitExpr(fc->args[0].get());
            if (!isTruthy(cond)) {
                std::string msg = "Assertion failed";
                if (fc->args.size() == 2) {
                    msg = valueToString(visitExpr(fc->args[1].get()));
                }
                throw std::runtime_error(msg);
            }
            return Value::fromBool(true);
        }

        auto it = functions.find(fc->name);
        if (it == functions.end()) {
            throw std::runtime_error("Undefined function: " + fc->name);
        }

        const FunctionDef& def = it->second;
        const bool hasVariadic = !def.params.empty() && def.params.back() == "_";
        const size_t fixedCount = hasVariadic ? (def.params.size() - 1) : def.params.size();
        if ((!hasVariadic && fc->args.size() != fixedCount) ||
            (hasVariadic && fc->args.size() < fixedCount)) {
            if (hasVariadic) {
                throw std::runtime_error("Function '" + fc->name + "' expects at least " + std::to_string(fixedCount) + " argument(s)");
            }
            throw std::runtime_error("Function '" + fc->name + "' expects " + std::to_string(fixedCount) + " argument(s)");
        }

        scopes.push_back({});
        for (size_t i = 0; i < fixedCount; ++i) {
            scopes.back()[def.params[i]] = visitExpr(fc->args[i].get());
        }
        if (hasVariadic) {
            std::vector<Value> rest;
            rest.reserve(fc->args.size() - fixedCount);
            for (size_t i = fixedCount; i < fc->args.size(); ++i) {
                rest.push_back(visitExpr(fc->args[i].get()));
            }
            scopes.back()["_"] = Value::fromList(std::move(rest));
        }

        Value result = Value::fromNumber(0.0);
        try {
            executeBlock(*def.body, false);
        } catch (const ReturnSignal& rs) {
            result = rs.value;
        }
        scopes.pop_back();
        return result;
    }

    throw std::runtime_error("Invalid expression");
}
