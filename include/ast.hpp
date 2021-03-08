#pragma once

#include <memory>
#include <vector>

#include "lexer.hpp"

enum TokenType : unsigned char;
struct Token;

enum NodeType {
    PROGRAM,
    BLOCK,

    IF,
    WHILE,
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

struct ASTNode {
    const NodeType nodeType;
    int locIndex;
    int endIndex;
    explicit ASTNode(NodeType nodeType) : nodeType(nodeType) {}
};

struct ASTExpression : ASTNode {
    explicit ASTExpression(NodeType nodeType) : ASTNode(nodeType) {}
};

template <class T>
std::unique_ptr<T> makeNode(int index) {
    auto node = std::make_unique<T>();
    node->locIndex = index;
    node->endIndex = index + 1;
    return node;
}

template <class T1, class T2>
inline std::unique_ptr<T1> castNodePtr(std::unique_ptr<T2>& nodePtr) {
    return std::unique_ptr<T1>(static_cast<T1*>(nodePtr.release()));
}

struct ASTProgram;
struct ASTBlock;
struct ASTIF;
struct ASTWhile;
struct ASTFor;
struct ASTBreak;
struct ASTCont;
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

struct ASTProgram : ASTNode {
    std::vector<std::unique_ptr<ASTDecl>> declarations;
    ASTProgram() : ASTNode(PROGRAM) {}
};

struct ASTBlock : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;  // A statement can be anything excluding ASTProgram
    ASTBlock() : ASTNode(BLOCK) {}
};

struct ASTIf : ASTNode {
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTNode> conseq;  // Consequence
    std::unique_ptr<ASTNode> alt;     // Alternative can be either an if statement or else block
    ASTIf() : ASTNode(IF) {}
};

struct ASTWhile : ASTNode {
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTNode> blockStmt;
    ASTWhile() : ASTNode(WHILE) {}
};

struct ASTFor : ASTNode {
    // TODO for loop needs to be more concretely defined
    std::unique_ptr<ASTNode> blockStmt;
    ASTFor() : ASTNode(FOR) {}
};

struct ASTBreak : ASTNode {
    ASTBreak() : ASTNode(BREAK) {}
};

struct ASTCont : ASTNode {
    ASTCont() : ASTNode(CONT) {}
};

struct ASTRet : ASTNode {
    std::unique_ptr<ASTExpression> retValue;
    ASTRet() : ASTNode(RET) {}
};

struct ASTDecl : ASTNode {
    std::unique_ptr<ASTExpression> lvalue;
    std::unique_ptr<ASTExpression> type;

    Token* assignType;
    std::unique_ptr<ASTExpression> rvalue;
    ASTDecl() : ASTNode(DECL) {}
};

struct ASTTypeLit : ASTExpression {
    TokenType type;
    ASTTypeLit() : ASTExpression(TYPE_LIT) {}
};

struct ASTFuncType : ASTExpression {
    std::vector<std::unique_ptr<ASTExpression>> inTypes;
    std::unique_ptr<ASTExpression> outType;
    ASTFuncType() : ASTExpression(FUNC_TYPE) {}
};

struct ASTMod : ASTExpression {
    std::vector<std::unique_ptr<ASTDecl>> declarations;
    ASTMod() : ASTExpression(MOD) {}
};

struct ASTTy : ASTExpression {
    std::vector<std::unique_ptr<ASTDecl>> declarations;
    ASTTy() : ASTExpression(TYPE_DEF) {}
};

struct FuncParam {
    std::unique_ptr<ASTName> paramRef;
    std::unique_ptr<ASTExpression> type;
};

struct ASTFunc : ASTExpression {
    std::vector<std::unique_ptr<FuncParam>> parameters;
    std::unique_ptr<ASTExpression> returnType;
    std::unique_ptr<ASTNode> blockOrExpr;
    ASTFunc() : ASTExpression(FUNC) {}
};

struct ASTName : ASTExpression {
    Token* ref;
    ASTName() : ASTExpression(NAME) {}
};

struct ASTDotOp : ASTExpression {
    std::unique_ptr<ASTExpression> base;
    std::unique_ptr<ASTName> member;
    ASTDotOp() : ASTExpression(DOT_OP) {}
};

struct ASTCall : ASTExpression {
    std::unique_ptr<ASTExpression> callRef;
    std::vector<std::unique_ptr<ASTExpression>> arguments;
    ASTCall() : ASTExpression(CALL) {}
};

struct ASTTypeInit : ASTExpression {
    std::unique_ptr<ASTExpression> typeRef;
    std::vector<std::unique_ptr<ASTDecl>> assignments;
    ASTTypeInit() : ASTExpression(TYPE_INIT) {}
};

struct ASTLit : ASTExpression {
    Token* value;
    ASTLit() : ASTExpression(LIT) {}
};

struct ASTUnOp : ASTExpression {
    Token* op;
    std::unique_ptr<ASTExpression> inner;
    ASTUnOp() : ASTExpression(UN_OP) {}
};

struct ASTDeref : ASTExpression {
    std::unique_ptr<ASTExpression> inner;
    ASTDeref() : ASTExpression(DEREF) {}
};

struct ASTBinOp : ASTExpression {
    std::unique_ptr<ASTExpression> left;
    Token* op;
    std::unique_ptr<ASTExpression> right;
    ASTBinOp() : ASTExpression(BIN_OP) {}
};

void dumpAST(const ASTNode& node, bool verbose);

std::string printExpr(const ASTExpression& expr);
std::string printAST(const ASTNode& node);
