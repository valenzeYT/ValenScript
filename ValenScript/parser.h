#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct ASTNode { virtual ~ASTNode() = default; };

struct UnaryOpNode : ASTNode {
    Token op;
    std::unique_ptr<ASTNode> operand;
    UnaryOpNode(Token o, std::unique_ptr<ASTNode> opnd) : op(o), operand(std::move(opnd)) {}
};

struct NumberNode : ASTNode {
    double value;
    NumberNode(double v) : value(v) {}
};

struct BoolNode : ASTNode {
    bool value;
    BoolNode(bool v) : value(v) {}
};

struct StringNode : ASTNode {
    std::string value;
    StringNode(const std::string& v) : value(v) {}
};

struct ListNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> items;
    ListNode(std::vector<std::unique_ptr<ASTNode>> i) : items(std::move(i)) {}
};

struct MapNode : ASTNode {
    std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> items;
    MapNode(std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> i)
        : items(std::move(i)) {}
};

struct IndexNode : ASTNode {
    std::unique_ptr<ASTNode> target;
    std::unique_ptr<ASTNode> index;
    IndexNode(std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> i)
        : target(std::move(t)), index(std::move(i)) {}
};

struct BinOpNode : ASTNode {
    std::unique_ptr<ASTNode> left;
    Token op;
    std::unique_ptr<ASTNode> right;
    BinOpNode(std::unique_ptr<ASTNode> l, Token o, std::unique_ptr<ASTNode> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

struct PrintNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> args;
    PrintNode(std::vector<std::unique_ptr<ASTNode>> a) : args(std::move(a)) {}
};

struct ImportNode : ASTNode {
    std::string moduleName;
    std::string alias;
    ImportNode(const std::string& name, const std::string& a = "") : moduleName(name), alias(a) {}
};

struct IfNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> thenBody;
    std::vector<std::unique_ptr<ASTNode>> elseBody;
    IfNode(std::unique_ptr<ASTNode> c,
           std::vector<std::unique_ptr<ASTNode>> t,
           std::vector<std::unique_ptr<ASTNode>> e)
        : condition(std::move(c)), thenBody(std::move(t)), elseBody(std::move(e)) {}
};

struct WhileNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> body;
    WhileNode(std::unique_ptr<ASTNode> c, std::vector<std::unique_ptr<ASTNode>> b)
        : condition(std::move(c)), body(std::move(b)) {}
};

struct RepeatNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> body;
    std::unique_ptr<ASTNode> countExpr;
    RepeatNode(std::vector<std::unique_ptr<ASTNode>> b, std::unique_ptr<ASTNode> c)
        : body(std::move(b)), countExpr(std::move(c)) {}
};

struct ForNode : ASTNode {
    std::string variable;
    std::unique_ptr<ASTNode> startExpr;
    std::unique_ptr<ASTNode> endExpr;
    std::vector<std::unique_ptr<ASTNode>> body;
    ForNode(const std::string& v,
            std::unique_ptr<ASTNode> s,
            std::unique_ptr<ASTNode> e,
            std::vector<std::unique_ptr<ASTNode>> b)
        : variable(v), startExpr(std::move(s)), endExpr(std::move(e)), body(std::move(b)) {}
};

struct AssignNode : ASTNode {
    std::unique_ptr<ASTNode> target;
    std::unique_ptr<ASTNode> value;
    AssignNode(std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> v)
        : target(std::move(t)), value(std::move(v)) {}
};

struct VarNode : ASTNode {
    std::string name;
    VarNode(const std::string& n) : name(n) {}
};

struct WaitNode : ASTNode {
    std::unique_ptr<ASTNode> seconds;
    WaitNode(std::unique_ptr<ASTNode> s) : seconds(std::move(s)) {}
};

struct BreakNode : ASTNode {};
struct ContinueNode : ASTNode {};

struct ReturnNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    ReturnNode(std::unique_ptr<ASTNode> e) : expr(std::move(e)) {}
};

struct FunctionDefNode : ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<ASTNode>> body;
    FunctionDefNode(const std::string& n, std::vector<std::string> p, std::vector<std::unique_ptr<ASTNode>> b)
        : name(n), params(std::move(p)), body(std::move(b)) {}
};

struct EventDefNode : ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<ASTNode>> body;
    EventDefNode(const std::string& n, std::vector<std::string> p, std::vector<std::unique_ptr<ASTNode>> b)
        : name(n), params(std::move(p)), body(std::move(b)) {}
};

struct FunctionCallNode : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;
    FunctionCallNode(const std::string& n, std::vector<std::unique_ptr<ASTNode>> a)
        : name(n), args(std::move(a)) {}
};

struct ExprStmtNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    ExprStmtNode(std::unique_ptr<ASTNode> e) : expr(std::move(e)) {}
};

struct ModuleFuncNode : ASTNode {
    std::string moduleName;
    std::string funcName;
    std::vector<std::unique_ptr<ASTNode>> args;
    ModuleFuncNode(const std::string& mod, const std::string& func, std::vector<std::unique_ptr<ASTNode>> a)
        : moduleName(mod), funcName(func), args(std::move(a)) {}
};

class Parser {
public:
    Parser(Lexer& lexer);
    std::vector<std::unique_ptr<ASTNode>> parse();

private:
    Lexer& lexer;
    Token currentToken;
    bool parsingIndexExpr = false;
    bool allowImplicitCalls = true;

    void skipNewlines();
    void eat(TokenType type);
    bool isExpressionStart(TokenType type) const;
    bool isSpaceArgStart(TokenType type) const;
    int fixedImplicitArity(const std::string& name) const;
    std::unique_ptr<ASTNode> statement();
    std::vector<std::unique_ptr<ASTNode>> block();
    std::vector<std::unique_ptr<ASTNode>> suite();

    std::unique_ptr<ASTNode> expr();
    std::unique_ptr<ASTNode> logicalOr();
    std::unique_ptr<ASTNode> logicalAnd();
    std::unique_ptr<ASTNode> equality();
    std::unique_ptr<ASTNode> comparison();
    std::unique_ptr<ASTNode> additive();
    std::unique_ptr<ASTNode> term();
    std::unique_ptr<ASTNode> unary();
    std::unique_ptr<ASTNode> factor();
    std::unique_ptr<ASTNode> parseSpaceArg();

    std::vector<std::unique_ptr<ASTNode>> parseArgList();
    std::vector<std::unique_ptr<ASTNode>> parseSpaceArgs();
};

#endif
