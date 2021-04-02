#include "lexer.hpp"

#include <fstream>

#include "flags.hpp"

std::string token_type_to_str(TokenType type) {
    switch (type) {
        case TokenType::UNKNOWN:
            return "'unknown token'";
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
        case TokenType::FOR:
            return "for";
        case TokenType::BREAK:
            return "break";
        case TokenType::CONTINUE:
            return "continue";
        case TokenType::IDENTIFIER:
            return "'identifier'";
        case TokenType::COLON:
            return ":";
        case TokenType::VOID_TYPE:
            return "Void";
        case TokenType::MOD_TYPE:
            return "Module";
        case TokenType::TY_TYPE:
            return "Type";
        case TokenType::INT_TYPE:
            return "Int";
        case TokenType::DOUBLE_TYPE:
            return "Double";
        case TokenType::STRING_TYPE:
            return "String";
        case TokenType::BOOL_TYPE:
            return "Bool";
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
            ASSERT(false, "Unknown token");
    }
}

bool Lexer::from_file_path(const char* filePath) {
    std::ifstream file(filePath);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        this->sourceStr = std::string(size, ' ');
        file.seekg(0);
        file.read(&this->sourceStr[0], size);

        curIndex = 0;
        curCLen = 1;
        return true;
    } else {
        return false;
    }
}

Token* Lexer::make_token(TokenType type) {
    Token* tkn = new Token();
    tkn->type = type;
    tkn->beginI = curIndex;
    tkn->endI = curIndex + curCLen;

    curIndex += curCLen;
    curCLen = 1;
    return tkn;
}

Token* Lexer::peek_token() {
    if (cacheIndex >= tokenCache.size) tokenCache.push(consume_token());
    return tokenCache[cacheIndex];
}

Token* Lexer::next_token() {
    Token* token = peek_token();
    if (token->type != TokenType::END) cacheIndex++;
    return token;
}

Token* Lexer::last_token() {
    ASSERT(cacheIndex > 0, "Can't get last token of the first token in the file.");
    return tokenCache[cacheIndex - 1];
}

