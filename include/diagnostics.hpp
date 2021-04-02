#pragma once

#include <chrono>
#include <string>

#include "utils.hpp"

static constexpr char TOTAL_DISPLAY_LINES = 3;

struct ASTNode;

struct Line {
    int line;
    int leading;
    int beginI;
    int endI;
    Line* next;
};

struct ErrorMsg {
    enum Tag { EMPTY, ERROR, WARNING };
    Tag infoTag = ERROR;

    int beginI;
    int endI;
    int offset = 0;  // Used to offset "after indicator" one after the current indicator
    const std::string msg;

    std::string fixMsg;
    std::string noteMsg;

    // Error building temporary variables
    int line;
    int ch;  // column number - 1
    int leading;
    Line* lines[TOTAL_DISPLAY_LINES];
    int maxNumLen;

    inline ErrorMsg(std::string&& msg, int beginI, int endI) : msg(std::move(msg)), beginI(beginI), endI(endI) {}
    inline ErrorMsg() {}

    ErrorMsg* tag(Tag errTag) {
        this->infoTag = errTag;
        return this;
    }
    ErrorMsg* fix(const std::string&& fixMsg) {
        this->fixMsg = std::move(fixMsg);
        return this;
    }
    ErrorMsg* note(const std::string&& noteMsg) {
        this->noteMsg = std::move(noteMsg);
        return this;
    }
};

struct Token;

class Diagnostics {
    std::chrono::high_resolution_clock::time_point start;
    SList<ErrorMsg*> errors;
    int maxIndex = 0;

    bool isRecovering = false;  // Discard any errors thrown

   public:
    const std::string& src;

    inline Diagnostics(const std::string& src) : src(src), errors({}) {
        start = std::chrono::high_resolution_clock::now();
    }
    inline ~Diagnostics() { errors.destroy(); }

    bool has_errors() { return !errors.empty(); }

    ErrorMsg* last_err();
    inline void pop_last_err() { errors.pop(); }
    inline void set_recover_mode(bool recover) { isRecovering = recover; };

    ErrorMsg* err_loc(std::string&& msg, int beginI, int endI);
    ErrorMsg* err_loc(std::string&& msg, int beginI);

    ErrorMsg* err_token(std::string&& msg, const Token& token);
    ErrorMsg* err_after_token(std::string&& msg, const Token& token);

    ErrorMsg* err_node(std::string&& msg, const ASTNode& node);

    std::string emit();
};