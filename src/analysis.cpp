#include "analysis.hpp"

#include <stdio.h>

#include <stdexcept>

#include "diagnostics.hpp"
// #include "lexer.hpp"
#include "ast_printer.hpp"

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

void Analyzer::analyze(ASTNode* prgmNode) {
    // TODO lineInstrs.reserve(traverse_get_ir_size(prgm));

    flatten_program(prgmNode);
    // printf("%dl", lineInstrs.capacity());
    ir_print();
}

void Analyzer::flatten_program(ASTNode* prgmNode) {
    for (auto& decl : prgmNode->prgm.declarations) {
        flatten_decl(decl.get());
    }
}

void Analyzer::flatten_decl(ASTNode* declNode) {
    // TODO flatten_expr on lvalue

    if (declNode->decl.rvalue != nullptr) flatten_expr(declNode->decl.rvalue.get());

    IRInstruct instr{};
    instr.index = lineInstrs.size();
    instr.instrType = IRType::DECL_DEF;
    instr.declDef.declNode = &declNode->decl;
    lineInstrs.push_back(std::move(instr));
}

void Analyzer::flatten_expr(ASTNode* exprNode) {
    IRInstruct instr{};
    switch (exprNode->nodeType) {
        case NodeType::NAME: {
            instr.index = lineInstrs.size();
            instr.instrType = IRType::VAR_YIELD;
            instr.varYield.nameNode = exprNode;
        } break;
        case NodeType::LIT: {
            instr.index = lineInstrs.size();
            instr.instrType = IRType::EXPR;
            switch (exprNode->lit.value->type) {
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
            instr.exprRef.exprNode = exprNode;
        } break;
        case NodeType::BIN_OP: {
            instr.instrType = IRType::BIN_OP;
            flatten_expr(exprNode->binOp.left.get());
            instr.binOp.leftIndex = lineInstrs.size() - 1;
            flatten_expr(exprNode->binOp.right.get());
            instr.binOp.rightIndex = lineInstrs.size() - 1;

            instr.binOp.binOpNode = exprNode;
            instr.index = lineInstrs.size();
        } break;
        default:
            instr.exprRef.exprNode = exprNode;
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
                res += ast_to_line(lineInstrs[l].declDef.declNode->lvalue.get());
                res += "\n";
                break;
            case IRType::VAR_YIELD:
                res += " : yield ";
                res += ast_to_line(lineInstrs[l].varYield.nameNode);
                break;
            case IRType::EXPR:
                res += " : ";
                res += ast_to_line(lineInstrs[l].exprRef.exprNode);
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