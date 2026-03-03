#include "../include/interpreter.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "../include/all_libs.h"
#include "../include/module_registry.h"

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

void Interpreter::registerModuleFunction(const std::string& moduleName,
                                         const std::string& funcName,
                                         std::function<Value(const std::vector<Value>&)> func) {
    modules[moduleName][funcName] = std::move(func);
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

Value Interpreter::visitExpr(ASTNode* node) {
    if (auto n = dynamic_cast<NumberNode*>(node)) {
        return Value::fromNumber(n->value);
    }
    if (auto b = dynamic_cast<BoolNode*>(node)) {
        return Value::fromBool(b->value);
    }
    if (auto s = dynamic_cast<StringNode*>(node)) {
        return Value::fromString(s->value);
    }
    if (auto v = dynamic_cast<VarNode*>(node)) {
        return getVariable(v->name);
    }
    if (auto list = dynamic_cast<ListNode*>(node)) {
        std::vector<Value> items;
        items.reserve(list->items.size());
        for (const auto& item : list->items) {
            items.push_back(visitExpr(item.get()));
        }
        return Value::fromList(std::move(items));
    }
    if (auto map = dynamic_cast<MapNode*>(node)) {
        std::unordered_map<std::string, Value> items;
        for (const auto& kv : map->items) {
            std::string key = valueKeyString(visitExpr(kv.first.get()));
            items[key] = visitExpr(kv.second.get());
        }
        return Value::fromMap(std::move(items));
    }
    if (auto idx = dynamic_cast<IndexNode*>(node)) {
        Value target = visitExpr(idx->target.get());
        Value indexValue = visitExpr(idx->index.get());
        if (target.type == ValueType::LIST) {
            double raw = expectNumber(indexValue, "List index must be integer");
            if (std::floor(raw) != raw) {
                throw std::runtime_error("List index must be integer");
            }
            int i = static_cast<int>(raw);
            if (i < 0 || i >= static_cast<int>(target.list.size())) {
                throw std::runtime_error("List index out of range");
            }
            return target.list[static_cast<size_t>(i)];
        }
        if (target.type == ValueType::MAP) {
            std::string key = valueKeyString(indexValue);
            auto it = target.map.find(key);
            if (it == target.map.end()) {
                throw std::runtime_error("Map key not found: " + key);
            }
            return it->second;
        }
        if (target.type == ValueType::STRING) {
            double raw = expectNumber(indexValue, "String index must be integer");
            if (std::floor(raw) != raw) {
                throw std::runtime_error("String index must be integer");
            }
            int i = static_cast<int>(raw);
            if (i < 0 || i >= static_cast<int>(target.str.size())) {
                throw std::runtime_error("String index out of range");
            }
            return Value::fromString(std::string(1, target.str[static_cast<size_t>(i)]));
        }
        throw std::runtime_error("Indexing only supports list, map, and string");
    }
    if (auto u = dynamic_cast<UnaryOpNode*>(node)) {
        if (u->op.type == TokenType::MINUS) {
            return Value::fromNumber(-expectNumber(visitExpr(u->operand.get()), "Unary '-' expects number"));
        }
        if (u->op.type == TokenType::NOT) {
            return Value::fromBool(!isTruthy(visitExpr(u->operand.get())));
        }
        throw std::runtime_error("Invalid unary operator");
    }
    if (auto bin = dynamic_cast<BinOpNode*>(node)) {
        if (bin->op.type == TokenType::AND) {
            Value left = visitExpr(bin->left.get());
            if (!isTruthy(left)) return Value::fromBool(false);
            return Value::fromBool(isTruthy(visitExpr(bin->right.get())));
        }
        if (bin->op.type == TokenType::OR) {
            Value left = visitExpr(bin->left.get());
            if (isTruthy(left)) return Value::fromBool(true);
            return Value::fromBool(isTruthy(visitExpr(bin->right.get())));
        }

        Value left = visitExpr(bin->left.get());
        Value right = visitExpr(bin->right.get());

        switch (bin->op.type) {
        case TokenType::PLUS:
            if (left.type == ValueType::STRING || right.type == ValueType::STRING) {
                return Value::fromString(valueToString(left) + valueToString(right));
            }
            if (left.type == ValueType::LIST && right.type == ValueType::LIST) {
                std::vector<Value> out = left.list;
                out.insert(out.end(), right.list.begin(), right.list.end());
                return Value::fromList(std::move(out));
            }
            return Value::fromNumber(expectNumber(left, "Addition expects numbers") +
                                     expectNumber(right, "Addition expects numbers"));
        case TokenType::MINUS:
            return Value::fromNumber(expectNumber(left, "Subtraction expects numbers") -
                                     expectNumber(right, "Subtraction expects numbers"));
        case TokenType::MUL:
            return Value::fromNumber(expectNumber(left, "Multiplication expects numbers") *
                                     expectNumber(right, "Multiplication expects numbers"));
        case TokenType::DIV: {
            double denom = expectNumber(right, "Division expects numbers");
            if (denom == 0.0) throw std::runtime_error("Division by zero");
            return Value::fromNumber(expectNumber(left, "Division expects numbers") / denom);
        }
        case TokenType::MOD: {
            double denom = expectNumber(right, "Modulo expects numbers");
            if (denom == 0.0) throw std::runtime_error("Modulo by zero");
            return Value::fromNumber(std::fmod(expectNumber(left, "Modulo expects numbers"), denom));
        }
        case TokenType::EQ:
            return Value::fromBool(valuesEqual(left, right));
        case TokenType::NE:
            return Value::fromBool(!valuesEqual(left, right));
        case TokenType::LT:
            return Value::fromBool(expectNumber(left, "Comparison expects numbers") <
                                   expectNumber(right, "Comparison expects numbers"));
        case TokenType::LE:
            return Value::fromBool(expectNumber(left, "Comparison expects numbers") <=
                                   expectNumber(right, "Comparison expects numbers"));
        case TokenType::GT:
            return Value::fromBool(expectNumber(left, "Comparison expects numbers") >
                                   expectNumber(right, "Comparison expects numbers"));
        case TokenType::GE:
            return Value::fromBool(expectNumber(left, "Comparison expects numbers") >=
                                   expectNumber(right, "Comparison expects numbers"));
        default:
            break;
        }
        throw std::runtime_error("Invalid binary operator");
    }
    if (auto mf = dynamic_cast<ModuleFuncNode*>(node)) {
        auto itModule = modules.find(mf->moduleName);
        if (itModule == modules.end()) {
            throw std::runtime_error("Module '" + mf->moduleName + "' not imported");
        }
        auto itFunc = itModule->second.find(mf->funcName);
        if (itFunc == itModule->second.end()) {
            throw std::runtime_error("Function '" + mf->funcName + "' not found in module '" + mf->moduleName + "'");
        }
        std::vector<Value> args;
        args.reserve(mf->args.size());
        for (const auto& arg : mf->args) {
            args.push_back(visitExpr(arg.get()));
        }
        return itFunc->second(args);
    }
    if (dynamic_cast<FunctionCallNode*>(node)) {
        return visit(node);
    }
    throw std::runtime_error("Invalid expression");
}

bool Interpreter::isTruthyPublic(const Value& value) const {
    return isTruthy(value);
}

std::string Interpreter::valueToStringPublic(const Value& value) const {
    return valueToString(value);
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
        auto initializer = module_registry::getModule(importName);
        if (!initializer) {
            throw std::runtime_error("Unknown module: " + importName);
        }
        (*initializer)(*this);
        if (!imp->alias.empty()) {
            modules[imp->alias] = modules[importName];
        }
        return Value::fromNumber(0.0);
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
