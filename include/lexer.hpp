#pragma once

#include <memory>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

#include "diagnostics.hpp"
#include "flags.hpp"

enum TokenType : unsigned char {
    UNKNOWN_TKN,

    // Blocks
    LEFT_CURLY_TKN,
    RIGHT_CURLY_TKN,
    LEFT_PARENS_TKN,
    RIGHT_PARENS_TKN,

    // Statement Keywords
    MOD_TKN,  // testMod = mod {}
    TY_TKN,   // particle = ty {}
    IF_TKN,
    ELSE_TKN,
    WHILE_TKN,
    FOR_TKN,
    IN_TKN,
    BREAK_TKN,
    CONTINUE_TKN,

    // Declarations
    IDENTIFIER_TKN,
    COLON_TKN,

    // Types
    VOID_TYPE_TKN,
    MOD_TYPE_TKN,  // spaceMod: module = mod { }
    TY_TYPE_TKN,   // thing: type = typeof( Particle )
    INT_TYPE_TKN,
    DOUBLE_TYPE_TKN,
    STRING_TYPE_TKN,
    BOOL_TYPE_TKN,

    // Assignments
    ASSIGNMENT_TKN,
    CONST_ASSIGNMENT_TKN,

    // Literal values
    INT_LITERAL_TKN,
    DOUBLE_LITERAL_TKN,
    STRING_LITERAL_TKN,
    TRUE_TKN,
    FALSE_TKN,

    // Conditionals
    COND_NOT_TKN,            // !
    COND_OR_TKN,             // ||
    COND_AND_TKN,            // &&
    COND_XOR_TKN,            // $$
    COND_EQUALS_TKN,         // ==
    COND_NOT_EQUALS_TKN,     // !=
    COND_LESS_TKN,           // <
    COND_LESS_EQUAL_TKN,     // <=
    COND_GREATER_TKN,        // >
    COND_GREATER_EQUAL_TKN,  // >=

    // Bitwise Operators
    BIT_NOT_TKN,          // ~
    BIT_OR_TKN,           // |
    BIT_AND_TKN,          // &
    BIT_XOR_TKN,          // $
    BIT_SHIFT_LEFT_TKN,   // <<
    BIT_SHIFT_RIGHT_TKN,  // >>

    // Operators
    DOT_TKN,
    OP_ADD_TKN,
    OP_SUBTR_TKN,
    OP_MULT_TKN,
    OP_DIV_TKN,
    OP_MOD_TKN,
    OP_CARROT_TKN,  // Exponents
    OP_ADD_ADD_TKN,
    OP_ADD_EQUAL_TKN,
    OP_SUBTR_SUBTR_TKN,
    OP_SUBTR_EQUAL_TKN,
    OP_MULT_EQUAL_TKN,
    OP_DIV_EQUAL_TKN,
    OP_MOD_EQUAL_TKN,

    // Function Defs
    ARROW_TKN,          // () "->" void
    SINGLE_RETURN_TKN,  // add = (a: int, b: int) :: a + b
    RETURN_TKN,

    // Miscellaneous
    COMMA_TKN,
    DEREF_TKN,  // Particle.*
    END_TKN
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

    // inline std::string_view toStr() { return std::string_view(src->c_str() + index, cLen); }
    // inline std::string toString() { return src->substr(index, cLen); }
    inline std::string toStr() { return src->substr(index, cLen); }
};

/*
inline std::string_view tokenToStr(Token& token) {
    return std::string_view(token.src->c_str() + token.index, token.cLen);
}
*/

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
