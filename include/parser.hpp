#pragma once

#include <memory>

#include "ast.hpp"
#include "diagnostics.hpp"
#include "lexer.hpp"

class Parser {
    void assert_token(TokenType type, const std::string& msg = "");
    inline bool check_token(TokenType type) { return lexer.peek_token()->type == type; }

    std::unique_ptr<ASTNode> parse_statement();
    std::unique_ptr<ASTNode> parse_block();

    std::unique_ptr<ASTNode> parse_if();
    std::unique_ptr<ASTNode> parse_for();

    std::unique_ptr<ASTNode> parse_return();

    std::unique_ptr<ASTNode> assert_parse_decl();
    std::unique_ptr<ASTNode> parse_decl_expr();

    std::unique_ptr<ASTNode> parse_left_paren_expr();
    std::unique_ptr<ASTNode> parse_function_type(std::unique_ptr<ASTNode>& reduceExpr);

    int exprDepth = 0;
    int unbalancedParenErrI = 0;
    std::unique_ptr<ASTNode> parse_expr();
    std::unique_ptr<ASTNode> parse_operand();
    std::unique_ptr<ASTNode> recur_expr(int prec);

   public:
    Lexer lexer;
    Diagnostics& dx;
    Parser() : dx(lexer.dx) {}

    std::unique_ptr<ASTNode> parse_program();
};