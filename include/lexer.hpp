#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "diagnostics.hpp"

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
    FOR,
    BREAK,
    CONTINUE,

    // Declarations
    IDENTIFIER,
    COLON,

    // Types
    VOID_TYPE,
    MOD_TYPE,  // spaceMod: Module = mod { }
    TY_TYPE,   // thing: Type = #type Particle
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

std::string token_type_to_str(TokenType type);

struct Token {
    TokenType type;

    int beginI;
    int endI;

    union {
        long long longVal;
        double doubleVal;
        std::string* sourceStr;
    };

    inline std::string get_string_val() { return sourceStr->substr(beginI, endI - beginI); }
};

class Lexer {
    int curIndex = 0;
    int curCLen = 1;

    int cacheIndex = 0;
    std::vector<std::unique_ptr<Token>> tokenCache;

   public:
    static constexpr size_t TAB_WIDTH = 4;

    Diagnostics dx;
    inline Lexer() : dx(sourceStr) {}

    std::string sourceStr;
    bool from_file_path(const char* filePath);

    std::unique_ptr<Token> make_token(TokenType type);
    std::unique_ptr<Token> consume_token();

    inline Token* peek_token() {
        if (cacheIndex >= tokenCache.size()) tokenCache.push_back(consume_token());
        return tokenCache[cacheIndex].get();
    }

    inline Token* next_token() {
        Token* token = peek_token();
        if (token->type != TokenType::END) cacheIndex++;
        return token;
    }

    inline Token* last_token() {
        ASSERT(cacheIndex > 0, "Can't get last token of the first token in the file.");
        return tokenCache.at(cacheIndex - 1).get();
    }

    inline bool is_cursor_char(char assertChar) { return sourceStr[curIndex + curCLen] == assertChar; }

    static inline bool is_whitespace(char ch) { return ch == ' ' || ch == '\t' || ch == '\n'; }
    static inline bool is_letter(char ch) {
        return (ch >= 65 && ch < 91) || (ch >= 97 && ch < 123) || ch == '\'' || ch == '_';
    }
    static inline int to_num(char ch) { return ch - 48; }
    static inline bool is_number(char ch) { return (ch >= 48 && ch <= 57) || ch == '_'; }
    static inline bool is_hex(char ch) { return (ch >= 65 && ch < 71) || (ch >= 97 && ch < 103) || is_number(ch); }
};
