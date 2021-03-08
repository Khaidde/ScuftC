#include "lexer.hpp"

#include <fstream>

#include "flags.hpp"

std::string tokenTypeToStr(TokenType type) {
    switch (type) {
        case LEFT_CURLY_TKN:
            return "{";
        case RIGHT_CURLY_TKN:
            return "}";
        case LEFT_PARENS_TKN:
            return "(";
        case RIGHT_PARENS_TKN:
            return ")";
        case MOD_TKN:
            return "mod";
        case TY_TKN:
            return "ty";
        case IF_TKN:
            return "if";
        case ELSE_TKN:
            return "else";
        case WHILE_TKN:
            return "while";
        case FOR_TKN:
            return "for";
        case IN_TKN:
            return "in";
        case BREAK_TKN:
            return "break";
        case CONTINUE_TKN:
            return "continue";
        case IDENTIFIER_TKN:
            return "'identifier'";
        case COLON_TKN:
            return ":";
        case VOID_TYPE_TKN:
            return "void";
        case MOD_TYPE_TKN:
            return "module";
        case TY_TYPE_TKN:
            return "type";
        case INT_TYPE_TKN:
            return "int";
        case DOUBLE_TYPE_TKN:
            return "double";
        case STRING_TYPE_TKN:
            return "string";
        case BOOL_TYPE_TKN:
            return "bool";
        case ASSIGNMENT_TKN:
            return "=";
        case CONST_ASSIGNMENT_TKN:
            return "=>";
        case INT_LITERAL_TKN:
            return "'int literal'";
        case DOUBLE_LITERAL_TKN:
            return "'double literal'";
        case STRING_LITERAL_TKN:
            return "'string literal'";
        case TRUE_TKN:
            return "true";
        case FALSE_TKN:
            return "false";
        case COND_NOT_TKN:
            return "!";
        case COND_OR_TKN:
            return "||";
        case COND_AND_TKN:
            return "&&";
        case COND_XOR_TKN:
            return "$$";
        case COND_EQUALS_TKN:
            return "==";
        case COND_NOT_EQUALS_TKN:
            return "!=";
        case COND_LESS_TKN:
            return "<";
        case COND_LESS_EQUAL_TKN:
            return "<=";
        case COND_GREATER_TKN:
            return ">";
        case COND_GREATER_EQUAL_TKN:
            return ">=";
        case BIT_NOT_TKN:
            return "~";
        case BIT_OR_TKN:
            return "|";
        case BIT_AND_TKN:
            return "&";
        case BIT_XOR_TKN:
            return "$";
        case BIT_SHIFT_LEFT_TKN:
            return "<<";
        case DOT_TKN:
            return ".";
        case OP_ADD_TKN:
            return "+";
        case OP_SUBTR_TKN:
            return "-";
        case OP_MULT_TKN:
            return "*";
        case OP_DIV_TKN:
            return "/";
        case OP_MOD_TKN:
            return "%";
        case OP_CARROT_TKN:
            return "^";
        case OP_ADD_ADD_TKN:
            return "++";
        case OP_ADD_EQUAL_TKN:
            return "+=";
        case OP_SUBTR_SUBTR_TKN:
            return "--";
        case OP_SUBTR_EQUAL_TKN:
            return "-=";
        case OP_MULT_EQUAL_TKN:
            return "*=";
        case OP_DIV_EQUAL_TKN:
            return "/=";
        case OP_MOD_EQUAL_TKN:
            return "%=";
        case ARROW_TKN:
            return "->";
        case SINGLE_RETURN_TKN:
            return "::";
        case RETURN_TKN:
            return "return";
        case COMMA_TKN:
            return ",";
        case DEREF_TKN:
            return ".*";
        case END_TKN:
            return "end of file";
        default:
            return "unknown token";
    }
}

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

std::unique_ptr<Token> Lexer::makeToken(TokenType type) {
    auto tkn = std::make_unique<Token>();
    tkn->type = type;
    tkn->index = curIndex;
    tkn->cLen = curCLen;
    tkn->src = &sourceStr;

    curIndex += curCLen;
    curCLen = 0;
    return tkn;
}

