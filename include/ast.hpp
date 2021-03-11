#pragma once

#include <memory>
#include <vector>

#include "lexer.hpp"
#include "sym_tab.hpp"

enum class TokenType : unsigned char;
struct Token;

struct SymTable;

enum class NodeType {
    UNKNOWN,

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

std::string nodeTypeToStr(NodeType type);

struct ASTNode;
struct ASTExpression;
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

template <class T>
std::unique_ptr<T> makeNode(int index, int cLen = 1) {
    auto node = std::make_unique<T>();
    node->locIndex = index;
    node->endIndex = index + cLen;
    return node;
}

template <class T1, class T2>
inline std::unique_ptr<T1> castNodePtr(std::unique_ptr<T2>& nodePtr) {
    return std::unique_ptr<T1>(static_cast<T1*>(nodePtr.release()));
}

struct ASTNode {
    NodeType nodeType;
    int locIndex;
    int endIndex;
    explicit ASTNode(NodeType nodeType) : nodeType(nodeType) {}
};

template <class T>
inline std::unique_ptr<T> unknownNode() {
    auto unknown = std::make_unique<T>();
    unknown->nodeType = NodeType::UNKNOWN;
    return unknown;
}

struct ASTExpression : ASTNode {
    explicit ASTExpression() : ASTNode(NodeType::UNKNOWN) {}
    explicit ASTExpression(NodeType nodeType) : ASTNode(nodeType) {}
};

inline std::unique_ptr<ASTExpression> unknownExpr() { return std::make_unique<ASTExpression>(NodeType::UNKNOWN); }

struct ASTProgram : ASTNode {
    std::vector<std::unique_ptr<ASTDecl>> declarations;
    ASTProgram() : ASTNode(NodeType::PROGRAM) {}
};

struct ASTBlock : ASTNode {
    std::unique_ptr<SymTable> symbolTable;
    std::vector<std::unique_ptr<ASTNode>> statements;  // A statement can be anything excluding ASTProgram
    ASTBlock() : ASTNode(NodeType::BLOCK) {}
};

struct ASTIf : ASTNode {
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTNode> conseq;  // Consequence
    std::unique_ptr<ASTNode> alt;     // Alternative can be either an if statement or else block
    ASTIf() : ASTNode(NodeType::IF) {}
};

struct ASTWhile : ASTNode {
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTNode> blockStmt;
    ASTWhile() : ASTNode(NodeType::WHILE) {}
};

struct ASTFor : ASTNode {
    // TODO for loop needs to be more concretely defined
    std::unique_ptr<ASTNode> blockStmt;
    ASTFor() : ASTNode(NodeType::FOR) {}
};

struct ASTBreak : ASTNode {
    ASTBreak() : ASTNode(NodeType::BREAK) {}
};

struct ASTCont : ASTNode {
    ASTCont() : ASTNode(NodeType::CONT) {}
};

struct ASTRet : ASTNode {
    std::unique_ptr<ASTExpression> retValue;
    ASTRet() : ASTNode(NodeType::RET) {}
};

struct ASTDecl : ASTNode {
    std::unique_ptr<ASTExpression> lvalue;
    std::unique_ptr<ASTExpression> type;

    Token* assignType;
    std::unique_ptr<ASTExpression> rvalue;
    ASTDecl() : ASTNode(NodeType::DECL) {}
};

struct ASTTypeLit : ASTExpression {
    TokenType type;
    ASTTypeLit() : ASTExpression(NodeType::TYPE_LIT) {}
};

struct ASTFuncType : ASTExpression {
    std::vector<std::unique_ptr<ASTExpression>> inTypes;
    std::unique_ptr<ASTExpression> outType;
    ASTFuncType() : ASTExpression(NodeType::FUNC_TYPE) {}
};

struct ASTMod : ASTExpression {
    std::vector<std::unique_ptr<ASTDecl>> declarations;
    ASTMod() : ASTExpression(NodeType::MOD) {}
};

struct ASTTy : ASTExpression {
    std::vector<std::unique_ptr<ASTDecl>> declarations;
    ASTTy() : ASTExpression(NodeType::TYPE_DEF) {}
};

struct ASTFunc : ASTExpression {
    std::vector<std::unique_ptr<ASTDecl>> parameters;
    std::unique_ptr<ASTExpression> returnType;
    std::unique_ptr<ASTNode> blockOrExpr;
    ASTFunc() : ASTExpression(NodeType::FUNC) {}
};

struct ASTName : ASTExpression {
    Token* ref;
    ASTName() : ASTExpression(NodeType::NAME) {}
};

struct ASTDotOp : ASTExpression {
    std::unique_ptr<ASTExpression> base;
    std::unique_ptr<ASTName> member;
    ASTDotOp() : ASTExpression(NodeType::DOT_OP) {}
};

struct ASTCall : ASTExpression {
    std::unique_ptr<ASTExpression> callRef;
    std::vector<std::unique_ptr<ASTExpression>> arguments;
    ASTCall() : ASTExpression(NodeType::CALL) {}
};

struct ASTTypeInit : ASTExpression {
    std::unique_ptr<ASTExpression> typeRef;
    std::vector<std::unique_ptr<ASTDecl>> assignments;
    ASTTypeInit() : ASTExpression(NodeType::TYPE_INIT) {}
};

struct ASTLit : ASTExpression {
    Token* value;
    ASTLit() : ASTExpression(NodeType::LIT) {}
};

struct ASTUnOp : ASTExpression {
    Token* op;
    std::unique_ptr<ASTExpression> inner;
    ASTUnOp() : ASTExpression(NodeType::UN_OP) {}
};

struct ASTDeref : ASTExpression {
    std::unique_ptr<ASTExpression> inner;
    ASTDeref() : ASTExpression(NodeType::DEREF) {}
};

struct ASTBinOp : ASTExpression {
    std::unique_ptr<ASTExpression> left;
    Token* op;
    std::unique_ptr<ASTExpression> right;
    ASTBinOp() : ASTExpression(NodeType::BIN_OP) {}
};

void dumpAST(const ASTNode& node, bool verbose);

std::string printExpr(const ASTExpression& expr);
std::string printAST(const ASTNode& node);