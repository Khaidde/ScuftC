#pragma once

#include <memory>
#include <string>

enum TokenType {

    // Blocks
    LEFT_CURLY_TKN,
    RIGHT_CURLY_TKN,
    LEFT_PARENS_TKN,
    RIGHT_PARENS_TKN,

    // Statement Keywords
    MODULE_TKN,
    TYPE_TKN,  // particle = type {} or typeof(particle) == type;
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
    INT_TYPE_TKN,
    STRING_TYPE_TKN,
    BOOL_TYPE_TKN,

    // Assignments
    ASSIGNMENT_TKN,
    CONST_ASSIGNMENT_TKN,

    // Literal values
    INT_LITERAL_TKN,
    FLOAT_LITERAL_TKN,
    STRING_LITERAL_TKN,
    TRUE_TKN,
    FALSE_TKN,

    // Conditionals
    COND_OR_TKN,             // ||
    COND_AND_TKN,            // &&
    COND_XOR_TKN,            // $$
    COND_NOT_TKN,            // !
    COND_EQUALS_TKN,         // ==
    COND_NOT_EQUALS_TKN,     // !=
    COND_LESS_TKN,           // <
    COND_LESS_EQUAL_TKN,     // <=
    COND_GREATER_TKN,        // >
    COND_GREATER_EQUAL_TKN,  // >=

    // Bitwise Operators
    BIT_OR_TKN,           // |
    BIT_AND_TKN,          // &
    BIT_XOR_TKN,          // $
    BIT_NOT_TKN,          // ~
    BIT_SHIFT_LEFT_TKN,   // <<
    BIT_SHIFT_RIGHT_TKN,  // >>

    // Operators
    OP_DOT_TKN,
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
    ARROW_TKN,  // () "->" void
    RETURN_TKN,

    // Miscellaneous
    COMMA_TKN,
    END_TKN,

    UNKNOWN_TKN
};

struct Token {
    TokenType type;

    int index;
    int cLen;  // character length of the token

    union {
        long long longVal;
        double floatPointVal;
    };
};

struct Lexer;

class CompTimeException : public std::exception {
    static const char TOTAL_DISPLAY_LINES = 3;
    static const char LINE_NUM_LEADING_WHITESPACE = 2;
    static const char LINE_NUM_TRAILING_WHITESPACE = 3;

    std::string msg;
    Lexer* lexer;

    CompTimeException& withIndicator(int col, int length, int leadingNumSpace);

   public:
    CompTimeException(const char* header, Lexer* lexer) : msg(header), lexer(lexer) {}

    CompTimeException& at(std::string msg, int index);
    CompTimeException& at(std::string msg, Token* token);
    const char* what() const override;
};

class Lexer {
    CompTimeException ERR{"Error in Lexer\n", this};

    int curIndex = 0;
    int curCLen = 0;

   public:
    static const size_t TAB_WIDTH = 4;

    std::string sourceStr;
    void fromFilePath(const char* filePath);

    std::unique_ptr<Token> nextToken();
    std::unique_ptr<Token> consumeToken();
    std::unique_ptr<Token> makeToken(TokenType type);

    inline bool isCursorChar(char assertChar) { return sourceStr[curIndex + curCLen] == assertChar; }

    static inline bool isWhitespace(char ch) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }
    static inline bool isLetter(char ch) {
        return (ch >= 65 && ch < 91) || (ch >= 97 && ch < 123) || ch == '\'' || ch == '_';
    }
    static inline char toNum(char ch) { return ch - 48; }
    static inline bool isNumber(char ch) { return (ch >= 48 && ch <= 57) || ch == '_'; }
    static inline bool isHex(char ch) { return (ch >= 65 && ch < 71) || (ch >= 97 && ch < 103) || isNumber(ch); }
};
