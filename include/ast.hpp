#pragma once

#include <string>

#include "lexer.hpp"
#include "utils.hpp"

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
    SList<ASTNode*> declarations;
};

struct ASTBlock {
    SList<ASTNode*> statements;  // A statement can be anything excluding ASTProgram
};

struct ASTIf {
    ASTNode* condition;
    ASTNode* conseq;  // Consequence
    ASTNode* alt;     // Alternative can be either an if statement or else block
};

struct ASTFor {
    ASTNode* initial;
    ASTNode* condition;
    ASTNode* post;

    ASTNode* blockStmt;
};

struct ASTRet {
    ASTNode* retExpr;
};

struct ASTDecl {
    ASTNode* lvalue;
    ASTNode* type;

    Token* assignType;
    ASTNode* rvalue;
};

struct ASTTypeLit {
    TokenType type;
};

struct ASTFuncType {
    SList<ASTNode*> inTypes;
    ASTNode* outType;
};

struct ASTMod {
    SList<ASTNode*> declarations;
};

struct ASTTy {
    SList<ASTNode*> declarations;
};

struct ASTFunc {
    SList<ASTNode*> parameters;
    ASTNode* returnType;

    bool isShorthand;
    union {
        ASTNode* block;
        ASTNode* expr;
    };
};

struct ASTName {
    Token* ref;
};

struct ASTDotOp {
    ASTNode* base;
    ASTNode* member;  // Always an ASTName
};

struct ASTCall {
    ASTNode* callRef;
    SList<ASTNode*> arguments;
};

struct ASTTypeInit {
    ASTNode* typeRef;
    SList<ASTNode*> assignments;
};

struct ASTLit {
    Token* value;
};

struct ASTUnOp {
    Token* op;
    ASTNode* inner;
};

struct ASTDeref {
    ASTNode* inner;
};

struct ASTBinOp {
    ASTNode* left;
    Token* op;
    ASTNode* right;
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

inline ASTNode* unknown_node(int beginI, int endI) {
    ASTNode* unknown = new ASTNode();
    unknown->nodeType = NodeType::UNKNOWN;
    unknown->beginI = beginI;
    unknown->endI = endI;
    return unknown;
}

inline ASTNode* make_node(NodeType nodeType, const Token* tkn) {
    ASTNode* node = new ASTNode();
    node->nodeType = nodeType;
    node->beginI = tkn->beginI;
    node->endI = tkn->endI;
    return node;
}

inline ASTNode* make_node(NodeType nodeType, const ASTNode* nodeLoc) {
    ASTNode* node = new ASTNode();
    node->nodeType = nodeType;
    node->beginI = nodeLoc->beginI;
    node->endI = nodeLoc->endI;
    return node;
}