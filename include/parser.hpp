#pragma once

#include "ast.hpp"
#include "flags.hpp"
#include "lexer.hpp"
#include "sym_tab.hpp"

class Parser {
    SymTable globalTable;
    Lexer lexer;

    enum { LOWEST_PRECEDENCE = 0, HIGHEST_PRECEDENCE = 100 };
    static int getPrecedence(TokenType type);

    void assertToken(TokenType type, const std::string& msg = "");
    inline bool checkToken(TokenType type) { return lexer.peekToken()->type == type; }

    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTBlock> parseBlock();

    std::unique_ptr<ASTIf> parseIf();
    std::unique_ptr<ASTWhile> parseWhile();
    std::unique_ptr<ASTFor> parseFor();

    std::unique_ptr<ASTNode> parseDeclOrExpr();

    std::unique_ptr<ASTMod> parseModule();
    std::unique_ptr<ASTTy> parseTy();

    std::unique_ptr<ASTExpression> parseLeftParenExpression(int depth);
    std::unique_ptr<ASTExpression> parseFunctionType(std::unique_ptr<ASTExpression>& reduceExpr, int depth);

    std::unique_ptr<ASTExpression> parseExpression();
    std::unique_ptr<ASTExpression> parseOperand(int depth);
    std::unique_ptr<ASTExpression> recurExpression(int prec, int depth);
    std::vector<std::unique_ptr<ASTExpression>> parseExprList();

   public:
    Diagnostics DX{"Parsing", &lexer};

    explicit Parser(Flags flags) : lexer(DX, flags) {}

    std::unique_ptr<ASTProgram> parseProgram();
};