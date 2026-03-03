#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <cctype>
#include <deque>
#include <vector>

enum class TokenType {
    PLUS,
    MINUS,
    MUL,
    DIV,
    MOD,
    ASSIGN,
    EQ,
    NE,
    LT,
    GT,
    LE,
    GE,

    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    DOT,
    PIPE,
    COLON,
    COMMA,

    NUMBER,
    STRING,
    IDENTIFIER,

    PRINT,
    WAIT,
    WHEN,
    WHILE,
    IF,
    ELSE,
    REPEAT,
    TIMES,
    FOR,
    TO,
    FUNC,
    RETURN,
    BREAK,
    CONTINUE,
    AND,
    OR,
    NOT,
    TRUE_LITERAL,
    FALSE_LITERAL,
    TYPE,
    IMPORT,
    NEWLINE,
    INDENT,
    DEDENT,

    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
public:
    Lexer(const std::string& text);
    Token getNextToken();
    Token peekNextToken();
    std::vector<Token> peekTokens(size_t count);

private:
    std::string text;
    size_t pos;
    char currentChar;
    bool atLineStart = true;
    std::vector<int> indentStack;
    std::deque<Token> pendingTokens;
    bool emittedEofDedents = false;

    void advance();
    void skipWhitespace();
    int consumeIndentation();
    Token number();
    Token identifier();
    Token string();
    char peekChar() const;
};

#endif
