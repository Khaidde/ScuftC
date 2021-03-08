#include "diagnostics.hpp"

#include <deque>

#include "lexer.hpp"

Diagnostics& Diagnostics::warn() {
    msg += "Warning ";
    warnings++;
    return *this;
}

Diagnostics& Diagnostics::err() {
    msg += "Error ";
    errors++;
    return *this;
}

Diagnostics& Diagnostics::withIndicator(int colLoc, int length, int leadingNumSpace) {
    this->msg += std::string(LINE_NUM_LEADING_WHITESPACE + leadingNumSpace - 3, ' ');
    this->msg += "...";
    this->msg += std::string(LINE_NUM_TRAILING_WHITESPACE + colLoc, ' ');
    this->msg += std::string(length, '^');
    this->msg += "\n";
    return *this;
}

Diagnostics& Diagnostics::at(const std::string& appendMsg, int index, int indicatorLen, bool useNewLine) {
    index = index < lexer->sourceStr.length() ? index : static_cast<int>(lexer->sourceStr.length() - 1);

    struct SrcLine {
        std::string strVal;
        int beginCol;
        int endCol;
    };
    std::deque<SrcLine> sourceLines;

    int line = 1;
    int col = 1;
    int curI = 0;
    while (curI <= index) {
        std::string curStrLine;
        int beginCol = 0;
        int endCol = 0;
        char ch = lexer->sourceStr[curI];
        while (curI < lexer->sourceStr.length() && ch != '\n') {
            if (!beginCol && !Lexer::isWhitespace(ch)) beginCol = col;
            if (curI < index) {
                if (ch == '\t') {
                    col += Lexer::TAB_WIDTH;
                } else {
                    col++;
                }
            }
            if (ch == '\t') {
                curStrLine += std::string(Lexer::TAB_WIDTH, ' ');
            } else {
                curStrLine += ch;
            }
            curI++;
            endCol++;
            ch = lexer->sourceStr[curI];
        }
        if (useNewLine) endCol++;
        if (sourceLines.size() >= TOTAL_DISPLAY_LINES) sourceLines.pop_front();
        sourceLines.push_back({curStrLine, beginCol, endCol});
        curStrLine = "";
        if (useNewLine && curI == index) break;
        if (curI <= index) {
            line++;
            col = 1;
            curI++;
        }
    }
    this->msg += "(line:";
    this->msg += std::to_string(line);
    this->msg += ", col:";
    this->msg += std::to_string(col);
    this->msg += "): ";
    this->msg += appendMsg;
    this->msg += "\n";
    int minBeginCol = sourceLines.back().beginCol;
    for (const SrcLine& curLine : sourceLines) {
        if (curLine.beginCol > 0 && curLine.beginCol < minBeginCol) minBeginCol = curLine.beginCol;
    }
    int lineNumStrLen = static_cast<int>(floor(log10(line + 1)) + 1);
    int curLineNum = static_cast<int>(line - sourceLines.size());
    for (const SrcLine& curLine : sourceLines) {
        curLineNum++;
        this->msg +=
            std::string(LINE_NUM_LEADING_WHITESPACE + lineNumStrLen - std::to_string(curLineNum).length(), ' ');
        this->msg += std::to_string(curLineNum);
        this->msg += std::string(LINE_NUM_TRAILING_WHITESPACE, ' ');
        if (curLine.beginCol > 0) this->msg += curLine.strVal.substr(minBeginCol - 1);
        this->msg += "\n";
    }
    int lenOnLine = sourceLines.back().endCol - (col - 1);
    if (indicatorLen < 0) indicatorLen = lenOnLine;
    return this->withIndicator(col - minBeginCol, indicatorLen < lenOnLine ? indicatorLen : lenOnLine, lineNumStrLen);
}

Diagnostics& Diagnostics::at(const std::string& appendMsg, const Token& token) {
    return this->at(appendMsg, token.index, token.cLen);
}

Diagnostics& Diagnostics::at(const std::string& appendMsg, const ASTNode& node) {
    if (node.endIndex == 0) return this->at(appendMsg, node.locIndex, 1);
    return this->at(appendMsg, node.locIndex, node.endIndex - node.locIndex);
}

Diagnostics& Diagnostics::after(const std::string& appendMsg, const Token& token) {
    return this->at(appendMsg, token.index + token.cLen, 1, true);
}

Diagnostics& Diagnostics::note(const std::string& appendMsg) {
    this->msg += std::string(LINE_NUM_LEADING_WHITESPACE, ' ');
    this->msg += "note: ";
    this->msg += appendMsg;
    this->msg += "\n";
    return *this;
}

Diagnostics& Diagnostics::fix(const std::string& appendMsg) {
    this->msg += std::string(LINE_NUM_LEADING_WHITESPACE, ' ');
    this->msg += "fix: ";
    this->msg += appendMsg;
    this->msg += "\n";
    return *this;
}

std::string Diagnostics::out() {
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    if (warnings > 0) {
        msg += "\n-- " + std::to_string(warnings) + " warning";
        if (warnings > 1) msg += "s";
    }
    if (errors > 0) {
        msg += "\n-- " + std::to_string(errors) + " error";
        if (errors > 1) msg += "s";
    }
    return msg + "\n-- Finished " + header + " in " + std::to_string(diff.count()) + "ms\n";
}