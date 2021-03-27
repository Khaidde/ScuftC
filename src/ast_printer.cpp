#include "ast_printer.hpp"

#include <iostream>

#include "lexer.hpp"

namespace internal {

inline std::string tab(int amt) { return std::string(Lexer::TAB_WIDTH * amt, ' '); }

inline std::string bar_tab(int count) {
    static constexpr int DUMP_INDENT_LENGTH = 2;

    std::string base = "|" + std::string(DUMP_INDENT_LENGTH - 1, ' ');
    std::string res = base;
    for (int i = 1; i < count; i++) {
        res += base;
    }
    return res;
}

std::string list_to_src(std::vector<std::unique_ptr<ASTNode>>& list, int indent);
std::string list_to_comma_sep(std::vector<std::unique_ptr<ASTNode>>& list);

std::string recur_ast_to_src(ASTNode* node, int indent);
std::string recur_print_fmt(ASTNode* node, int indent);

};  // namespace internal

const char* node_type_to_str(NodeType type) {
    switch (type) {
        case NodeType::UNKNOWN:
            return "[UNKNOWN]";
        case NodeType::PROGRAM:
            return "Program";
        case NodeType::BLOCK:
            return "Block";
        case NodeType::IF:
            return "If";
        case NodeType::FOR:
            return "For";
        case NodeType::BREAK:
            return "Break";
        case NodeType::CONT:
            return "Continue";
        case NodeType::RET:
            return "Return";
        case NodeType::DECL:
            return "Declaration";
        case NodeType::TYPE_LIT:
            return "Type Literal";
        case NodeType::FUNC_TYPE:
            return "Function Type";
        case NodeType::MOD:
            return "Module";
        case NodeType::TYPE_DEF:
            return "Type Definition";
        case NodeType::FUNC:
            return "Function";
        case NodeType::NAME:
            return "Name";
        case NodeType::DOT_OP:
            return "Dot Operator";
        case NodeType::CALL:
            return "Call";
        case NodeType::TYPE_INIT:
            return "Type Initializer";
        case NodeType::LIT:
            return "Literal";
        case NodeType::UN_OP:
            return "Unary Operator";
        case NodeType::DEREF:
            return "Dereference";
        case NodeType::BIN_OP:
            return "Binary Operator";
    }
}

std::string internal::list_to_src(std::vector<std::unique_ptr<ASTNode>>& list, int indent) {
    if (list.empty()) return "";
    std::string res;
    for (int i = 0; i < list.size(); i++) {
        res += tab(indent) + recur_ast_to_src(list[i].get(), indent);
        res += "\n";
    }
    return res;
}

std::string internal::list_to_comma_sep(std::vector<std::unique_ptr<ASTNode>>& list) {
    if (list.empty()) return "";
    std::string res = ast_to_line(list[0].get());
    for (int i = 1; i < list.size(); i++) {
        res += ", ";
        res += ast_to_line(list[i].get());
    }
    return res;
}

std::string internal::recur_ast_to_src(ASTNode* node, int indent) {
    std::string src;
    switch (node->nodeType) {
        case NodeType::PROGRAM:
            for (int i = 0; i < node->prgm.declarations.size(); i++) {
                src += recur_ast_to_src(node->prgm.declarations[i].get(), indent);
            }
            break;
        case NodeType::BLOCK:
            src += "{\n";
            src += list_to_src(node->block.statements, indent + 1);
            src += tab(indent) + "}";
            break;
        case NodeType::IF: {
            src += "if " + ast_to_line(node->ifNode.condition.get()) + " ";
            src += recur_ast_to_src(node->ifNode.conseq.get(), indent);
            if (node->ifNode.alt) {
                src += " else ";
                src += recur_ast_to_src(node->ifNode.alt.get(), indent);
            }
        } break;
        case NodeType::FOR: {
            src += "for ";
            if (node->forLoop.initial) {
                src += ast_to_line(node->forLoop.initial.get()) + ", ";
            }
            if (node->forLoop.condition) {
                src += ast_to_line(node->forLoop.condition.get());
                if (node->forLoop.post) src += ",";
                src += " ";
            }
            if (node->forLoop.post) {
                src += ast_to_line(node->forLoop.post.get()) + " ";
            }
            src += recur_ast_to_src(node->forLoop.blockStmt.get(), indent);
        } break;
        case NodeType::BREAK:
            return "break";
        case NodeType::CONT:
            return "continue";
        case NodeType::RET:
            src += "return";
            if (node->ret.retExpr) {
                src += " " + ast_to_line(node->ret.retExpr.get());
            }
            break;
        case NodeType::DECL:
            src += ast_to_line(node->decl.lvalue.get());
            if (node->decl.type) {
                src += ": " + ast_to_line(node->decl.type.get());
            }
            if (node->decl.rvalue) {
                src += " " + token_type_to_str(node->decl.assignType->type) + " ";
                src += recur_ast_to_src(node->decl.rvalue.get(), indent);
            }
            break;
        case NodeType::MOD:
            src += "mod {\n";
            src += list_to_src(node->mod.declarations, indent + 1);
            src += tab(indent) + "}";
            break;
        case NodeType::TYPE_DEF:
            src += "ty {\n";
            src += list_to_src(node->ty.declarations, indent + 1);
            src += tab(indent) + "}";
            break;
        case NodeType::FUNC:
            src = "(" + ast_to_line(node->func.parameters[0].get());
            for (int i = 0; i < node->func.parameters.size(); i++) {
                src += ", ";
                src += ast_to_line(node->func.parameters[i].get());
            }
            if (node->func.returnType) {
                src += ") -> " + recur_ast_to_src(node->func.returnType.get(), indent) + " ";
            } else {
                src += ") ";
            }
            if (node->func.isShorthand) {
                src += ":: ";
                src += ast_to_line(node->func.expr.get());
            } else {
                src += recur_ast_to_src(node->func.block.get(), indent);
            }
            break;
        default:
            return ast_to_line(node);
    }
    return src;
}

