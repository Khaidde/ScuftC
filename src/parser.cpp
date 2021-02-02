#include "parser.hpp"

Parser::PrecedenceMap Parser::precedence = initPrecedenceMap();

std::unique_ptr<ASTProgram> Parser::parseProgram() {
    auto prgm = makeNode<ASTProgram>(0);
    return prgm;
}