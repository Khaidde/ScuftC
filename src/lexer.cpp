#include "lexer.hpp"

#include <cmath>
#include <deque>
#include <fstream>

CompTimeException& CompTimeException::withIndicator(int colLoc, int length, int leadingNumSpace) {
    this->msg += std::string(LINE_NUM_LEADING_WHITESPACE + leadingNumSpace - 3, ' ');
    this->msg += "...";
    this->msg += std::string(LINE_NUM_TRAILING_WHITESPACE + colLoc, ' ');
    this->msg += "^";
    return *this;
}

CompTimeException& CompTimeException::at(std::string msg, int index) {
    if (index >= lexer->sourceStr.length()) {
        throw "Compilation error index must be less than length of source string";
    }

    struct SrcLine {
        std::string strVal;
        int beginCol;
    };
    std::deque<SrcLine> sourceLines;

    int line = 1;
    int col = 1;
    int curI = 0;
    while (curI <= index) {
        std::string curStrLine;
        int beginCol = 0;
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
            ch = lexer->sourceStr[curI];
        }
        if (sourceLines.size() >= TOTAL_DISPLAY_LINES) sourceLines.pop_front();
        sourceLines.push_back({curStrLine, beginCol});
        curStrLine = "";
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
    this->msg += msg;
    this->msg += "\n";
    int sourceLinesLen = sourceLines.size();
    int minBeginCol = sourceLines.front().beginCol;
    for (SrcLine curLine : sourceLines) {
        if (curLine.beginCol > 0 && curLine.beginCol < minBeginCol) minBeginCol = curLine.beginCol;
    }

    size_t lineNumStrLen = floor(log10(line + 1)) + 1;
    int curLineNum = line - sourceLines.size();
    for (SrcLine curLine : sourceLines) {
        curLineNum++;
        this->msg +=
            std::string(LINE_NUM_LEADING_WHITESPACE + lineNumStrLen - std::to_string(curLineNum).length(), ' ');
        this->msg += std::to_string(curLineNum);
        this->msg += std::string(LINE_NUM_TRAILING_WHITESPACE, ' ');
        this->msg += curLine.strVal.substr(minBeginCol - 1);
        this->msg += "\n";
    }
    return this->withIndicator(col - minBeginCol, 1, lineNumStrLen);
}

CompTimeException& CompTimeException::at(std::string msg, Token* token) { return this->at(msg, token->index); }

const char* CompTimeException::what() const { return msg.c_str(); }

void Lexer::fromFilePath(const char* filePath) {
    std::ifstream file(filePath);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        this->sourceStr = std::string(size, ' ');
        file.seekg(0);
        file.read(&this->sourceStr[0], size);

        curIndex = 0;
        curCLen = 0;
    } else {
        throw std::ifstream::failure("File not found");
    }
}

std::unique_ptr<Token> Lexer::nextToken() { return consumeToken(); }

