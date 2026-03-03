#include "../include/lexer.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

Lexer::Lexer(const std::string& t) : text(t), pos(0) {
    currentChar = pos < text.size() ? text[pos] : '\0';
    indentStack.push_back(0);
    atLineStart = true;
    emittedEofDedents = false;
}

void Lexer::advance() {
    pos++;
    currentChar = pos < text.size() ? text[pos] : '\0';
}

void Lexer::skipWhitespace() {
    while (currentChar &&
           std::isspace(static_cast<unsigned char>(currentChar)) &&
           currentChar != '\n' &&
           currentChar != '\r')
        advance();
}

int Lexer::consumeIndentation() {
    int indent = 0;
    while (currentChar == ' ' || currentChar == '\t') {
        if (currentChar == '\t') indent += 4;
        else indent += 1;
        advance();
    }
    return indent;
}

char Lexer::peekChar() const {
    size_t nextPos = pos + 1;
    return nextPos < text.size() ? text[nextPos] : '\0';
}

Token Lexer::number() {
    std::string result;
    bool hasDot = false;
    bool hasExp = false;
    bool sawDigit = false;

    while (currentChar) {
        if (std::isdigit(static_cast<unsigned char>(currentChar))) {
            result += currentChar;
            sawDigit = true;
            advance();
            continue;
        }

        if (currentChar == '_') {
            if (!sawDigit || !std::isdigit(static_cast<unsigned char>(peekChar()))) {
                throw std::runtime_error("Invalid numeric literal");
            }
            advance();
            continue;
        }

        if (currentChar == '.') {
            if (hasDot || hasExp) break;
            hasDot = true;
            result += currentChar;
            advance();
            continue;
        }

        if ((currentChar == 'e' || currentChar == 'E') && sawDigit && !hasExp) {
            hasExp = true;
            result += 'e';
            advance();
            if (currentChar == '+' || currentChar == '-') {
                result += currentChar;
                advance();
            }
            if (!std::isdigit(static_cast<unsigned char>(currentChar))) {
                throw std::runtime_error("Invalid exponent in numeric literal");
            }
            continue;
        }
        break;
    }

    return { TokenType::NUMBER, result };
}

Token Lexer::string() {
    advance(); // skip opening quote
    std::string result;

    while (currentChar && currentChar != '"') {
        if (currentChar == '\\') {
            advance();
            if (!currentChar) {
                throw std::runtime_error("Unterminated escape sequence");
            }
            switch (currentChar) {
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default:
                    throw std::runtime_error(std::string("Unknown escape sequence: \\") + currentChar);
            }
            advance();
            continue;
        }
        result += currentChar;
        advance();
    }

    if (currentChar != '"')
        throw std::runtime_error("Unterminated string");

    advance(); // skip closing quote
    return { TokenType::STRING, result };
}

Token Lexer::identifier() {
    std::string result;

    while (currentChar && (std::isalnum(static_cast<unsigned char>(currentChar)) || currentChar == '_')) {
        result += currentChar;
        advance();
    }

    if (result == "PRINT")  return { TokenType::PRINT, result };
    if (result == "WAIT")   return { TokenType::WAIT, result };
    if (result == "WHILE")  return { TokenType::WHILE, result };
    if (result == "IF")     return { TokenType::IF, result };
    if (result == "ELSE")   return { TokenType::ELSE, result };
    if (result == "REPEAT") return { TokenType::REPEAT, result };
    if (result == "TIMES")  return { TokenType::TIMES, result };
    if (result == "FOR")    return { TokenType::FOR, result };
    if (result == "TO")     return { TokenType::TO, result };
    if (result == "FUNC")   return { TokenType::FUNC, result };
    if (result == "RETURN") return { TokenType::RETURN, result };
    if (result == "BREAK")  return { TokenType::BREAK, result };
    if (result == "CONTINUE") return { TokenType::CONTINUE, result };
    if (result == "AND")    return { TokenType::AND, result };
    if (result == "OR")     return { TokenType::OR, result };
    if (result == "NOT")    return { TokenType::NOT, result };
    if (result == "TRUE")   return { TokenType::TRUE_LITERAL, result };
    if (result == "FALSE")  return { TokenType::FALSE_LITERAL, result };
    if (result == "TYPE")   return { TokenType::TYPE, result };
    if (result == "WHEN")   return { TokenType::WHEN, result };
    if (result == "IMPORT") return { TokenType::IMPORT, result };

    return { TokenType::IDENTIFIER, result };
}