Token* Lexer::consume_token() {
    if (curIndex < sourceStr.length() && sourceStr[curIndex] == '\r') {
        dx.err_loc("\\r is not a supported character in this language", curIndex)->note("Use \\n instead");
        return make_token(TokenType::UNKNOWN);
    }

    // Skip over whitespace
    while (curIndex < sourceStr.length() && is_whitespace(sourceStr[curIndex])) curIndex++;

    if (curIndex >= sourceStr.length()) return make_token(TokenType::END);

    switch (sourceStr[curIndex]) {
        case ';':
            if (!Flags::dwSemiColons) {
                dx.err_loc("Semi-colons are not required in this language", curIndex)
                    ->tag(ErrorMsg::WARNING)
                    ->note("Semi-colons are treated as whitespace. Use -dw-semi-colons to disable warning");
            }
            curIndex++;
            return consume_token();
        case '{':
            return make_token(TokenType::LEFT_CURLY);
        case '}':
            return make_token(TokenType::RIGHT_CURLY);
        case '(':
            return make_token(TokenType::LEFT_PARENS);
        case ')':
            return make_token(TokenType::RIGHT_PARENS);
        case ':':
            if (is_cursor_char(':')) {
                curCLen++;
                return make_token(TokenType::SINGLE_RETURN);
            } else {
                return make_token(TokenType::COLON);
            }
        case '=':
            if (is_cursor_char('>')) {
                curCLen++;
                return make_token(TokenType::CONST_ASSIGNMENT);
            } else if (is_cursor_char('=')) {
                curCLen++;
                return make_token(TokenType::COND_EQUALS);
            } else {
                return make_token(TokenType::ASSIGNMENT);
            }
        case '!':
            if (is_cursor_char('=')) {
                curCLen++;
                return make_token(TokenType::COND_NOT_EQUALS);
            } else {
                return make_token(TokenType::COND_NOT);
            }
        case '|':
            if (is_cursor_char('|')) {
                curCLen++;
                return make_token(TokenType::COND_OR);
            } else {
                return make_token(TokenType::BIT_OR);
            }
        case '&':
            if (is_cursor_char('&')) {
                curCLen++;
                return make_token(TokenType::COND_AND);
            } else {
                return make_token(TokenType::BIT_AND);
            }
        case '$':
            if (is_cursor_char('$')) {
                curCLen++;
                return make_token(TokenType::COND_XOR);
            } else {
                return make_token(TokenType::BIT_XOR);
            }
        case '<':
            if (is_cursor_char('=')) {
                curCLen++;
                return make_token(TokenType::COND_LESS_EQUAL);
            } else if (is_cursor_char('<')) {
                curCLen++;
                return make_token(TokenType::BIT_SHIFT_LEFT);
            } else {
                return make_token(TokenType::COND_LESS);
            }
        case '>':
            if (is_cursor_char('=')) {
                curCLen++;
                return make_token(TokenType::COND_GREATER_EQUAL);
            } else if (is_cursor_char('>')) {
                curCLen++;
                return make_token(TokenType::BIT_SHIFT_RIGHT);
            } else {
                return make_token(TokenType::COND_GREATER);
            }
        case '.':
            if (is_cursor_char('*')) {
                curCLen++;
                return make_token(TokenType::DEREF);
            } else {
                return make_token(TokenType::DOT);
            }
        case '+':
            if (is_cursor_char('=')) {
                curCLen++;
                return make_token(TokenType::OP_ADD_EQUAL);
            } else if (is_cursor_char('+')) {
                curCLen++;
                return make_token(TokenType::OP_ADD_ADD);
            } else {
                return make_token(TokenType::OP_ADD);
            }
        case '-':
            if (is_cursor_char('=')) {
                curCLen++;
                return make_token(TokenType::OP_SUBTR_EQUAL);
            } else if (is_cursor_char('-')) {
                curCLen++;
                return make_token(TokenType::OP_SUBTR_SUBTR);
            } else if (is_cursor_char('>')) {
                curCLen++;
                return make_token(TokenType::ARROW);
            } else {
                return make_token(TokenType::OP_SUBTR);
            }
        case '*':
            if (is_cursor_char('*')) {
                curCLen++;
                return make_token(TokenType::OP_MULT_EQUAL);
            } else if (is_cursor_char('/')) {
                curCLen++;
                dx.err_loc("Invalid closing of block comment", curIndex);
                return make_token(TokenType::UNKNOWN);
            } else {
                return make_token(TokenType::OP_MULT);
            }
        case '/':
            if (is_cursor_char('=')) {
                curCLen++;
                return make_token(TokenType::OP_DIV_EQUAL);
            } else if (is_cursor_char('/')) {
                while (curIndex < sourceStr.length() && sourceStr[curIndex] != '\n') {
                    curIndex++;
                }
                curIndex++;
                curCLen = 1;
                return consume_token();
            } else if (is_cursor_char('*')) {
                while (curIndex + curCLen < sourceStr.length() &&
                       !(sourceStr[curIndex + curCLen - 1] == '*' && is_cursor_char('/'))) {
                    curCLen++;
                }
                curCLen++;
                if (curIndex + curCLen > sourceStr.length()) {
                    dx.err_loc("Unterminated block comment", curIndex);
                    return make_token(TokenType::UNKNOWN);
                }
                curIndex += curCLen;
                curCLen = 1;
                return consume_token();
            } else {
                return make_token(TokenType::OP_DIV);
            }
        case '%':
            if (is_cursor_char('=')) {
                curCLen++;
                return make_token(TokenType::OP_MOD_EQUAL);
            } else {
                return make_token(TokenType::OP_MOD);
            }
        case '^':
            return make_token(TokenType::OP_CARROT);
        case ',':
            return make_token(TokenType::COMMA);
        case '"': {
            while (curIndex + curCLen < sourceStr.length() && !is_cursor_char('"')) {
                if (is_cursor_char('\\')) curCLen++;
                curCLen++;
            }
            curCLen++;
            if (curIndex + curCLen > sourceStr.length()) {
                dx.err_loc("Unterminated string literal", curIndex);
                return make_token(TokenType::UNKNOWN);
            }
            auto tkn = make_token(TokenType::STRING_LITERAL);
            tkn->sourceStr = &sourceStr;
            return tkn;
        }
        default:
            if (is_letter(sourceStr[curIndex])) {
                while (is_letter(sourceStr[curIndex + curCLen]) || is_number(sourceStr[curIndex + curCLen])) {
                    curCLen++;
                }
                std::string_view keyword(sourceStr.c_str() + curIndex, curCLen);
                if (keyword == "mod") {
                    return make_token(TokenType::MOD);
                } else if (keyword == "ty") {
                    return make_token(TokenType::TY);
                } else if (keyword == "if") {
                    return make_token(TokenType::IF);
                } else if (keyword == "else") {
                    return make_token(TokenType::ELSE);
                } else if (keyword == "for") {
                    return make_token(TokenType::FOR);
                } else if (keyword == "while") {
                    dx.err_loc("While loops are not allowed in this language", curIndex, curIndex + curCLen)
                        ->fix("Use for loop instead in the form: for [condition] {}")
                        ->note("Using 'while' as a variable name can cause confusion with other languages");
                } else if (keyword == "break") {
                    return make_token(TokenType::BREAK);
                } else if (keyword == "continue") {
                    return make_token(TokenType::CONTINUE);
                } else if (keyword == "true") {
                    return make_token(TokenType::TRUE);
                } else if (keyword == "false") {
                    return make_token(TokenType::FALSE);
                } else if (keyword == "Void") {
                    return make_token(TokenType::VOID_TYPE);
                } else if (keyword == "Module") {
                    return make_token(TokenType::MOD_TYPE);
                } else if (keyword == "Type") {
                    return make_token(TokenType::TY_TYPE);
                } else if (keyword == "Int") {
                    return make_token(TokenType::INT_TYPE);
                } else if (keyword == "Double") {
                    return make_token(TokenType::DOUBLE_TYPE);
                } else if (keyword == "String") {
                    return make_token(TokenType::STRING_TYPE);
                } else if (keyword == "Bool") {
                    return make_token(TokenType::BOOL_TYPE);
                } else if (keyword == "return") {
                    return make_token(TokenType::RETURN);
                } else {
                    auto tkn = make_token(TokenType::IDENTIFIER);
                    tkn->sourceStr = &sourceStr;
                    return tkn;
                }
            } else if (is_number(sourceStr[curIndex])) {
                int base = 10;
                if (sourceStr[curIndex] == '0') {
                    if (is_cursor_char('b')) {
                        base = 2;
                        curCLen++;
                    } else if (is_cursor_char('o')) {
                        base = 8;
                        curCLen++;
                    } else if (is_cursor_char('x')) {
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
                while ((base == 16 && is_hex(ch)) || is_number(ch) || ch == '.') {
                    if (ch != '_') {
                        if (ch == '.') {
                            if (sourceStr[curIndex + curCLen - 1] == '_') {
                                dx.err_loc("Underscore is not allowed here", curIndex + curCLen - 1);
                            }
                            if (divisor > 0) {
                                dx.err_loc("Numeric literal has too many decimal points \"" +
                                               sourceStr.substr(curIndex, curCLen) + ".\"",
                                           curIndex + curCLen);
                            } else {
                                divisor = 1;
                            }
                        } else {
                            // Convert digit into corresponding base: (in hexadecimal) A -> 10
                            int val;
                            if (to_num(ch) >= 0 && to_num(ch) < 10) {
                                val = to_num(ch);
                            } else if (is_hex(ch)) {
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
                                dx.err_loc(std::to_string(to_num(ch)) + " is an invalid digit value in base " +
                                               std::to_string(base),
                                           curIndex + curCLen);
                            }
                        }
                    } else if (divisor == 1) {
                        dx.err_loc("Underscore is not allowed here", curIndex + curCLen);
                    }
                    curCLen++;
                    ch = sourceStr[curIndex + curCLen];
                }
                if (sourceStr[curIndex + curCLen - 1] == '_') {
                    dx.err_loc("Underscore is not allowed here", curIndex + curCLen - 1);
                }
                if (overflow && divisor == 0) {
                    dx.err_loc("Numeric literal is too large to fit in an int ", curIndex, curIndex + curCLen)
                        ->tag(ErrorMsg::WARNING)
                        ->note("The max value for an int literal is 2^31-1 = 2_147_483_647");
                }

                if (divisor > 0) {
                    // TODO make sure there is no precision loss
                    auto tkn = make_token(TokenType::DOUBLE_LITERAL);
                    tkn->doubleVal = static_cast<double>(number) / divisor;
                    return tkn;
                } else {
                    auto tkn = make_token(TokenType::INT_LITERAL);
                    tkn->longVal = number;
                    return tkn;
                }
            }
            return make_token(TokenType::UNKNOWN);
    }
}