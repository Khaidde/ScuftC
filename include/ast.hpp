#pragma once

#include <vector>

#include "lexer.hpp"

enum NodeType {
    PROGRAM,
    BLOCK,

    IF,
    WHILE,
    FOR,
    BREAK,
    CONTINUE,
    RETURN,

    DECLARATION,

    TYPE_LITERAL,
    FUNCTION_TYPE,

    MODULE,
    TYPE_DEF,

    FUNCTION,

    NAME,
    DOT_OP,
    CALL,
    TYPE_CONSTRUCT,

    LITERAL,
    UNARY_OP,
    BINARY_OP
};

template <class T>
std::unique_ptr<T> makeNode(int locIndex) {
    auto node = std::make_unique<T>();
    node->locIndex = locIndex;
    return node;
};

struct ASTNode {
    NodeType type;
    int locIndex;
    int nodeCLen;
    ASTNode(NodeType type) : type(type) {}
};

struct ASTDeclaration;

struct ASTProgram : ASTNode {
    std::vector<ASTDeclaration> declarations;
    ASTProgram() : ASTNode(PROGRAM) {}
};

struct ASTDeclaration : ASTNode {
    ASTDeclaration() : ASTNode(DECLARATION) {}
};
