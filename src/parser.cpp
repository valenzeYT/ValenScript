#include "../include/parser.h"
#include <unordered_map>
#include <stdexcept>

Parser::Parser(Lexer& l) : lexer(l) {
    currentToken = lexer.getNextToken();
}

void Parser::skipNewlines() {
    while (currentToken.type == TokenType::NEWLINE) {
        eat(TokenType::NEWLINE);
    }
}

void Parser::eat(TokenType type) {
    if (currentToken.type != type) {
        throw std::runtime_error("Unexpected token: " + currentToken.value);
    }
    currentToken = lexer.getNextToken();
}

bool Parser::isExpressionStart(TokenType type) const {
    return type == TokenType::MINUS ||
           type == TokenType::NOT ||
           type == TokenType::NUMBER ||
           type == TokenType::STRING ||
           type == TokenType::LBRACKET ||
           type == TokenType::LBRACE ||
           type == TokenType::TRUE_LITERAL ||
           type == TokenType::FALSE_LITERAL ||
           type == TokenType::IDENTIFIER ||
           type == TokenType::TYPE;
}

bool Parser::isSpaceArgStart(TokenType type) const {
    return type == TokenType::NUMBER ||
           type == TokenType::STRING ||
           type == TokenType::LBRACKET ||
           type == TokenType::LBRACE ||
           type == TokenType::TRUE_LITERAL ||
           type == TokenType::FALSE_LITERAL ||
           type == TokenType::IDENTIFIER ||
           type == TokenType::TYPE;
}

int Parser::fixedImplicitArity(const std::string& name) const {
    static const std::unordered_map<std::string, int> fixedArity = {
        {"CHANNEL_CREATE", 0},
        {"NUM", 1},
        {"BOOL", 1},
        {"TYPE", 1},
        {"LEN", 1}
    };
    auto it = fixedArity.find(name);
    if (it == fixedArity.end()) {
        return -1;
    }
    return it->second;
}

