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
};

struct IRVarYield {
    ASTExpression* nameNode;
};

struct IRExpression {
    ASTExpression* exprNode;
};

enum class BinOpType {
    ADD,
    SUBTR,
    MULT,
    DIV,
};

struct IRBinOp {
    ASTExpression* binOpNode;

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

    int traverse_get_ir_size(ASTNode& node);

    void flatten_program(ASTProgram& prgm);
    void flatten_decl(ASTDecl& decl);

    void flatten_expr(ASTExpression& expr);

   public:
    std::vector<IRInstruct> lineInstrs;

    explicit inline Analyzer(Diagnostics& dx) : dx(dx) {}
    void analyze(ASTProgram& prgm);

    void ir_print();
};