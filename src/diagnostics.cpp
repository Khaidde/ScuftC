#include "diagnostics.hpp"

#include <cmath>
#include <stdexcept>

#include "ast.hpp"
#include "lexer.hpp"

ErrorMsg* Diagnostics::last_err() {
    ASSERT(!errors.empty(), "No last error was ever recorded");
    return errors.back().get();
}

ErrorMsg* Diagnostics::err_loc(std::string&& msg, int beginI, int endI) {
    ASSERT(endI > beginI, "Invalid error location(beginI = " + std::to_string(beginI) +
                              ", endI = " + std::to_string(endI) + " ). End index must be greater than begin index");
    if (endI > maxIndex) maxIndex = endI;

    // Ignore duplicate errors at the same place
    auto errorMsg = std::make_unique<ErrorMsg>(std::move(msg), beginI, endI);

    if (isRecovering) {
        discardErrors.push_back(std::move(errorMsg));
        return discardErrors.back().get();
    } else {
        errors.push_back(std::move(errorMsg));
        return errors.back().get();
    }
}

ErrorMsg* Diagnostics::err_loc(std::string&& msg, int beginI) { return err_loc(std::move(msg), beginI, beginI + 1); }

ErrorMsg* Diagnostics::err_token(std::string&& msg, const Token& token) {
    return err_loc(std::move(msg), token.beginI, token.endI);
}

ErrorMsg* Diagnostics::err_after_token(std::string&& msg, const Token& token) {
    if (token.endI > maxIndex) maxIndex = token.endI;
    auto errMsg = err_loc(std::move(msg), token.endI - 1, token.endI);
    errMsg->offset = 1;
    return errMsg;
}

ErrorMsg* Diagnostics::err_node(std::string&& msg, const ASTNode& node) {
    return err_loc(std::move(msg), node.beginI, node.endI);
}

namespace {
constexpr char TAG_LEN = 6;
constexpr char LINE_NUM_TRAILING_WHITESPACE = 3;
constexpr int ELLIPSES_LEN = 3;
constexpr char FIX_LEN = 7;
constexpr char NOTE_LEN = 6;

inline int strlenNum(int num) { return static_cast<int>(floor(log10(num)) + 1); }
}  // namespace

std::string Diagnostics::emit() {
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    std::string finishTime = "\n-- Finished in " + std::to_string(diff.count()) + "ms";

    int line = 1;
    Line* lastLine = nullptr;
    for (int c = 0; c <= maxIndex; c++) {
        Line* l = new Line;
        l->line = line++;
        l->beginI = c;
        l->leading = 0;
        bool isLeading = true;
        for (; c < src.length() && src[c] != '\n'; c++) {
            if (isLeading && !Lexer::isWhitespace(src[c])) {
                l->leading = c - l->beginI;
                isLeading = false;
            }
        }
        l->endI = c;
        l->next = lastLine;
        lastLine = l;
    }

    size_t size = 0;
    ErrorMsg* lastErr = nullptr;
    for (const auto& e : errors) {
        size += e->msg.size() + 1;  // Add new line char
        for (Line* l = lastLine; l != nullptr; l = l->next) {
            if (e->beginI >= l->beginI) {
                int ch = 0;
                for (int i = l->beginI; i < e->beginI; i++) {
                    ch += src[i] == '\t' ? Lexer::TAB_WIDTH : 1;
                }
                size += TAG_LEN;
                size += 6;  // (line:
                e->line = l->line;
                size += strlenNum(e->line);
                size += 6;  // , col:
                e->ch = ch;
                size += strlenNum(e->ch + 1 + e->offset);
                size += 2;  // )

                // Get minimum leading space
                Line* b = l;
                e->leading = l->leading;
                for (int i = 0; i < TOTAL_DISPLAY_LINES && l != nullptr; i++) {
                    if (l->leading < e->leading) e->leading = l->leading;
                    l = l->next;
                }

                // Calculated new begin index after leading space adjustments
                l = b;
                e->maxNumLen = std::max(ELLIPSES_LEN, strlenNum(e->line));
                for (int i = 0; i < TOTAL_DISPLAY_LINES && l != nullptr; i++) {
                    size += TAG_LEN + e->maxNumLen + LINE_NUM_TRAILING_WHITESPACE;
                    e->lines[i] = l;
                    size += l->endI - l->beginI - e->leading;
                    size += 1;  // Add new line char
                    l = l->next;
                }
                size += TAG_LEN + e->maxNumLen;
                size += LINE_NUM_TRAILING_WHITESPACE + e->ch - e->leading + e->offset;
                size += std::min(e->lines[0]->endI, e->endI) - e->beginI;

                if (!e->fixMsg.empty()) {
                    size += FIX_LEN;
                    size += e->fixMsg.size();
                }
                size += 1;  // Add new line char
                if (!e->noteMsg.empty()) {
                    size += TAG_LEN + e->maxNumLen;
                    size += NOTE_LEN;
                    size += e->noteMsg.size();
                    size += 1;  // Add new line char
                }
                break;
            }
        }
    }
    size += finishTime.size();

    std::string res;
    res.reserve(size);
    for (auto& e : errors) {
        switch (e->infoTag) {
            case ErrorMsg::EMPTY:
                res += "------";
                break;
            case ErrorMsg::CONTEXT:
                res += "   In:";
                break;
            case ErrorMsg::ERROR:
                res += "Error:";
                break;
            case ErrorMsg::WARNING:
                res += "Warn::";
                break;
        }
        res += "(line:";
        res += std::to_string(e->line);
        res += ", col:";
        res += std::to_string(e->ch + 1 + e->offset);
        res += ") ";
        res += e->msg;
        res += "\n";
        for (int i = TOTAL_DISPLAY_LINES - 1; i >= 0; i--) {
            int lineNum = e->line - i;
            if (lineNum <= 0) continue;

            res += std::string(TAG_LEN, ' ');

            res += std::string(e->maxNumLen - strlenNum(lineNum), ' ');
            res += std::to_string(lineNum);
            res += std::string(LINE_NUM_TRAILING_WHITESPACE, ' ');
            res += src.substr(e->lines[i]->beginI + e->leading, e->lines[i]->endI - e->lines[i]->beginI - e->leading);
            res += "\n";
        }
        res += std::string(TAG_LEN, ' ');
        res += std::string(e->maxNumLen - ELLIPSES_LEN, ' ');
        res += "...";
        res += std::string(LINE_NUM_TRAILING_WHITESPACE + e->ch - e->leading + e->offset, ' ');
        res += "^";
        Line* errLine = e->lines[0];
        if (e->endI > e->beginI + 1) {
            if (e->endI > errLine->endI) {
                res += std::string(errLine->endI - e->beginI - 1, '-');
            } else {
                res += std::string(e->endI - e->beginI - 2, '-');
                res += "^";
            }
        }
        if (!e->fixMsg.empty()) {
            res += "  fix: ";
            res += e->fixMsg;
        }
        res += "\n";
        if (!e->noteMsg.empty()) {
            res += std::string(TAG_LEN + e->maxNumLen, ' ');
            res += "note: ";
            res += e->noteMsg;
            res += "\n";
        }
    }
    res += finishTime;
    ASSERT(res.size() == size, "Bad string size allocation");

    Line* cur = lastLine;
    while (cur != nullptr) {
        Line* next = cur->next;
        delete cur;
        cur = next;
    }
    return res;
}