std::unique_ptr<ASTNode> Parser::statement() {
    skipNewlines();

    if (currentToken.type == TokenType::PRINT) {
        eat(TokenType::PRINT);
        std::vector<std::unique_ptr<ASTNode>> args;
        if (currentToken.type == TokenType::LBRACKET) {
            args = parseArgList();
        } else if (isExpressionStart(currentToken.type)) {
            args.push_back(expr());
        } else {
            throw std::runtime_error("PRINT expects a value");
        }
        return std::make_unique<PrintNode>(std::move(args));
    }

    if (currentToken.type == TokenType::IMPORT) {
        eat(TokenType::IMPORT);
        Token lib = currentToken;
        eat(TokenType::IDENTIFIER);
        std::string alias;
        if (currentToken.type == TokenType::IDENTIFIER &&
            (currentToken.value == "AS" || currentToken.value == "as")) {
            eat(TokenType::IDENTIFIER);
            Token a = currentToken;
            eat(TokenType::IDENTIFIER);
            alias = a.value;
        }
        return std::make_unique<ImportNode>(lib.value, alias);
    }

    if (currentToken.type == TokenType::WHEN) {
        eat(TokenType::WHEN);
        if (currentToken.type != TokenType::IDENTIFIER && currentToken.type != TokenType::TYPE) {
            throw std::runtime_error("WHEN expects event name");
        }

        std::string eventName = currentToken.value;
        eat(currentToken.type);

        while (currentToken.type == TokenType::DOT) {
            eat(TokenType::DOT);
            if (currentToken.type != TokenType::IDENTIFIER && currentToken.type != TokenType::TYPE) {
                throw std::runtime_error("WHEN expects dotted event name segment");
            }
            eventName += ".";
            eventName += currentToken.value;
            eat(currentToken.type);
        }

        std::vector<std::string> params;
        while (currentToken.type == TokenType::IDENTIFIER || currentToken.type == TokenType::TYPE) {
            params.push_back(currentToken.value);
            eat(currentToken.type);
        }

        auto body = suite();
        return std::make_unique<EventDefNode>(eventName, std::move(params), std::move(body));
    }

    if (currentToken.type == TokenType::IF) {
        eat(TokenType::IF);
        auto cond = expr();
        auto thenBody = suite();
        std::vector<std::unique_ptr<ASTNode>> elseBody;
        if (currentToken.type == TokenType::ELSE) {
            eat(TokenType::ELSE);
            elseBody = suite();
        }
        return std::make_unique<IfNode>(std::move(cond), std::move(thenBody), std::move(elseBody));
    }

    if (currentToken.type == TokenType::WHILE) {
        eat(TokenType::WHILE);
        auto condition = expr();
        auto body = suite();
        return std::make_unique<WhileNode>(std::move(condition), std::move(body));
    }

    if (currentToken.type == TokenType::REPEAT) {
        eat(TokenType::REPEAT);
        auto body = suite();
        eat(TokenType::TIMES);
        auto countExpr = expr();
        return std::make_unique<RepeatNode>(std::move(body), std::move(countExpr));
    }

    if (currentToken.type == TokenType::FOR) {
        eat(TokenType::FOR);
        Token var = currentToken;
        eat(TokenType::IDENTIFIER);
        eat(TokenType::ASSIGN);
        auto start = expr();
        eat(TokenType::TO);
        auto end = expr();
        auto body = suite();
        return std::make_unique<ForNode>(var.value, std::move(start), std::move(end), std::move(body));
    }

    if (currentToken.type == TokenType::FUNC) {
        eat(TokenType::FUNC);
        Token name = currentToken;
        eat(TokenType::IDENTIFIER);
        std::vector<std::string> params;
        if (currentToken.type == TokenType::LBRACKET) {
            eat(TokenType::LBRACKET);
            if (currentToken.type != TokenType::RBRACKET) {
                Token param = currentToken;
                eat(TokenType::IDENTIFIER);
                params.push_back(param.value);
                while (currentToken.type == TokenType::COMMA) {
                    eat(TokenType::COMMA);
                    Token p = currentToken;
                    eat(TokenType::IDENTIFIER);
                    params.push_back(p.value);
                }
            }
            eat(TokenType::RBRACKET);
        } else {
            while (currentToken.type == TokenType::IDENTIFIER) {
                Token p = currentToken;
                eat(TokenType::IDENTIFIER);
                params.push_back(p.value);
            }
        }
        auto body = suite();
        return std::make_unique<FunctionDefNode>(name.value, std::move(params), std::move(body));
    }

    if (currentToken.type == TokenType::RETURN) {
        eat(TokenType::RETURN);
        if (!isExpressionStart(currentToken.type)) {
            return std::make_unique<ReturnNode>(std::make_unique<NumberNode>(0.0));
        }
        return std::make_unique<ReturnNode>(expr());
    }

    if (currentToken.type == TokenType::BREAK) {
        eat(TokenType::BREAK);
        return std::make_unique<BreakNode>();
    }

    if (currentToken.type == TokenType::CONTINUE) {
        eat(TokenType::CONTINUE);
        return std::make_unique<ContinueNode>();
    }

    if (currentToken.type == TokenType::WAIT) {
        eat(TokenType::WAIT);
        std::unique_ptr<ASTNode> seconds;
        if (currentToken.type == TokenType::LBRACKET) {
            eat(TokenType::LBRACKET);
            seconds = expr();
            eat(TokenType::RBRACKET);
        } else {
            seconds = expr();
        }
        return std::make_unique<WaitNode>(std::move(seconds));
    }

    if (isExpressionStart(currentToken.type)) {
        auto left = expr();
        if (currentToken.type == TokenType::ASSIGN) {
            eat(TokenType::ASSIGN);
            return std::make_unique<AssignNode>(std::move(left), expr());
        }
        return std::make_unique<ExprStmtNode>(std::move(left));
    }

    throw std::runtime_error("Unknown statement: " + currentToken.value);
}

std::vector<std::unique_ptr<ASTNode>> Parser::parse() {
    std::vector<std::unique_ptr<ASTNode>> stmts;
    skipNewlines();
    while (currentToken.type != TokenType::END_OF_FILE) {
        if (currentToken.type == TokenType::INDENT || currentToken.type == TokenType::DEDENT) {
            eat(currentToken.type);
            skipNewlines();
            continue;
        }
        stmts.push_back(statement());
        skipNewlines();
    }
    return stmts;
}

