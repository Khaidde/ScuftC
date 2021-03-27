#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "sym_tab.hpp"

enum class NodeType : unsigned char {
    UNKNOWN,

    PROGRAM,
    BLOCK,

    IF,
    FOR,
    BREAK,
    CONT,
    RET,

    DECL,

    TYPE_LIT,
    FUNC_TYPE,

    MOD,
    TYPE_DEF,

    FUNC,

    NAME,
    DOT_OP,
    CALL,
    TYPE_INIT,

    LIT,
    UN_OP,
    DEREF,
    BIN_OP
};

struct ASTNode;
struct ASTProgram;
struct ASTBlock;
struct ASTIf;
struct ASTFor;
struct ASTRet;
struct ASTDecl;
struct ASTTypeLit;
struct ASTFuncType;
struct ASTMod;
struct ASTTy;
struct ASTFunc;
struct ASTName;
struct ASTDotOp;
struct ASTCall;
struct ASTTypeInit;
struct ASTLit;
struct ASTUnOp;
struct ASTDeref;
struct ASTBinOp;

struct ASTProgram {
    // ASTNode* declarations{nullptr};
    // int size = 0;
    // int capacity = 0;
    std::vector<std::unique_ptr<ASTNode>> declarations;
};

struct ASTBlock {
    std::unique_ptr<SymTable> symbolTable;
    std::vector<std::unique_ptr<ASTNode>> statements;  // A statement can be anything excluding ASTProgram
};

struct ASTIf {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> conseq;  // Consequence
    std::unique_ptr<ASTNode> alt;     // Alternative can be either an if statement or else block
};

struct ASTFor {
    std::unique_ptr<ASTNode> initial;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> post;

    std::unique_ptr<ASTNode> blockStmt;
};

struct ASTRet {
    std::unique_ptr<ASTNode> retExpr;
};

struct ASTDecl {
    std::unique_ptr<ASTNode> lvalue;
    std::unique_ptr<ASTNode> type;

    Token* assignType;
    std::unique_ptr<ASTNode> rvalue;
};

struct ASTTypeLit {
    TokenType type;
};

struct ASTFuncType {
    std::vector<std::unique_ptr<ASTNode>> inTypes;
    std::unique_ptr<ASTNode> outType;
};

struct ASTMod {
    std::vector<std::unique_ptr<ASTNode>> declarations;
};

struct ASTTy {
    std::vector<std::unique_ptr<ASTNode>> declarations;
};

struct ASTFunc {
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> returnType;

    bool isShorthand;
    union {
        std::unique_ptr<ASTNode> block;
        std::unique_ptr<ASTNode> expr;
    };
};

struct ASTName {
    Token* ref;
};

struct ASTDotOp {
    std::unique_ptr<ASTNode> base;
    std::unique_ptr<ASTNode> member;  // Always an ASTName
};

struct ASTCall {
    std::unique_ptr<ASTNode> callRef;
    std::vector<std::unique_ptr<ASTNode>> arguments;
};

struct ASTTypeInit {
    std::unique_ptr<ASTNode> typeRef;
    std::vector<std::unique_ptr<ASTNode>> assignments;
};

struct ASTLit {
    Token* value;
};

struct ASTUnOp {
    Token* op;
    std::unique_ptr<ASTNode> inner;
};

struct ASTDeref {
    std::unique_ptr<ASTNode> inner;
};

struct ASTBinOp {
    std::unique_ptr<ASTNode> left;
    Token* op;
    std::unique_ptr<ASTNode> right;
};

struct ASTNode {
    NodeType nodeType;
    int beginI;
    int endI;

    union {
        ASTProgram prgm;
        ASTBlock block;
        ASTIf ifNode;
        ASTFor forLoop;
        ASTRet ret;
        ASTDecl decl;
        ASTTypeLit typeLit;
        ASTFuncType funcType;
        ASTMod mod;
        ASTTy ty;
        ASTFunc func;
        ASTName name;
        ASTDotOp dotOp;
        ASTCall call;
        ASTTypeInit typeInit;
        ASTLit lit;
        ASTUnOp unOp;
        ASTDeref deref;
        ASTBinOp binOp;
    };
};

inline bool is_expression_type(NodeType nodeType) { return nodeType >= NodeType::TYPE_LIT; }

inline std::unique_ptr<ASTNode> unknown_node(int beginI, int endI) {
    auto unknown = std::make_unique<ASTNode>();
    unknown->nodeType = NodeType::UNKNOWN;
    unknown->beginI = beginI;
    unknown->endI = endI;
    return unknown;
}

inline std::unique_ptr<ASTNode> make_node(NodeType nodeType, const Token* tkn) {
    auto node = std::make_unique<ASTNode>();
    node->nodeType = nodeType;
    node->beginI = tkn->beginI;
    node->endI = tkn->endI;
    return node;
}

inline std::unique_ptr<ASTNode> make_node(NodeType nodeType, const std::unique_ptr<ASTNode>& nodeLoc) {
    auto node = std::make_unique<ASTNode>();
    node->nodeType = nodeType;
    node->beginI = nodeLoc->beginI;
    node->endI = nodeLoc->endI;
    return node;
}