Token Lexer::getNextToken() {
    if (!pendingTokens.empty()) {
        Token t = pendingTokens.front();
        pendingTokens.pop_front();
        return t;
    }

    while (currentChar) {
        if (atLineStart) {
            int indent = consumeIndentation();

            if (currentChar == ';') {
                while (currentChar && currentChar != '\n' && currentChar != '\r') {
                    advance();
                }
            }

            if (currentChar == '\r') {
                advance();
                if (currentChar == '\n') advance();
                atLineStart = true;
                return { TokenType::NEWLINE, "\\n" };
            }
            if (currentChar == '\n') {
                advance();
                atLineStart = true;
                return { TokenType::NEWLINE, "\\n" };
            }

            int currentIndent = indentStack.back();
            if (indent > currentIndent) {
                indentStack.push_back(indent);
                atLineStart = false;
                return { TokenType::INDENT, "<INDENT>" };
            }
            if (indent < currentIndent) {
                while (indent < indentStack.back()) {
                    indentStack.pop_back();
                    pendingTokens.push_back({ TokenType::DEDENT, "<DEDENT>" });
                }
                if (indent != indentStack.back()) {
                    size_t line = 1, col = 1;
                    for (size_t i = 0; i < pos; ++i) {
                        if (text[i] == '\n') {
                            ++line;
                            col = 1;
                        } else {
                            ++col;
                        }
                    }
                    throw std::runtime_error(
                        "Invalid dedent level at line " + std::to_string(line) +
                        ", column " + std::to_string(col)
                    );
                }
                if (!pendingTokens.empty()) {
                    atLineStart = false;
                    Token t = pendingTokens.front();
                    pendingTokens.pop_front();
                    return t;
                }
            }
            atLineStart = false;
        }

        if (currentChar == '\r') {
            advance();
            if (currentChar == '\n') advance();
            atLineStart = true;
            return { TokenType::NEWLINE, "\\n" };
        }
        if (currentChar == '\n') {
            advance();
            atLineStart = true;
            return { TokenType::NEWLINE, "\\n" };
        }

        if (std::isspace(static_cast<unsigned char>(currentChar))) {
            skipWhitespace();
            continue;
        }

        if (currentChar == ';') {
            while (currentChar && currentChar != '\n' && currentChar != '\r') {
                advance();
            }
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(currentChar)))
            return number();

        if (std::isalpha(static_cast<unsigned char>(currentChar)) || currentChar == '_')
            return identifier();

        if (currentChar == '"')
            return string();

        if (currentChar == '+') { advance(); return { TokenType::PLUS, "+" }; }
        if (currentChar == '-') { advance(); return { TokenType::MINUS, "-" }; }
        if (currentChar == '*') { advance(); return { TokenType::MUL, "*" }; }
        if (currentChar == '/') { advance(); return { TokenType::DIV, "/" }; }
        if (currentChar == '%') { advance(); return { TokenType::MOD, "%" }; }
        if (currentChar == '=') {
            if (peekChar() == '=') {
                advance();
                advance();
                return { TokenType::EQ, "==" };
            }
            advance();
            return { TokenType::ASSIGN, "=" };
        }
        if (currentChar == '!') {
            if (peekChar() == '=') {
                advance();
                advance();
                return { TokenType::NE, "!=" };
            }
            throw std::runtime_error("Unknown character: !");
        }
        if (currentChar == '<') {
            if (peekChar() == '=') {
                advance();
                advance();
                return { TokenType::LE, "<=" };
            }
            advance();
            return { TokenType::LT, "<" };
        }
        if (currentChar == '>') {
            if (peekChar() == '=') {
                advance();
                advance();
                return { TokenType::GE, ">=" };
            }
            advance();
            return { TokenType::GT, ">" };
        }

        if (currentChar == '[') { advance(); return { TokenType::LBRACKET, "[" }; }
        if (currentChar == ']') { advance(); return { TokenType::RBRACKET, "]" }; }
        if (currentChar == '{') { advance(); return { TokenType::LBRACE, "{" }; }
        if (currentChar == '}') { advance(); return { TokenType::RBRACE, "}" }; }
        if (currentChar == '.') { advance(); return { TokenType::DOT, "." }; }
        if (currentChar == '|') { advance(); return { TokenType::PIPE, "|" }; }
        if (currentChar == ':') { advance(); return { TokenType::COLON, ":" }; }
        if (currentChar == ',') { advance(); return { TokenType::COMMA, "," }; }

        {
            // Debug output for unknown character
            size_t line = 1, col = 1;
            for (size_t i = 0; i < pos; ++i) {
                if (text[i] == '\n') {
                    ++line;
                    col = 1;
                } else {
                    ++col;
                }
            }
            std::cerr << "Unknown character '" << currentChar << "' at line " << line << ", column " << col << std::endl;
            throw std::runtime_error(std::string("Unknown character: ") + currentChar);
        }
    }

    if (!emittedEofDedents && indentStack.size() > 1) {
        indentStack.pop_back();
        return { TokenType::DEDENT, "<DEDENT>" };
    }
    emittedEofDedents = true;
    return { TokenType::END_OF_FILE, "" };
}

Token Lexer::peekNextToken() {
    size_t savedPos = pos;
    char savedCurrentChar = currentChar;
    bool savedAtLineStart = atLineStart;
    std::vector<int> savedIndentStack = indentStack;
    std::deque<Token> savedPendingTokens = pendingTokens;
    bool savedEmittedEofDedents = emittedEofDedents;
    Token next = getNextToken();
    pos = savedPos;
    currentChar = savedCurrentChar;
    atLineStart = savedAtLineStart;
    indentStack = std::move(savedIndentStack);
    pendingTokens = std::move(savedPendingTokens);
    emittedEofDedents = savedEmittedEofDedents;
    return next;
}

std::vector<Token> Lexer::peekTokens(size_t count) {
    size_t savedPos = pos;
    char savedCurrentChar = currentChar;
    bool savedAtLineStart = atLineStart;
    std::vector<int> savedIndentStack = indentStack;
    std::deque<Token> savedPendingTokens = pendingTokens;
    bool savedEmittedEofDedents = emittedEofDedents;

    std::vector<Token> tokens;
    tokens.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        tokens.push_back(getNextToken());
    }

    pos = savedPos;
    currentChar = savedCurrentChar;
    atLineStart = savedAtLineStart;
    indentStack = std::move(savedIndentStack);
    pendingTokens = std::move(savedPendingTokens);
    emittedEofDedents = savedEmittedEofDedents;
    return tokens;
}