std::vector<std::unique_ptr<ASTNode>> Parser::block() {
    eat(TokenType::LBRACE);
    std::vector<std::unique_ptr<ASTNode>> stmts;
    skipNewlines();
    while (currentToken.type != TokenType::RBRACE) {
        if (currentToken.type == TokenType::END_OF_FILE) {
            throw std::runtime_error("Unterminated block: missing '}'");
        }
        stmts.push_back(statement());
        skipNewlines();
    }
    eat(TokenType::RBRACE);
    return stmts;
}

std::vector<std::unique_ptr<ASTNode>> Parser::suite() {
    if (currentToken.type == TokenType::LBRACE) {
        return block();
    }

    if (currentToken.type != TokenType::NEWLINE) {
        throw std::runtime_error("Expected newline before indented block");
    }
    eat(TokenType::NEWLINE);
    skipNewlines();
    eat(TokenType::INDENT);

    std::vector<std::unique_ptr<ASTNode>> stmts;
    skipNewlines();
    while (currentToken.type != TokenType::DEDENT &&
           currentToken.type != TokenType::END_OF_FILE) {
        stmts.push_back(statement());
        skipNewlines();
    }
    if (currentToken.type == TokenType::DEDENT) {
        eat(TokenType::DEDENT);
    }
    return stmts;
}

std::unique_ptr<ASTNode> Parser::expr() { return logicalOr(); }

