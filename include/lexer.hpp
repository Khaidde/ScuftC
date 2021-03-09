#pragma once

#include <memory>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

#include "diagnostics.hpp"
#include "flags.hpp"

enum class TokenType : unsigned char {
    UNKNOWN,

    // Blocks
    LEFT_CURLY,
    RIGHT_CURLY,
    LEFT_PARENS,
    RIGHT_PARENS,

    // Statement Keywords
    MOD,  // testMod = mod {}
    TY,   // particle = ty {}
    IF,
    ELSE,
    WHILE,
    FOR,
    IN,
    BREAK,
    CONTINUE,

    // Declarations
    IDENTIFIER,
    COLON,

    // Types
    VOID_TYPE,
    MOD_TYPE,  // spaceMod: module = mod { }
    TY_TYPE,   // thing: type = typeof( Particle )
    INT_TYPE,
    DOUBLE_TYPE,
    STRING_TYPE,
    BOOL_TYPE,

    // Assignments
    ASSIGNMENT,
    CONST_ASSIGNMENT,

    // Literal values
    INT_LITERAL,
    DOUBLE_LITERAL,
    STRING_LITERAL,
    TRUE,
    FALSE,

    // Conditionals
    COND_NOT,            // !
    COND_OR,             // ||
    COND_AND,            // &&
    COND_XOR,            // $$
    COND_EQUALS,         // ==
    COND_NOT_EQUALS,     // !=
    COND_LESS,           // <
    COND_LESS_EQUAL,     // <=
    COND_GREATER,        // >
    COND_GREATER_EQUAL,  // >=

    // Bitwise Operators
    BIT_NOT,          // ~
    BIT_OR,           // |
    BIT_AND,          // &
    BIT_XOR,          // $
    BIT_SHIFT_LEFT,   // <<
    BIT_SHIFT_RIGHT,  // >>

    // Operators
    DOT,
    OP_ADD,
    OP_SUBTR,
    OP_MULT,
    OP_DIV,
    OP_MOD,
    OP_CARROT,  // Exponents
    OP_ADD_ADD,
    OP_ADD_EQUAL,
    OP_SUBTR_SUBTR,
    OP_SUBTR_EQUAL,
    OP_MULT_EQUAL,
    OP_DIV_EQUAL,
    OP_MOD_EQUAL,

    // Function Defs
    ARROW,          // () "->" void
    SINGLE_RETURN,  // add = (a: int, b: int) :: a + b
    RETURN,

    // Miscellaneous
    COMMA,
    DEREF,  // Particle.*
    END,
};

std::string tokenTypeToStr(TokenType type);

struct Token {
    TokenType type;

    int index;
    int cLen;  // character length of the token
    std::string* src;

    union {
        long long longVal;
        double doubleVal;
    };

    inline std::string toStr() { return src->substr(index, cLen); }
};

class Diagnostics;

class Lexer {
    Diagnostics& DX;
    Flags flags;

    int curIndex = 0;
    int curCLen = 0;

    int cacheIndex = 0;
    std::vector<std::unique_ptr<Token>> tokenCache;

   public:
    static const size_t TAB_WIDTH = 4;

    Lexer(Diagnostics& diagnostics, Flags flags) : DX(diagnostics), flags(flags) { this->fromFilePath(flags.filePath); }

    std::string sourceStr;
    void fromFilePath(const char* filePath);

    std::unique_ptr<Token> makeToken(TokenType type);
    std::unique_ptr<Token> consumeToken();

    Token* peekToken();
    Token* nextToken();

    Token* lastToken();

    inline bool isCursorChar(char assertChar) { return sourceStr[curIndex + curCLen] == assertChar; }

    static inline bool isWhitespace(char ch) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }
    static inline bool isLetter(char ch) {
        return (ch >= 65 && ch < 91) || (ch >= 97 && ch < 123) || ch == '\'' || ch == '_';
    }
    static inline int toNum(char ch) { return ch - 48; }
    static inline bool isNumber(char ch) { return (ch >= 48 && ch <= 57) || ch == '_'; }
    static inline bool isHex(char ch) { return (ch >= 65 && ch < 71) || (ch >= 97 && ch < 103) || isNumber(ch); }
};