std::string ast_to_line(ASTNode* node) {
    std::string line;
    switch (node->nodeType) {
        case NodeType::DECL:
            line = ast_to_line(node->decl.lvalue.get());
            if (node->decl.type) {
                line += ": " + ast_to_line(node->decl.type.get());
            }
            if (node->decl.rvalue) {
                line += token_type_to_str(node->decl.assignType->type);
                line += ast_to_line(node->decl.rvalue.get());
            }
            break;
        case NodeType::TYPE_LIT:
            line = token_type_to_str(node->typeLit.type);
            break;
        case NodeType::FUNC_TYPE:
            line = "(";
            line += internal::list_to_comma_sep(node->funcType.inTypes);
            line += ") -> " + ast_to_line(node->funcType.outType.get());
            break;
        case NodeType::MOD:
            line = "mod {";
            line += internal::list_to_comma_sep(node->mod.declarations);
            line += "}";
            break;
        case NodeType::TYPE_DEF:
            line = "ty {";
            line += internal::list_to_comma_sep(node->ty.declarations);
            line += "}";
            break;
        case NodeType::FUNC:
            line = "(";
            line += internal::list_to_comma_sep(node->func.parameters);
            if (node->func.returnType.get()) {
                line += ") -> ";
                line += ast_to_line(node->func.returnType.get());
                line += " {...}";
            } else {
                line += ") {...}";
            }
            break;
        case NodeType::NAME:
            line = node->name.ref->get_string_val();
            break;
        case NodeType::DOT_OP:
            line = "(";
            line += ast_to_line(node->dotOp.base.get());
            line += ".";
            line += ast_to_line(node->dotOp.member.get());
            line += ")";
            break;
        case NodeType::CALL:
            line = ast_to_line(node->call.callRef.get());
            line += "(";
            line += internal::list_to_comma_sep(node->call.arguments);
            line += ")";
            break;
        case NodeType::TYPE_INIT:
            line = ast_to_line(node->typeInit.typeRef.get());
            line += ".{";
            line += internal::list_to_comma_sep(node->typeInit.assignments);
            line += "}";
            break;
        case NodeType::LIT:
            switch (node->lit.value->type) {
                case TokenType::INT_LITERAL:
                    line = std::to_string(node->lit.value->longVal);
                    break;
                case TokenType::DOUBLE_LITERAL:
                    line = std::to_string(node->lit.value->doubleVal);
                    break;
                case TokenType::STRING_LITERAL:
                    line = node->lit.value->get_string_val();
                    break;
                case TokenType::TRUE:
                    line = "true";
                    break;
                case TokenType::FALSE:
                    line = "false";
                    break;
                default:
                    ASSERT(false, "Not a literal: " + token_type_to_str(node->lit.value->type));
            }
            break;
        case NodeType::UN_OP:
            line += token_type_to_str(node->unOp.op->type);
            line += "(";
            line += ast_to_line(node->unOp.inner.get());
            line += ")";
            break;
        case NodeType::DEREF:
            line += "(";
            line += ast_to_line(node->deref.inner.get());
            line += ").*";
            break;
        case NodeType::BIN_OP:
            line = "(";
            line += ast_to_line(node->binOp.left.get());
            line += token_type_to_str(node->binOp.op->type);
            line += ast_to_line(node->binOp.right.get());
            line += ")";
            break;
        case NodeType::UNKNOWN:
            return "Unknown";
        default:
            ASSERT(false, "Node is not an expression: " + node_type_to_str(node->nodeType));
    }
    return line;
}