std::unique_ptr<Token> Lexer::consumeToken() {
    // Skip over whitespace (ignore new line char \n)
    while (curIndex < sourceStr.length() && isWhitespace(sourceStr[curIndex])) curIndex++;

    if (curIndex >= sourceStr.length()) return makeToken(END_TKN);

    curCLen++;
    switch (sourceStr[curIndex]) {
        case '{':
            return makeToken(LEFT_CURLY_TKN);
        case '}':
            return makeToken(RIGHT_CURLY_TKN);
        case '(':
            return makeToken(LEFT_PARENS_TKN);
        case ')':
            return makeToken(RIGHT_PARENS_TKN);
        case ':':
            return makeToken(COLON_TKN);
        case '=':
            if (isCursorChar('>')) {
                curCLen++;
                return makeToken(CONST_ASSIGNMENT_TKN);
            } else {
                return makeToken(ASSIGNMENT_TKN);
            }
        case '|':
            if (isCursorChar('|')) {
                curCLen++;
                return makeToken(COND_OR_TKN);
            } else {
                return makeToken(BIT_OR_TKN);
            }
        case '&':
            if (isCursorChar('&')) {
                curCLen++;
                return makeToken(COND_AND_TKN);
            } else {
                return makeToken(BIT_AND_TKN);
            }
        case '$':
            if (isCursorChar('$')) {
                curCLen++;
                return makeToken(COND_XOR_TKN);
            } else {
                return makeToken(BIT_XOR_TKN);
            }
        case '!':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(COND_NOT_EQUALS_TKN);
            } else {
                return makeToken(COND_NOT_TKN);
            }
        case '<':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(COND_LESS_EQUAL_TKN);
            } else if (isCursorChar('<')) {
                curCLen++;
                return makeToken(BIT_SHIFT_LEFT_TKN);
            } else {
                return makeToken(COND_LESS_TKN);
            }
        case '>':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(COND_GREATER_EQUAL_TKN);
            } else if (isCursorChar('>')) {
                curCLen++;
                return makeToken(BIT_SHIFT_RIGHT_TKN);
            } else {
                return makeToken(COND_GREATER_TKN);
            }
        case '.':
            return makeToken(OP_DOT_TKN);
        case '+':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(OP_ADD_EQUAL_TKN);
            } else if (isCursorChar('+')) {
                curCLen++;
                return makeToken(OP_ADD_ADD_TKN);
            } else {
                return makeToken(OP_ADD_TKN);
            }
        case '-':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(OP_SUBTR_EQUAL_TKN);
            } else if (isCursorChar('-')) {
                curCLen++;
                return makeToken(OP_SUBTR_SUBTR_TKN);
            } else if (isCursorChar('>')) {
                curCLen++;
                return makeToken(ARROW_TKN);
            } else {
                return makeToken(OP_SUBTR_TKN);
            }
        case '*':
            if (isCursorChar('*')) {
                curCLen++;
                return makeToken(OP_MULT_EQUAL_TKN);
            } else if (isCursorChar('/')) {
                throw ERR.at("Invalid closing of block comment", curIndex);
            } else {
                return makeToken(OP_MULT_TKN);
            }
        case '/':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(OP_DIV_EQUAL_TKN);
            } else if (isCursorChar('/')) {
                while (curIndex < sourceStr.length() && sourceStr[curIndex] != '\n') {
                    curIndex++;
                }
                curIndex++;
                curCLen = 0;
                return consumeToken();
            } else if (isCursorChar('*')) {
                while (curIndex + curCLen < sourceStr.length() &&
                       !(sourceStr[curIndex + curCLen - 1] == '*' && isCursorChar('/'))) {
                    curCLen++;
                }
                curCLen++;
                if (curIndex + curCLen > sourceStr.length()) {
                    throw ERR.at("Unterminated block comment", curIndex);
                }
                curIndex += curCLen;
                curCLen = 0;
                return consumeToken();
            } else {
                return makeToken(OP_DIV_TKN);
            }
        case '%':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(OP_MOD_EQUAL_TKN);
            } else {
                return makeToken(OP_MOD_TKN);
            }
        case '^':
            return makeToken(OP_CARROT_TKN);
        case ',':
            return makeToken(COMMA_TKN);
        case '"': {
            while (curIndex + curCLen < sourceStr.length() && !isCursorChar('"')) {
                if (isCursorChar('\\')) curCLen++;
                curCLen++;
            }
            curCLen++;
            if (curIndex + curCLen > sourceStr.length()) {
                throw ERR.at("Unterminated string literal", curIndex);
            }
            return makeToken(STRING_LITERAL_TKN);
        }
        default:
            if (isLetter(sourceStr[curIndex])) {
                while (isLetter(sourceStr[curIndex + curCLen]) || isNumber(sourceStr[curIndex + curCLen])) {
                    curCLen++;
                }
                std::string_view keyword(sourceStr.c_str() + curIndex, curCLen);
                if (keyword == "module") {
                    return makeToken(MODULE_TKN);
                } else if (keyword == "type") {
                    return makeToken(TYPE_TKN);
                } else if (keyword == "if") {
                    return makeToken(IF_TKN);
                } else if (keyword == "else") {
                    return makeToken(ELSE_TKN);
                } else if (keyword == "while") {
                    return makeToken(WHILE_TKN);
                } else if (keyword == "for") {
                    return makeToken(FOR_TKN);
                } else if (keyword == "in") {
                    return makeToken(IN_TKN);
                } else if (keyword == "break") {
                    return makeToken(BREAK_TKN);
                } else if (keyword == "continue") {
                    return makeToken(CONTINUE_TKN);
                } else if (keyword == "true") {
                    return makeToken(TRUE_TKN);
                } else if (keyword == "false") {
                    return makeToken(FALSE_TKN);
                } else if (keyword == "void") {
                    return makeToken(VOID_TYPE_TKN);
                } else if (keyword == "int") {
                    return makeToken(INT_TYPE_TKN);
                } else if (keyword == "string") {
                    return makeToken(STRING_TYPE_TKN);
                } else if (keyword == "bool") {
                    return makeToken(BOOL_TYPE_TKN);
                } else if (keyword == "return") {
                    return makeToken(RETURN_TKN);
                } else {
                    return makeToken(IDENTIFIER_TKN);
                }
            } else if (isNumber(sourceStr[curIndex])) {
                int base = 10;
                if (sourceStr[curIndex] == '0') {
                    if (isCursorChar('b')) {
                        base = 2;
                        curCLen++;
                    } else if (isCursorChar('o')) {
                        base = 8;
                        curCLen++;
                    } else if (isCursorChar('x')) {
                        base = 16;
                        curCLen++;
                    }
                } else {
                    curCLen--;
                }
                long number = 0;
                bool point = false;
                double divideBy = 1;
                char ch = sourceStr[curIndex + curCLen];
                while ((base == 16 && isHex(ch)) || isNumber(ch) || ch == '.') {
                    if (ch != '_') {
                        if (ch == '.') {
                            if (point) {
                                throw ERR.at("Numeric literal has too many decimal points \"" +
                                                 sourceStr.substr(curIndex, curCLen) + ".\"",
                                             curIndex + curCLen);
                            } else {
                                point = true;
                            }
                        } else {
                            int val;
                            if (toNum(ch) >= 0 && toNum(ch) < 10) {
                                val = toNum(ch);
                            } else if (isHex(ch)) {
                                val = ch - 65 + 10;
                                if (val > 15) {
                                    val -= (97 - 65);
                                }
                            }
                            if (val < base) {
                                number = base * number + val;
                                if (point) divideBy = divideBy * base;
                            } else {
                                throw ERR.at(std::to_string(toNum(ch)) + " is an invalid digit value in base " +
                                                 std::to_string(base),
                                             curIndex + curCLen);
                            }
                        }
                    }
                    curCLen++;
                    ch = sourceStr[curIndex + curCLen];
                }

                if (point) {
                    // TODO make sure there is no precision loss
                    auto tkn = makeToken(FLOAT_LITERAL_TKN);
                    tkn->floatPointVal = number / divideBy;
                    return tkn;
                } else {
                    // TODO turn into int or long depending on size of number
                    auto tkn = makeToken(INT_LITERAL_TKN);
                    tkn->longVal = number;
                    return tkn;
                }
            } else {
                return makeToken(UNKNOWN_TKN);
            }
            return makeToken(STRING_TYPE_TKN);
    }
}

std::unique_ptr<Token> Lexer::makeToken(TokenType type) {
    auto tkn = std::make_unique<Token>();
    tkn->type = type;
    tkn->index = curIndex;
    tkn->cLen = curCLen;

    curIndex += curCLen;
    curCLen = 0;
    return tkn;
}