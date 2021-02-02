#pragma once

#include <map>

#include "ast.hpp"
#include "lexer.hpp"

class Parser {
    typedef std::map<TokenType, int> PrecedenceMap;
    static PrecedenceMap precedence;
    static PrecedenceMap initPrecedenceMap() {
        PrecedenceMap map;
        // TODO add the rest of the operators here
        map[OP_ADD_TKN] = 10;
        map[OP_SUBTR_TKN] = 10;

        map[OP_MULT_TKN] = 11;
        map[OP_DIV_TKN] = 11;
        return map;
    }

    Lexer lexer;

   public:
    std::unique_ptr<ASTProgram> parseProgram();
};