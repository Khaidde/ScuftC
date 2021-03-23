#pragma once

#include <memory>
#include <string>
#include <vector>

#include "sym_tab.hpp"

enum class TokenType : unsigned char;
struct Token;

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

std::string node_type_to_str(NodeType type);

struct ASTNode;
struct ASTExpression;
struct ASTProgram;
struct ASTBlock;
struct ASTIF;
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
struct ASTEmpty;
struct ASTName;
struct ASTDotOp;
struct ASTCall;
struct ASTTypeInit;
struct ASTLit;
struct ASTUnOp;
struct ASTDeref;
struct ASTBinOp;

inline bool is_expression_type(NodeType nodeType) { return nodeType >= NodeType::TYPE_LIT; }

template <class T>
inline std::unique_ptr<T> make_node(const Token& tkn) {
    auto node = std::make_unique<T>();
    node->beginI = tkn.beginI;
    node->endI = tkn.endI;
    return node;
}

template <class T>
inline std::unique_ptr<T> make_node(const ASTNode& nodeLoc) {
    auto node = std::make_unique<T>();
    node->beginI = nodeLoc.beginI;
    node->endI = nodeLoc.endI;
    return node;
}

template <class T1, class T2>
inline std::unique_ptr<T1> cast_node_ptr(std::unique_ptr<T2>& nodePtr) {
    return std::unique_ptr<T1>(static_cast<T1*>(nodePtr.release()));
}

struct ASTNode {
    NodeType nodeType;
    int beginI;
    int endI;
    explicit ASTNode(NodeType nodeType) : nodeType(nodeType) {}
};

template <class T>
inline std::unique_ptr<T> unknown_node(int beginI, int endI) {
    auto unknown = std::make_unique<T>();
    unknown->nodeType = NodeType::UNKNOWN;
    unknown->beginI = beginI;
    unknown->endI = endI;
    return unknown;
}

struct ASTExpression : ASTNode {
    explicit ASTExpression() : ASTNode(NodeType::UNKNOWN) {}
    explicit ASTExpression(NodeType nodeType) : ASTNode(nodeType) {}
};

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

struct ASTFor : ASTNode {
    std::unique_ptr<ASTNode> initial;
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTNode> post;

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

void dump_ast(const ASTNode& node, bool verbose);

std::string print_ast(const ASTNode& node);
std::string print_expr(const ASTExpression& expr);