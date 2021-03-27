#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"

enum class BaseType : unsigned char {
    UNKNOWN,
    DEF_TYPE,  // User defined type like "Point"
    FN_PROTO,  // (Int, Int) -> Void

    VOID,
    MODULE,
    TYPE,  // the type of "Point"
    INT,
    DOUBLE,
    STRING,
    BOOL,
};

struct ExprType {
    BaseType baseType;

    union {
        struct {
            const char* name;
        } definedType;

        struct {
            ExprType* paramTypes;
            ExprType* returnType;
        } fnProtoType;
    };
};

enum class IRType : unsigned char {
    UNKNOWN,
    NONE,
    DECL_DEF,
    EXPR,
    VAR_YIELD,
    BIN_OP,
};

struct IRInstruct;

struct IRDeclDef {
    ASTDecl* declNode;
    bool isAnalyzing;  // Detect declaration cycles
};

struct IRVarYield {
    ASTNode* nameNode;
};

struct IRExpression {
    ASTNode* exprNode;
};

enum class BinOpType {
    ADD,
    SUBTR,
    MULT,
    DIV,
};

struct IRBinOp {
    ASTNode* binOpNode;

    int leftIndex;
    int rightIndex;
    BinOpType opType;
};

struct IRInstruct {
    int index;
    bool isConst;
    IRType instrType = IRType::UNKNOWN;

    ExprType type = {BaseType::UNKNOWN};

    union {
        IRDeclDef declDef;
        IRVarYield varYield;
        IRExpression exprRef;
        IRBinOp binOp;
    };
};

class Analyzer {
    Diagnostics& dx;

    void flatten_program(ASTNode* prgmNode);
    void flatten_decl(ASTNode* declNode);

    void flatten_expr(ASTNode* exprNode);

   public:
    std::vector<IRInstruct> lineInstrs;

    explicit inline Analyzer(Diagnostics& dx) : dx(dx) {}
    void analyze(ASTNode* prgmNode);

    void ir_print();
};