std::string ast_to_src(ASTNode* node) { return internal::recur_ast_to_src(node, 0); }

std::string internal::recur_print_fmt(ASTNode* node, int indent) {
    std::string res = bar_tab(indent);
    res += node_type_to_str(node->nodeType);
    res += " ";
    switch (node->nodeType) {
        case NodeType::PROGRAM:
            for (int i = 0; i < node->prgm.declarations.size(); i++) {
                res += "\n";
                res += recur_print_fmt(node->prgm.declarations[i].get(), indent + 1);
            }
            break;
        case NodeType::BLOCK:
            if (node->block.statements.empty()) {
                res += " {}";
            } else {
                for (int i = 0; i < node->block.statements.size(); i++) {
                    res += "\n";
                    res += recur_print_fmt(node->block.statements[i].get(), indent + 1);
                }
            }
            break;
        case NodeType::IF:
            res += ast_to_line(node->ifNode.condition.get()) + "\n";
            res += recur_print_fmt(node->ifNode.conseq.get(), indent + 1);
            if (node->ifNode.alt) {
                res += "\n";
                res += bar_tab(indent + 1);
                res += "<alt>\n";
                res += recur_print_fmt(node->ifNode.alt.get(), indent + 1);
            }
            break;
        case NodeType::FOR:
            ASSERT(false, "Unimplemented for loop dump");
            /*
            auto& forLoop = static_cast<const ASTFor&>(node);
            dump += "\n";
            if (verbose) {
                dump += indent_guide(indentCt + 1) + "<blockStmt>\n";
                dump += recur_dump(*forLoop.blockStmt, indentCt + 2, verbose);
            } else {
                dump += recur_dump(*forLoop.blockStmt, indentCt + 1, verbose);
            }
            */
            break;
        case NodeType::BREAK:
        case NodeType::CONT:
            break;
        case NodeType::RET:
            if (node->ret.retExpr) {
                res += "\n";
                res += recur_print_fmt(node->ret.retExpr.get(), indent + 1);
            }
            break;
        case NodeType::DECL:
            res += ast_to_line(node->decl.lvalue.get()) + "\n";
            if (node->decl.type) {
                res += bar_tab(indent + 1);
                res += ": ";
                res += ast_to_line(node->decl.type.get());
                if (node->decl.rvalue) res += "\n";
            }
            if (node->decl.rvalue) {
                res += bar_tab(indent + 1);
                res += token_type_to_str(node->decl.assignType->type);
                res += " ";
                /*
                if (node->decl.rvalue->nodeType == NodeType::MOD || node->decl.rvalue->nodeType == NodeType::TYPE_DEF ||
                    node->decl.rvalue->nodeType == NodeType::FUNC) {
                    res += "\n";
                    res += recur_print_fmt(node->decl.rvalue.get(), indent + 1);
                } else {
                    res += ast_to_line(node->decl.rvalue.get());
                }
                */
                res += "\n";
                res += recur_print_fmt(node->decl.rvalue.get(), indent + 1);
            }
            break;
        case NodeType::TYPE_LIT:
            res += ast_to_line(node);
            break;
        case NodeType::FUNC_TYPE:
            res += "\n";
            if (!node->funcType.inTypes.empty()) {
                res += bar_tab(indent + 1);
                res += "<inTypes>\n";
                for (int i = 0; i < node->funcType.inTypes.size(); i++) {
                    res += recur_print_fmt(node->funcType.inTypes[i].get(), indent + 3);
                    res += "\n";
                }
            }
            res += bar_tab(indent + 1);
            res += "<outType>\n";
            res += bar_tab(indent + 2);
            res += ast_to_line(node->funcType.outType.get());
            break;
        case NodeType::MOD:
            if (node->mod.declarations.empty()) {
                res += " {}";
            } else {
                for (int i = 0; i < node->mod.declarations.size(); i++) {
                    res += "\n";
                    res += recur_print_fmt(node->mod.declarations[i].get(), indent + 1);
                }
            }
            break;
        case NodeType::TYPE_DEF:
            if (node->ty.declarations.empty()) {
                res += " {}";
            } else {
                for (int i = 0; i < node->ty.declarations.size(); i++) {
                    res += "\n";
                    res += recur_print_fmt(node->ty.declarations[i].get(), indent + 1);
                }
            }
            break;
        case NodeType::FUNC:
            res += "\n";
            if (!node->func.parameters.empty()) {
                res += bar_tab(indent + 1);
                res += "(";
                res += list_to_comma_sep(node->func.parameters);
                res += ")\n";
            }
            if (node->func.returnType) {
                res += bar_tab(indent + 1);
                res += "-> ";
                res += ast_to_line(node->func.returnType.get());
                res += "\n";
            }
            res += recur_print_fmt(node->func.block.get(), indent + 1);
            break;
        case NodeType::NAME:
            res += ast_to_line(node);
            break;
        case NodeType::DOT_OP:
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<base>\n";
            res += recur_print_fmt(node->dotOp.base.get(), indent + 2);
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<member>\n";
            res += recur_print_fmt(node->dotOp.member.get(), indent + 2);
            break;
        case NodeType::CALL:
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<callRef>\n";
            res += recur_print_fmt(node->call.callRef.get(), indent + 2);
            if (!node->call.arguments.empty()) {
                res += "\n";
                res += bar_tab(indent + 1);
                res += "<arguments>";
                for (int i = 0; i < node->call.arguments.size(); i++) {
                    res += "\n";
                    res += recur_print_fmt(node->call.arguments[i].get(), indent + 2);
                }
            }
            break;
        case NodeType::TYPE_INIT:
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<typeRef>\n";
            res += recur_print_fmt(node->typeInit.typeRef.get(), indent + 2);
            if (!node->typeInit.assignments.empty()) {
                res += "\n";
                res += bar_tab(indent + 1);
                res += "<assignments>";
                for (int i = 0; i < node->typeInit.assignments.size(); i++) {
                    res += "\n";
                    res += bar_tab(indent + 2);
                    res += "Assignment\n";
                    res += bar_tab(indent + 3);
                    res += "<fieldRef> ";
                    res += ast_to_line(node->typeInit.assignments[i]->decl.lvalue.get());
                    res += "\n";
                    res += bar_tab(indent + 3);
                    res += "<rvalue>\n";
                    res += recur_print_fmt(node->typeInit.assignments[i]->decl.rvalue.get(), indent + 4);
                }
            }
            break;
        case NodeType::LIT:
            res += ast_to_line(node);
            break;
        case NodeType::UN_OP:
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<op> ";
            res += token_type_to_str(node->unOp.op->type);
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<inner>\n";
            res += recur_print_fmt(node->unOp.inner.get(), indent + 2);
            break;
        case NodeType::DEREF:
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<inner>\n";
            res += recur_print_fmt(node->deref.inner.get(), indent + 2);
            break;
        case NodeType::BIN_OP:
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<left>\n";
            res += recur_print_fmt(node->binOp.left.get(), indent + 2);
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<op> ";
            res += token_type_to_str(node->binOp.op->type);
            res += "\n";
            res += bar_tab(indent + 1);
            res += "<right>\n";
            res += recur_print_fmt(node->binOp.right.get(), indent + 2);
            break;
        default:
            ASSERT(false,
                   "TODO DELETE: Unimplemented AST formatted print for node: " + node_type_to_str(node->nodeType));
    }
    return res;
}

void print_fmt_ast(ASTNode* node) { std::cout << internal::recur_print_fmt(node, 0) << std::endl; }

/* TODO possible formatting of dump print 2/8/21
testMod.varName: int = 3 + 2
newType = type {
    val: bool;
}

ASTProgram                      (1:0)
 1> ASTDecl                     (1:0)
     l> ASTDotOp                (1:0:  "testMod.varName")
         r> ASTName testMod     (1:0:  "testMod")
         m> ASTName varName     (1:9:  "varName")
     t> ASTTypeLit int          (1:18: "int")
     r> ASTBinOp +              (1:24: "3 + 2")
         l> ASTLit 3            (1:24: "3")
         r> ASTLit 2            (1:28: "2")
 2> ASTDecl                     (2:0)
     l> ASTName newType         (2:0:  "newType")
     r> ASTTypeDef              (2:11)
         1> ASTDecl             (3:5)
             l> ASTName val     (3:5:  "val")
             t> ASTTypeLit bool (3:10: "bool")
*/