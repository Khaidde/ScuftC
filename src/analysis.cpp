#include "analysis.hpp"

#include <stdio.h>

#include <stdexcept>

#include "diagnostics.hpp"
#include "lexer.hpp"

static const char* expr_type_to_str(ExprType exprType) {
    switch (exprType.baseType) {
        case BaseType::UNKNOWN:
            return "_";
        case BaseType::VOID:
            return "Void";
        case BaseType::MODULE:
            return "Module";
        case BaseType::TYPE:
            return "Type";
        case BaseType::INT:
            return "Int";
        case BaseType::DOUBLE:
            return "Double";
        case BaseType::STRING:
            return "String";
        case BaseType::BOOL:
            return "Bool";
        default:
            ASSERT(false, "Unimplemented expr type to str");
    }
}

void Analyzer::analyze(ASTProgram& prgm) {
    // TODO lineInstrs.reserve(traverse_get_ir_size(prgm));

    flatten_program(prgm);
    // printf("%dl", lineInstrs.capacity());
    ir_print();
}

// TODO
int Analyzer::traverse_get_ir_size(ASTNode& node) {
    int size = 0;
    switch (node.nodeType) {
        case NodeType::PROGRAM: {
            auto& prgm = static_cast<ASTProgram&>(node);
            for (auto& decl : prgm.declarations) {
                size += traverse_get_ir_size(*decl);
            }
        } break;
        case NodeType::DECL: {
            auto& decl = static_cast<ASTDecl&>(node);
            size += 1;
            // TODO get size of decl.lvalue and decl.type
            size += traverse_get_ir_size(*decl.rvalue);
        } break;
        case NodeType::CALL: {
            auto& call = static_cast<ASTCall&>(node);
            size += 1;
            for (auto& arg : call.arguments) {
                size += traverse_get_ir_size(*arg);
            }
        } break;
        case NodeType::BIN_OP: {
            auto& binOp = static_cast<ASTBinOp&>(node);
            size += 1;
            size += traverse_get_ir_size(*binOp.left);
            size += traverse_get_ir_size(*binOp.right);
        } break;
        case NodeType::TYPE_LIT:
        case NodeType::NAME:
        case NodeType::LIT:
            return 1;
        default:
            ASSERT(false, "Can't get IR size for unknown nodetype: " + node_type_to_str(node.nodeType));
    }
    return size;
}

void Analyzer::flatten_program(ASTProgram& prgm) {
    for (auto& decl : prgm.declarations) {
        flatten_decl(*decl);
    }
}

void Analyzer::flatten_decl(ASTDecl& decl) {
    // TODO flatten_expr on lvalue

    if (decl.rvalue != nullptr) flatten_expr(*decl.rvalue);

    IRInstruct instr{};
    instr.index = lineInstrs.size();
    instr.instrType = IRType::DECL_DEF;
    instr.declDef.declNode = &decl;
    lineInstrs.push_back(std::move(instr));
}

void Analyzer::flatten_expr(ASTExpression& expr) {
    IRInstruct instr{};
    switch (expr.nodeType) {
        case NodeType::NAME: {
            auto& name = static_cast<ASTName&>(expr);
            instr.index = lineInstrs.size();
            instr.instrType = IRType::VAR_YIELD;
            instr.varYield.nameNode = &expr;
        } break;
        case NodeType::LIT: {
            auto& lit = static_cast<ASTLit&>(expr);

            instr.index = lineInstrs.size();
            instr.instrType = IRType::EXPR;
            switch (lit.value->type) {
                case TokenType::INT_LITERAL:
                    instr.type.baseType = BaseType::INT;
                    break;
                case TokenType::DOUBLE_LITERAL:
                    instr.type.baseType = BaseType::DOUBLE;
                    break;
                case TokenType::TRUE:
                case TokenType::FALSE:
                    instr.type.baseType = BaseType::BOOL;
                    break;
                default:
                    break;
            }
            instr.exprRef.exprNode = &expr;
        } break;
        case NodeType::BIN_OP: {
            auto& binOp = static_cast<ASTBinOp&>(expr);

            instr.instrType = IRType::BIN_OP;
            flatten_expr(*binOp.left);
            instr.binOp.leftIndex = lineInstrs.size() - 1;
            flatten_expr(*binOp.right);
            instr.binOp.rightIndex = lineInstrs.size() - 1;

            instr.binOp.binOpNode = &expr;
            instr.index = lineInstrs.size();
        } break;
        default:
            instr.exprRef.exprNode = &expr;
            instr.type.baseType = BaseType::UNKNOWN;
            break;
    }
    lineInstrs.push_back(std::move(instr));
}

static constexpr char TYPE_COLUMN_LEN = 6;

static inline std::string str_lead(int maxLen, std::string&& str) {
    if (str.length() > maxLen) return str;
    return std::string(maxLen - str.length(), ' ') + str;
}

void Analyzer::ir_print() {
    std::string res;
    for (int l = 0; l < lineInstrs.size(); l++) {
        res += str_lead(3, std::to_string(l));
        res += " | ";
        res += str_lead(TYPE_COLUMN_LEN, expr_type_to_str(lineInstrs[l].type));
        switch (lineInstrs[l].instrType) {
            case IRType::DECL_DEF:
                res += " : defineDecl ";
                res += print_expr(*lineInstrs[l].declDef.declNode->lvalue);
                res += "\n";
                break;
            case IRType::VAR_YIELD:
                res += " : yield ";
                res += print_expr(*lineInstrs[l].varYield.nameNode);
                break;
            case IRType::EXPR:
                res += " : ";
                res += print_expr(*lineInstrs[l].exprRef.exprNode);
                break;
            case IRType::BIN_OP:
                res += " : ";
                res += "(#";
                res += std::to_string(lineInstrs[l].binOp.leftIndex);
                // res += token_type_to_str(lineInstrs[l].binOp.binOpNode.) res +=
                res += " #";
                res += std::to_string(lineInstrs[l].binOp.rightIndex);
                res += ")";
                break;
            default:
                break;
        }
        // printf(":%d:%s\n", i++, print_expr(*line.declRef.decl->lvalue).c_str());
        if (l < lineInstrs.size()) res += "\n";
    }
    printf("%s", res.c_str());
}