std::unique_ptr<ASTNode> Parser::logicalOr() {
    auto node = logicalAnd();
    while (currentToken.type == TokenType::OR) {
        Token op = currentToken;
        eat(TokenType::OR);
        node = std::make_unique<BinOpNode>(std::move(node), op, logicalAnd());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::logicalAnd() {
    auto node = equality();
    while (currentToken.type == TokenType::AND) {
        Token op = currentToken;
        eat(TokenType::AND);
        node = std::make_unique<BinOpNode>(std::move(node), op, equality());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::equality() {
    auto node = comparison();
    while (currentToken.type == TokenType::EQ || currentToken.type == TokenType::NE) {
        Token op = currentToken;
        eat(op.type);
        node = std::make_unique<BinOpNode>(std::move(node), op, comparison());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::comparison() {
    auto node = additive();
    while (!parsingIndexExpr &&
           (currentToken.type == TokenType::LT ||
            currentToken.type == TokenType::GT ||
            currentToken.type == TokenType::LE ||
            currentToken.type == TokenType::GE)) {
        Token op = currentToken;
        eat(op.type);
        node = std::make_unique<BinOpNode>(std::move(node), op, additive());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::additive() {
    auto node = term();
    auto isChainOpToken = [](TokenType t) {
        return t == TokenType::PLUS || t == TokenType::MINUS ||
               t == TokenType::MUL || t == TokenType::DIV;
    };
    while (true) {
        if (currentToken.type == TokenType::PLUS || currentToken.type == TokenType::MINUS) {
            Token op = currentToken;
            Token next = lexer.peekNextToken();
            bool isRepeatedPostfixMinus = (op.type == TokenType::MINUS && next.type == TokenType::MINUS);
            if (!isExpressionStart(next.type) || isRepeatedPostfixMinus || isChainOpToken(next.type)) {
                int count = 0;
                while (currentToken.type == op.type) {
                    eat(op.type);
                    ++count;
                }
                node = std::make_unique<BinOpNode>(
                    std::move(node),
                    op,
                    std::make_unique<NumberNode>(static_cast<double>(count))
                );
                continue;
            }
            eat(op.type);
            node = std::make_unique<BinOpNode>(std::move(node), op, term());
            continue;
        }

        // Allow mixed postfix operator chains to continue across precedence boundaries.
        if (currentToken.type == TokenType::MUL || currentToken.type == TokenType::DIV) {
            Token op = currentToken;
            Token next = lexer.peekNextToken();
            if (!isExpressionStart(next.type) || isChainOpToken(next.type)) {
                int count = 0;
                while (currentToken.type == op.type) {
                    eat(op.type);
                    ++count;
                }
                node = std::make_unique<BinOpNode>(
                    std::move(node),
                    op,
                    std::make_unique<NumberNode>(static_cast<double>(count))
                );
                continue;
            }
        }
        break;
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::term() {
    auto node = unary();
    auto isChainOpToken = [](TokenType t) {
        return t == TokenType::PLUS || t == TokenType::MINUS ||
               t == TokenType::MUL || t == TokenType::DIV;
    };
    while (currentToken.type == TokenType::MUL ||
           currentToken.type == TokenType::DIV ||
           currentToken.type == TokenType::MOD) {
        Token op = currentToken;
        if ((op.type == TokenType::MUL || op.type == TokenType::DIV) &&
            (!isExpressionStart(lexer.peekNextToken().type) || isChainOpToken(lexer.peekNextToken().type))) {
            int count = 0;
            while (currentToken.type == op.type) {
                eat(op.type);
                ++count;
            }
            node = std::make_unique<BinOpNode>(
                std::move(node),
                op,
                std::make_unique<NumberNode>(static_cast<double>(count))
            );
            continue;
        }
        eat(op.type);
        node = std::make_unique<BinOpNode>(std::move(node), op, unary());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::unary() {
    if (currentToken.type == TokenType::MINUS || currentToken.type == TokenType::NOT) {
        Token op = currentToken;
        eat(op.type);
        return std::make_unique<UnaryOpNode>(op, unary());
    }
    return factor();
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseArgList() {
    eat(TokenType::LBRACKET);
    std::vector<std::unique_ptr<ASTNode>> args;
    if (currentToken.type != TokenType::RBRACKET) {
        args.push_back(expr());
        while (currentToken.type == TokenType::COMMA) {
            eat(TokenType::COMMA);
            args.push_back(expr());
        }
    }
    eat(TokenType::RBRACKET);
    return args;
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseSpaceArgs() {
    std::vector<std::unique_ptr<ASTNode>> args;
    while (isSpaceArgStart(currentToken.type)) {
        args.push_back(parseSpaceArg());
    }
    return args;
}

std::unique_ptr<ASTNode> Parser::parseSpaceArg() {
    if (currentToken.type == TokenType::IDENTIFIER || currentToken.type == TokenType::TYPE) {
        std::vector<Token> lookahead = lexer.peekTokens(2);
        if (!lookahead.empty() && lookahead[0].type == TokenType::DOT) {
            return unary();
        }
        if (fixedImplicitArity(currentToken.value) >= 0) {
            return unary();
        }
    }

    bool savedAllowImplicitCalls = allowImplicitCalls;
    allowImplicitCalls = false;
    auto node = unary();
    allowImplicitCalls = savedAllowImplicitCalls;
    return node;
}

std::unique_ptr<ASTNode> Parser::factor() {
    Token token = currentToken;
    auto applyPostfix = [this](std::unique_ptr<ASTNode> node) {
        while (true) {
            // Use <<index>> to avoid ambiguity with comparison operators.
            if (currentToken.type == TokenType::LT && lexer.peekNextToken().type == TokenType::LT) {
                eat(TokenType::LT);
                eat(TokenType::LT);
                bool wasParsingIndexExpr = parsingIndexExpr;
                parsingIndexExpr = true;
                auto idx = expr();
                parsingIndexExpr = wasParsingIndexExpr;
                eat(TokenType::GT);
                eat(TokenType::GT);
                node = std::make_unique<IndexNode>(std::move(node), std::move(idx));
                continue;
            }
            break;
        }
        return node;
    };

    if (token.type == TokenType::NUMBER) {
        eat(TokenType::NUMBER);
        return applyPostfix(std::make_unique<NumberNode>(std::stod(token.value)));
    }

    if (token.type == TokenType::STRING) {
        eat(TokenType::STRING);
        return applyPostfix(std::make_unique<StringNode>(token.value));
    }

    if (token.type == TokenType::TRUE_LITERAL) {
        eat(TokenType::TRUE_LITERAL);
        return applyPostfix(std::make_unique<BoolNode>(true));
    }

    if (token.type == TokenType::FALSE_LITERAL) {
        eat(TokenType::FALSE_LITERAL);
        return applyPostfix(std::make_unique<BoolNode>(false));
    }

    if (token.type == TokenType::IDENTIFIER || token.type == TokenType::TYPE) {
        Token name = token;
        eat(token.type);

        std::unique_ptr<ASTNode> node;

        if (currentToken.type == TokenType::DOT) {
            eat(TokenType::DOT);
            Token func = currentToken;
            if (currentToken.type != TokenType::IDENTIFIER &&
                currentToken.type != TokenType::TYPE &&
                currentToken.type != TokenType::PRINT &&
                currentToken.type != TokenType::IMPORT &&
                currentToken.type != TokenType::WHEN &&
                currentToken.type != TokenType::WAIT &&
                currentToken.type != TokenType::WHILE &&
                currentToken.type != TokenType::IF &&
                currentToken.type != TokenType::ELSE &&
                currentToken.type != TokenType::REPEAT &&
                currentToken.type != TokenType::TIMES &&
                currentToken.type != TokenType::FOR &&
                currentToken.type != TokenType::TO &&
                currentToken.type != TokenType::FUNC &&
                currentToken.type != TokenType::RETURN &&
                currentToken.type != TokenType::BREAK &&
                currentToken.type != TokenType::CONTINUE &&
                currentToken.type != TokenType::AND &&
                currentToken.type != TokenType::OR &&
                currentToken.type != TokenType::NOT &&
                currentToken.type != TokenType::TRUE_LITERAL &&
                currentToken.type != TokenType::FALSE_LITERAL) {
                throw std::runtime_error("Expected function name after '.', got: " + currentToken.value);
            }
            eat(currentToken.type);
            std::vector<std::unique_ptr<ASTNode>> args;
            if (currentToken.type == TokenType::LBRACKET) {
                args = parseArgList();
            } else {
                args = parseSpaceArgs();
            }
            node = std::make_unique<ModuleFuncNode>(name.value, func.value, std::move(args));
            return applyPostfix(std::move(node));
        }

        if (currentToken.type == TokenType::LBRACKET) {
            auto args = parseArgList();
            node = std::make_unique<FunctionCallNode>(name.value, std::move(args));
            return applyPostfix(std::move(node));
        }
        if (allowImplicitCalls && isSpaceArgStart(currentToken.type)) {
            std::vector<std::unique_ptr<ASTNode>> args;
            int arity = fixedImplicitArity(name.value);
            if (arity >= 0) {
                for (int i = 0; i < arity; ++i) {
                    if (!isSpaceArgStart(currentToken.type)) {
                        throw std::runtime_error("Not enough arguments for " + name.value);
                    }
                    args.push_back(parseSpaceArg());
                }
            } else {
                args = parseSpaceArgs();
            }
            if (!args.empty()) {
                node = std::make_unique<FunctionCallNode>(name.value, std::move(args));
                return applyPostfix(std::move(node));
            }
        }

        if (allowImplicitCalls && fixedImplicitArity(name.value) == 0) {
            node = std::make_unique<FunctionCallNode>(name.value, std::vector<std::unique_ptr<ASTNode>>{});
            return applyPostfix(std::move(node));
        }

        node = std::make_unique<VarNode>(name.value);
        return applyPostfix(std::move(node));
    }

    if (token.type == TokenType::LBRACKET) {
        eat(TokenType::LBRACKET);
        if (currentToken.type == TokenType::RBRACKET) {
            eat(TokenType::RBRACKET);
            return applyPostfix(std::make_unique<ListNode>(std::vector<std::unique_ptr<ASTNode>>{}));
        }

        auto first = expr();
        if (currentToken.type == TokenType::PIPE) {
            std::vector<std::unique_ptr<ASTNode>> items;
            items.push_back(std::move(first));
            while (currentToken.type == TokenType::PIPE) {
                eat(TokenType::PIPE);
                items.push_back(expr());
            }
            eat(TokenType::RBRACKET);
            return applyPostfix(std::make_unique<ListNode>(std::move(items)));
        }

        eat(TokenType::RBRACKET);
        return applyPostfix(std::move(first));
    }

    if (token.type == TokenType::LBRACE) {
        eat(TokenType::LBRACE);
        std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> items;
        if (currentToken.type != TokenType::RBRACE) {
            auto key = expr();
            eat(TokenType::COLON);
            auto value = expr();
            items.push_back({std::move(key), std::move(value)});
            while (currentToken.type == TokenType::PIPE) {
                eat(TokenType::PIPE);
                key = expr();
                eat(TokenType::COLON);
                value = expr();
                items.push_back({std::move(key), std::move(value)});
            }
        }
        eat(TokenType::RBRACE);
        return applyPostfix(std::make_unique<MapNode>(std::move(items)));
    }

    throw std::runtime_error("Expected factor, got: " + token.value);
}
