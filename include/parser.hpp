#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"
#include "lexer.hpp"

class Parser {
    void assert_token(TokenType type, const std::string& msg = "");
    inline bool check_token(TokenType type) { return lexer.peek_token()->type == type; }

    ASTNode* parse_statement();
    ASTNode* parse_block();

    ASTNode* parse_if();
    ASTNode* parse_for();

    ASTNode* parse_return();

    ASTNode* assert_parse_decl();
    ASTNode* parse_decl_expr();

    ASTNode* parse_left_paren_expr();
    ASTNode* parse_function_type(ASTNode* reduceExpr);

    int exprDepth = 0;
    int unbalancedParenErrI = 0;
    ASTNode* parse_expr();
    ASTNode* parse_operand();
    ASTNode* recur_expr(int prec);

   public:
    Lexer lexer;
    Diagnostics& dx;
    Parser() : dx(lexer.dx) {}

    ASTNode* parse_program();
};