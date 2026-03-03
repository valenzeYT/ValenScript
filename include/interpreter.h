#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"
#include <functional>
#include <future>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

enum class ValueType {
    NUMBER,
    BOOL,
    STRING,
    LIST,
    MAP
};

struct Value {
    ValueType type = ValueType::NUMBER;
    double number = 0.0;
    bool boolean = false;
    std::string str;
    std::vector<Value> list;
    std::unordered_map<std::string, Value> map;

    static Value fromNumber(double n) {
        Value v;
        v.type = ValueType::NUMBER;
        v.number = n;
        return v;
    }

    static Value fromString(const std::string& s) {
        Value v;
        v.type = ValueType::STRING;
        v.str = s;
        return v;
    }

    static Value fromBool(bool b) {
        Value v;
        v.type = ValueType::BOOL;
        v.boolean = b;
        return v;
    }

    static Value fromList(std::vector<Value> l) {
        Value v;
        v.type = ValueType::LIST;
        v.list = std::move(l);
        return v;
    }

    static Value fromMap(std::unordered_map<std::string, Value> m) {
        Value v;
        v.type = ValueType::MAP;
        v.map = std::move(m);
        return v;
    }
};

struct FunctionDef {
    std::vector<std::string> params;
    const std::vector<std::unique_ptr<ASTNode>>* body = nullptr;
};

struct EventDef {
    std::vector<std::string> params;
    const std::vector<std::unique_ptr<ASTNode>>* body = nullptr;
};

class Interpreter {
public:
    Interpreter();
    void interpret(const std::vector<std::unique_ptr<ASTNode>>& statements);
    void registerModuleFunction(const std::string& moduleName,
                                const std::string& funcName,
                                std::function<Value(const std::vector<Value>&)> func);
    double expectNumber(const Value& value, const std::string& errorMessage) const;
    std::string expectString(const Value& value, const std::string& errorMessage) const;
    void expectArity(const std::vector<Value>& args, size_t expected, const std::string& name) const;
    void fireEvent(const std::string& name, const std::vector<Value>& args = {});
    bool isTruthyPublic(const Value& value) const;
    std::string valueToStringPublic(const Value& value) const;

private:
    Value visit(ASTNode* node);
    Value visitExpr(ASTNode* node);
    void visitPrint(PrintNode* node);
    void executeBlock(const std::vector<std::unique_ptr<ASTNode>>& statements, bool newScope);

    bool isTruthy(const Value& value) const;
    bool valuesEqual(const Value& a, const Value& b) const;
    std::string valueToString(const Value& value) const;
    std::string valueKeyString(const Value& value) const;

    Value getVariable(const std::string& name) const;
    void setVariable(const std::string& name, const Value& value);

    std::unordered_map<std::string, std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>> modules;
    std::unordered_map<std::string, FunctionDef> functions;
    std::unordered_map<std::string, std::vector<EventDef>> events;
    std::vector<std::unordered_map<std::string, Value>> scopes;

    int nextTaskId = 1;
    int nextChannelId = 1;
    std::unordered_map<int, std::shared_future<Value>> tasks;
    std::unordered_map<int, std::deque<Value>> channels;
    std::mutex asyncMutex;
};

#endif
