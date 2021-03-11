#include "lexer.hpp"

#include <fstream>

#include "flags.hpp"

std::string tokenTypeToStr(TokenType type) {
    switch (type) {
        case TokenType::LEFT_CURLY:
            return "{";
        case TokenType::RIGHT_CURLY:
            return "}";
        case TokenType::LEFT_PARENS:
            return "(";
        case TokenType::RIGHT_PARENS:
            return ")";
        case TokenType::MOD:
            return "mod";
        case TokenType::TY:
            return "ty";
        case TokenType::IF:
            return "if";
        case TokenType::ELSE:
            return "else";
        case TokenType::WHILE:
            return "while";
        case TokenType::FOR:
            return "for";
        case TokenType::IN:
            return "in";
        case TokenType::BREAK:
            return "break";
        case TokenType::CONTINUE:
            return "continue";
        case TokenType::IDENTIFIER:
            return "'identifier'";
        case TokenType::COLON:
            return ":";
        case TokenType::VOID_TYPE:
            return "void";
        case TokenType::MOD_TYPE:
            return "module";
        case TokenType::TY_TYPE:
            return "type";
        case TokenType::INT_TYPE:
            return "int";
        case TokenType::DOUBLE_TYPE:
            return "double";
        case TokenType::STRING_TYPE:
            return "string";
        case TokenType::BOOL_TYPE:
            return "bool";
        case TokenType::ASSIGNMENT:
            return "=";
        case TokenType::CONST_ASSIGNMENT:
            return "=>";
        case TokenType::INT_LITERAL:
            return "'int literal'";
        case TokenType::DOUBLE_LITERAL:
            return "'double literal'";
        case TokenType::STRING_LITERAL:
            return "'string literal'";
        case TokenType::TRUE:
            return "true";
        case TokenType::FALSE:
            return "false";
        case TokenType::COND_NOT:
            return "!";
        case TokenType::COND_OR:
            return "||";
        case TokenType::COND_AND:
            return "&&";
        case TokenType::COND_XOR:
            return "$$";
        case TokenType::COND_EQUALS:
            return "==";
        case TokenType::COND_NOT_EQUALS:
            return "!=";
        case TokenType::COND_LESS:
            return "<";
        case TokenType::COND_LESS_EQUAL:
            return "<=";
        case TokenType::COND_GREATER:
            return ">";
        case TokenType::COND_GREATER_EQUAL:
            return ">=";
        case TokenType::BIT_NOT:
            return "~";
        case TokenType::BIT_OR:
            return "|";
        case TokenType::BIT_AND:
            return "&";
        case TokenType::BIT_XOR:
            return "$";
        case TokenType::BIT_SHIFT_LEFT:
            return "<<";
        case TokenType::DOT:
            return ".";
        case TokenType::OP_ADD:
            return "+";
        case TokenType::OP_SUBTR:
            return "-";
        case TokenType::OP_MULT:
            return "*";
        case TokenType::OP_DIV:
            return "/";
        case TokenType::OP_MOD:
            return "%";
        case TokenType::OP_CARROT:
            return "^";
        case TokenType::OP_ADD_ADD:
            return "++";
        case TokenType::OP_ADD_EQUAL:
            return "+=";
        case TokenType::OP_SUBTR_SUBTR:
            return "--";
        case TokenType::OP_SUBTR_EQUAL:
            return "-=";
        case TokenType::OP_MULT_EQUAL:
            return "*=";
        case TokenType::OP_DIV_EQUAL:
            return "/=";
        case TokenType::OP_MOD_EQUAL:
            return "%=";
        case TokenType::ARROW:
            return "->";
        case TokenType::SINGLE_RETURN:
            return "::";
        case TokenType::RETURN:
            return "return";
        case TokenType::COMMA:
            return ",";
        case TokenType::DEREF:
            return ".*";
        case TokenType::END:
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

    if (curIndex >= sourceStr.length()) return makeToken(TokenType::END);

    curCLen++;
    switch (sourceStr[curIndex]) {
        case ';':
            if (!Flags::dwSemiColons) {
                DX->warn()
                    .at("Semi-colons are not required in this language", curIndex)
                    .note("Semi-colons are treated as whitespace. Use -dw-semi-colons to disable warning");
            }
            curIndex++;
            curCLen = 0;
            return consumeToken();
        case '{':
            return makeToken(TokenType::LEFT_CURLY);
        case '}':
            return makeToken(TokenType::RIGHT_CURLY);
        case '(':
            return makeToken(TokenType::LEFT_PARENS);
        case ')':
            return makeToken(TokenType::RIGHT_PARENS);
        case ':':
            if (isCursorChar(':')) {
                curCLen++;
                return makeToken(TokenType::SINGLE_RETURN);
            } else {
                return makeToken(TokenType::COLON);
            }
        case '=':
            if (isCursorChar('>')) {
                curCLen++;
                return makeToken(TokenType::CONST_ASSIGNMENT);
            } else {
                return makeToken(TokenType::ASSIGNMENT);
            }
        case '!':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(TokenType::COND_NOT_EQUALS);
            } else {
                return makeToken(TokenType::COND_NOT);
            }
        case '|':
            if (isCursorChar('|')) {
                curCLen++;
                return makeToken(TokenType::COND_OR);
            } else {
                return makeToken(TokenType::BIT_OR);
            }
        case '&':
            if (isCursorChar('&')) {
                curCLen++;
                return makeToken(TokenType::COND_AND);
            } else {
                return makeToken(TokenType::BIT_AND);
            }
        case '$':
            if (isCursorChar('$')) {
                curCLen++;
                return makeToken(TokenType::COND_XOR);
            } else {
                return makeToken(TokenType::BIT_XOR);
            }
        case '<':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(TokenType::COND_LESS_EQUAL);
            } else if (isCursorChar('<')) {
                curCLen++;
                return makeToken(TokenType::BIT_SHIFT_LEFT);
            } else {
                return makeToken(TokenType::COND_LESS);
            }
        case '>':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(TokenType::COND_GREATER_EQUAL);
            } else if (isCursorChar('>')) {
                curCLen++;
                return makeToken(TokenType::BIT_SHIFT_RIGHT);
            } else {
                return makeToken(TokenType::COND_GREATER);
            }
        case '.':
            if (isCursorChar('*')) {
                curCLen++;
                return makeToken(TokenType::DEREF);
            } else {
                return makeToken(TokenType::DOT);
            }
        case '+':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(TokenType::OP_ADD_EQUAL);
            } else if (isCursorChar('+')) {
                curCLen++;
                return makeToken(TokenType::OP_ADD_ADD);
            } else {
                return makeToken(TokenType::OP_ADD);
            }
        case '-':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(TokenType::OP_SUBTR_EQUAL);
            } else if (isCursorChar('-')) {
                curCLen++;
                return makeToken(TokenType::OP_SUBTR_SUBTR);
            } else if (isCursorChar('>')) {
                curCLen++;
                return makeToken(TokenType::ARROW);
            } else {
                return makeToken(TokenType::OP_SUBTR);
            }
        case '*':
            if (isCursorChar('*')) {
                curCLen++;
                return makeToken(TokenType::OP_MULT_EQUAL);
            } else if (isCursorChar('/')) {
                panic(DX->err().at("Invalid closing of block comment", curIndex));
            } else {
                return makeToken(TokenType::OP_MULT);
            }
        case '/':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(TokenType::OP_DIV_EQUAL);
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
                    panic(DX->err().at("Unterminated block comment", curIndex));
                }
                curIndex += curCLen;
                curCLen = 0;
                return consumeToken();
            } else {
                return makeToken(TokenType::OP_DIV);
            }
        case '%':
            if (isCursorChar('=')) {
                curCLen++;
                return makeToken(TokenType::OP_MOD_EQUAL);
            } else {
                return makeToken(TokenType::OP_MOD);
            }
        case '^':
            return makeToken(TokenType::OP_CARROT);
        case ',':
            return makeToken(TokenType::COMMA);
        case '"': {
            while (curIndex + curCLen < sourceStr.length() && !isCursorChar('"')) {
                if (isCursorChar('\\')) curCLen++;
                curCLen++;
            }
            curCLen++;
            if (curIndex + curCLen > sourceStr.length()) {
                panic(DX->err().at("Unterminated string literal", curIndex));
            }
            return makeToken(TokenType::STRING_LITERAL);
        }
        default:
            if (isLetter(sourceStr[curIndex])) {
                while (isLetter(sourceStr[curIndex + curCLen]) || isNumber(sourceStr[curIndex + curCLen])) {
                    curCLen++;
                }
                std::string_view keyword(sourceStr.c_str() + curIndex, curCLen);
                if (keyword == "mod") {
                    return makeToken(TokenType::MOD);
                } else if (keyword == "ty") {
                    return makeToken(TokenType::TY);
                } else if (keyword == "if") {
                    return makeToken(TokenType::IF);
                } else if (keyword == "else") {
                    return makeToken(TokenType::ELSE);
                } else if (keyword == "while") {
                    return makeToken(TokenType::WHILE);
                } else if (keyword == "for") {
                    return makeToken(TokenType::FOR);
                } else if (keyword == "in") {
                    return makeToken(TokenType::IN);
                } else if (keyword == "break") {
                    return makeToken(TokenType::BREAK);
                } else if (keyword == "continue") {
                    return makeToken(TokenType::CONTINUE);
                } else if (keyword == "true") {
                    return makeToken(TokenType::TRUE);
                } else if (keyword == "false") {
                    return makeToken(TokenType::FALSE);
                } else if (keyword == "void") {
                    return makeToken(TokenType::VOID_TYPE);
                } else if (keyword == "module") {
                    return makeToken(TokenType::MOD_TYPE);
                } else if (keyword == "type") {
                    return makeToken(TokenType::TY_TYPE);
                } else if (keyword == "int") {
                    return makeToken(TokenType::INT_TYPE);
                } else if (keyword == "double") {
                    return makeToken(TokenType::DOUBLE_TYPE);
                } else if (keyword == "string") {
                    return makeToken(TokenType::STRING_TYPE);
                } else if (keyword == "bool") {
                    return makeToken(TokenType::BOOL_TYPE);
                } else if (keyword == "return") {
                    return makeToken(TokenType::RETURN);
                } else {
                    return makeToken(TokenType::IDENTIFIER);
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
                                panic(DX->err().at("Underscore is not allowed here", curIndex + curCLen - 1));
                            }
                            if (divisor > 0) {
                                panic(DX->err().at("Numeric literal has too many decimal points \"" +
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
                                panic(DX->err().at(std::to_string(toNum(ch)) + " is an invalid digit value in base " +
                                                       std::to_string(base),
                                                   curIndex + curCLen));
                            }
                        }
                    } else if (divisor == 1) {
                        panic(DX->err().at("Underscore is not allowed here", curIndex + curCLen));
                    }
                    curCLen++;
                    ch = sourceStr[curIndex + curCLen];
                }
                if (sourceStr[curIndex + curCLen - 1] == '_') {
                    panic(DX->err().at("Underscore is not allowed here", curIndex + curCLen - 1));
                }
                if (overflow && divisor == 0) {
                    DX->warn()
                        .at("Numeric literal is too large to fit in an int ", curIndex, curCLen)
                        .note("The max value for an int literal is 2^31-1 = 2_147_483_647");
                }

                if (divisor > 0) {
                    // TODO make sure there is no precision loss
                    auto tkn = makeToken(TokenType::DOUBLE_LITERAL);
                    tkn->doubleVal = static_cast<double>(number) / divisor;
                    return tkn;
                } else {
                    auto tkn = makeToken(TokenType::INT_LITERAL);
                    tkn->longVal = number;
                    return tkn;
                }
            }
            return makeToken(TokenType::UNKNOWN);
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