std::unique_ptr<Token> Lexer::consumeToken() {
    // Skip over whitespace
    while (curIndex < sourceStr.length() && isWhitespace(sourceStr[curIndex])) curIndex++;

    if (curIndex >= sourceStr.length()) return makeToken(END_TKN);

    curCLen++;
    switch (sourceStr[curIndex]) {
        case ';':
            if (!flags.dwSemiColons) {
                DX.warn()
                    .at("Semi-colons are not required in this language", curIndex)
                    .note("Semi-colons are treated as whitespace. Use -dw-semi-colons to disable warning");
            }
            curIndex++;
            curCLen = 0;
            return consumeToken();
        case '{':
            return makeToken(LEFT_CURLY_TKN);
        case '}':
            return makeToken(RIGHT_CURLY_TKN);
        case '(':
            return makeToken(LEFT_PARENS_TKN);
        case ')':
            return makeToken(RIGHT_PARENS_TKN);
        case ':':
            if (isCursorChar(':')) {
                curCLen++;
                return makeToken(SINGLE_RETURN_TKN);
            } else {
                return makeToken(COLON_TKN);
            }
        case '=':
            if (isCursorChar('>')) {
                curCLen++;
                return makeToken(CONST_ASSIGNMENT_TKN);
            } else {
                return makeToken(ASSIGNMENT_TKN);
            }
        case '!':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(COND_NOT_EQUALS_TKN);
            } else {
                return makeToken(COND_NOT_TKN);
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
            if (isCursorChar('*')) {
                curCLen++;
                return makeToken(DEREF_TKN);
            } else {
                return makeToken(DOT_TKN);
            }
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
                panic(DX.err().at("Invalid closing of block comment", curIndex));
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
                    panic(DX.err().at("Unterminated block comment", curIndex));
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
                panic(DX.err().at("Unterminated string literal", curIndex));
            }
            return makeToken(STRING_LITERAL_TKN);
        }
        default:
            if (isLetter(sourceStr[curIndex])) {
                while (isLetter(sourceStr[curIndex + curCLen]) || isNumber(sourceStr[curIndex + curCLen])) {
                    curCLen++;
                }
                std::string_view keyword(sourceStr.c_str() + curIndex, curCLen);
                if (keyword == "mod") {
                    return makeToken(MOD_TKN);
                } else if (keyword == "ty") {
                    return makeToken(TY_TKN);
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
                } else if (keyword == "module") {
                    return makeToken(MOD_TYPE_TKN);
                } else if (keyword == "type") {
                    return makeToken(TY_TYPE_TKN);
                } else if (keyword == "int") {
                    return makeToken(INT_TYPE_TKN);
                } else if (keyword == "double") {
                    return makeToken(DOUBLE_TYPE_TKN);
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
                bool overflow = false;
                long long number = 0;
                double divisor = 0;
                char ch = sourceStr[curIndex + curCLen];
                while ((base == 16 && isHex(ch)) || isNumber(ch) || ch == '.') {
                    if (ch != '_') {
                        if (ch == '.') {
                            if (sourceStr[curIndex + curCLen - 1] == '_') {
                                panic(DX.err().at("Underscore is not allowed here", curIndex + curCLen - 1));
                            }
                            if (divisor > 0) {
                                panic(DX.err().at("Numeric literal has too many decimal points \"" +
                                                      sourceStr.substr(curIndex, curCLen) + ".\"",
                                                  curIndex + curCLen));
                            } else {
                                divisor = 1;
                            }
                        } else {
                            // Convert digit into corresponding base: (in hexadecimal) A -> 10
                            int val;
                            if (toNum(ch) >= 0 && toNum(ch) < 10) {
                                val = toNum(ch);
                            } else if (isHex(ch)) {
                                val = ch - 65 + 10;
                                if (val > 15) {
                                    val -= (97 - 65);
                                }
                            } else {
                                ASSERT(false, "String is not assignable to a numeric value");
                            }

                            if (val < base) {
                                number = base * number + val;
                                if (divisor > 0) {
                                    divisor *= base;
                                } else {
                                    // Check for overflow
                                    // Negative number can indicate an overflow
                                    if ((number > INT_MAX || number < 0) && !overflow) overflow = true;
                                }
                            } else {
                                panic(DX.err().at(std::to_string(toNum(ch)) + " is an invalid digit value in base " +
                                                      std::to_string(base),
                                                  curIndex + curCLen));
                            }
                        }
                    } else if (divisor == 1) {
                        panic(DX.err().at("Underscore is not allowed here", curIndex + curCLen));
                    }
                    curCLen++;
                    ch = sourceStr[curIndex + curCLen];
                }
                if (sourceStr[curIndex + curCLen - 1] == '_') {
                    panic(DX.err().at("Underscore is not allowed here", curIndex + curCLen - 1));
                }
                if (overflow && divisor == 0) {
                    DX.warn()
                        .at("Numeric literal is too large to fit in an int ", curIndex, curCLen)
                        .note("The max value for an int literal is 2^31-1 = 2_147_483_647");
                }

                if (divisor > 0) {
                    // TODO make sure there is no precision loss
                    auto tkn = makeToken(DOUBLE_LITERAL_TKN);
                    tkn->doubleVal = static_cast<double>(number) / divisor;
                    return tkn;
                } else {
                    auto tkn = makeToken(INT_LITERAL_TKN);
                    tkn->longVal = number;
                    return tkn;
                }
            }
            return makeToken(UNKNOWN_TKN);
    }
}

Token* Lexer::peekToken() {
    if (cacheIndex >= tokenCache.size()) {
        tokenCache.push_back(consumeToken());
    }
    return tokenCache.at(cacheIndex).get();
}

Token* Lexer::nextToken() {
    Token* token = peekToken();
    cacheIndex++;
    return token;
}

Token* Lexer::lastToken() {
    ASSERT(cacheIndex > 0, "Can't get last token of the first token in the file.");
    return tokenCache.at(cacheIndex - 1).get();
}