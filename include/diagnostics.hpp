#pragma once

#include <chrono>
#include <stdexcept>
#include <string>

#include "ast.hpp"

struct Token;
class Lexer;

#ifndef NDEBUG
#define ASSERT(assertion, errMsg)                                                                              \
    do {                                                                                                       \
        if (!(assertion))                                                                                      \
            throw std::runtime_error("--ASSERTION ERROR-- in " + std::string(__FILE__) + "(" +                 \
                                     std::to_string(__LINE__) + "): \n\t" + __PRETTY_FUNCTION__ + "\n\t    " + \
                                     std::to_string(__LINE__) + "    " errMsg);                                \
    } while (false)
#else
#define ASSERT(assertion, errMsg) \
    do {                          \
    } while (false)
#endif

struct ASTNode;

class Diagnostics {
    static const char TOTAL_DISPLAY_LINES = 3;
    static const char LINE_NUM_LEADING_WHITESPACE = 2;
    static const char LINE_NUM_TRAILING_WHITESPACE = 3;

    std::chrono::high_resolution_clock::time_point start;
    std::string header;
    std::string msg;
    Lexer* lexer;

    int warnings = 0;
    int errors = 0;

    Diagnostics& withIndicator(int col, int length, int leadingNumSpace);

   public:
    Diagnostics(std::string header, Lexer* lexer) : header(std::move(header)), lexer(lexer) {
        start = std::chrono::high_resolution_clock::now();
    }

    Diagnostics& warn();
    Diagnostics& err();

    Diagnostics& at(const std::string& appendMsg, int index, int indicatorLen = 1, bool useNewLine = false);
    Diagnostics& at(const std::string& appendMsg, const Token& token);
    Diagnostics& at(const std::string& appendMsg, const ASTNode& node);
    Diagnostics& after(const std::string& appendMsg, const Token& token);

    Diagnostics& note(const std::string& appendMsg);
    Diagnostics& fix(const std::string& appendMsg);

    std::string out();
};

[[noreturn]] inline void panic(Diagnostics& dx) { throw std::runtime_error(dx.out()); }