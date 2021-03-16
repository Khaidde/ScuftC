#include "ast.hpp"

#include <iostream>
#include <string>

#include "diagnostics.hpp"
#include "lexer.hpp"

std::string node_type_to_str(NodeType type) {
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

namespace {

constexpr int DUMP_INDENT_LENGTH = 2;

std::string indent_guide(int count) {
    std::string base = "|" + std::string(DUMP_INDENT_LENGTH - 1, ' ');
    std::string res = base;
    for (int i = 1; i < count; i++) {
        res += base;
    }
    return res;
}

inline std::string indent(int count) { return std::string(Lexer::TAB_WIDTH * count, ' '); }

std::string recur_dump(const ASTNode& node, int indentCt, bool verbose) {
    std::string dump = indent_guide(indentCt) + node_type_to_str(node.nodeType) + " ";
    switch (node.nodeType) {
        case NodeType::PROGRAM: {
            auto& prgm = static_cast<const ASTProgram&>(node);
            dump += "\n";
            for (auto&& decl : prgm.declarations) {
                dump += recur_dump(*decl, indentCt + 1, verbose) + "\n";
            }
        } break;
        case NodeType::BLOCK: {
            auto& block = static_cast<const ASTBlock&>(node);
            if (!block.statements.empty()) {
                dump += "\n";
                for (auto&& stmt : block.statements) {
                    dump += recur_dump(*stmt, indentCt + 1, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            } else {
                dump += " {EMPTY}";
            }
        } break;
        case NodeType::IF: {
            auto& ifStmt = static_cast<const ASTIf&>(node);
            if (verbose) {
                dump += "\n";
                dump += indent_guide(indentCt + 1) + "<condition>\n";
                dump += recur_dump(*ifStmt.condition, indentCt + 2, verbose) + "\n";
                dump += indent_guide(indentCt + 1) + "<conseq>\n";
                dump += recur_dump(*ifStmt.conseq, indentCt + 2, verbose);
            } else {
                dump += print_expr(*ifStmt.condition) + "\n";
                dump += recur_dump(*ifStmt.conseq, indentCt + 1, verbose);
            }
            if (ifStmt.alt != nullptr) {
                dump += "\n";
                dump += indent_guide(indentCt + 1) + "<alt>\n";
                dump += recur_dump(*ifStmt.alt, indentCt + 2, verbose);
            }
        } break;
        case NodeType::FOR: {
            ASSERT(false, "Unimplemented for loop dump");
            auto& forLoop = static_cast<const ASTFor&>(node);
            dump += "\n";
            if (verbose) {
                dump += indent_guide(indentCt + 1) + "<blockStmt>\n";
                dump += recur_dump(*forLoop.blockStmt, indentCt + 2, verbose);
            } else {
                dump += recur_dump(*forLoop.blockStmt, indentCt + 1, verbose);
            }
        } break;
        case NodeType::BREAK:
        case NodeType::CONT:
            break;
        case NodeType::RET: {
            auto& ret = static_cast<const ASTRet&>(node);
            if (ret.retValue != nullptr) {
                dump += "\n";
                dump += recur_dump(*ret.retValue, indentCt + 1, verbose);
            }
        } break;
        case NodeType::DECL: {
            auto& decl = static_cast<const ASTDecl&>(node);
            if (verbose) {
                dump += "\n";
                dump += indent_guide(indentCt + 1) + "<lvalue>\n";
                dump += recur_dump(*decl.lvalue, indentCt + 2, verbose) + "\n";
            } else {
                dump += print_expr(*decl.lvalue) + "\n";
            }
            if (decl.type != nullptr) {
                if (verbose) {
                    dump += indent_guide(indentCt + 1) + "<type>\n";
                    dump += recur_dump(*decl.type, indentCt + 2, verbose);
                } else {
                    dump += indent_guide(indentCt + 1) + ": " + print_expr(*decl.type);
                }
                if (decl.rvalue != nullptr) dump += "\n";
            }
            if (decl.rvalue != nullptr) {
                if (verbose) {
                    dump += indent_guide(indentCt + 1);
                    dump += "<assignType> " + token_type_to_str(decl.assignType->type) + "\n";
                    dump += indent_guide(indentCt + 1) + "<rvalue>\n";
                    dump += recur_dump(*decl.rvalue, indentCt + 2, verbose);
                } else {
                    dump += indent_guide(indentCt + 1);
                    dump += token_type_to_str(decl.assignType->type) + " ";
                    if (decl.rvalue->nodeType == NodeType::MOD || decl.rvalue->nodeType == NodeType::TYPE_DEF ||
                        decl.rvalue->nodeType == NodeType::FUNC) {
                        dump += "\n";
                        dump += recur_dump(*decl.rvalue, indentCt + 1, verbose);
                    } else {
                        dump += print_expr(*decl.rvalue);
                    }
                }
            }
        } break;
        case NodeType::TYPE_LIT: {
            auto& typeLit = static_cast<const ASTTypeLit&>(node);
            dump += print_ast(typeLit);
        } break;
        case NodeType::FUNC_TYPE: {
            auto& funcType = static_cast<const ASTFuncType&>(node);
            dump += "\n";
            if (!funcType.inTypes.empty()) {
                dump += indent_guide(indentCt + 1) + "<inTypes>\n";
                for (auto&& type : funcType.inTypes) {
                    dump += recur_dump(*type, indentCt + 3, verbose) + "\n";
                }
            }
            dump += indent_guide(indentCt + 1) + "<outType>\n";
            dump += indent_guide(indentCt + 2) + print_ast(*funcType.outType);
        } break;
        case NodeType::MOD: {
            auto& mod = static_cast<const ASTMod&>(node);
            if (!mod.declarations.empty()) {
                dump += "\n";
                if (verbose) {
                    dump += indent_guide(indentCt + 1);
                    dump += "<declarations>\n";
                    indentCt++;  // HACK to force an extra indent
                }
                for (auto&& decl : mod.declarations) {
                    dump += recur_dump(*decl, indentCt + 1, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            } else {
                dump += "{EMPTY}";
            }
        } break;
        case NodeType::TYPE_DEF: {
            auto& typeDef = static_cast<const ASTTy&>(node);
            if (!typeDef.declarations.empty()) {
                dump += "\n";
                if (verbose) {
                    dump += indent_guide(indentCt + 1);
                    dump += "<declarations>\n";
                    indentCt++;  // HACK to force an extra indent
                }
                for (auto&& decl : typeDef.declarations) {
                    dump += recur_dump(*decl, indentCt + 1, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            } else {
                dump += " {EMPTY}";
            }
        } break;
        case NodeType::FUNC: {
            auto& func = static_cast<const ASTFunc&>(node);
            dump += "\n";
            if (!func.parameters.empty()) {
                if (verbose) {
                    dump += indent_guide(indentCt + 1) + "<parameters>\n";
                    for (auto&& param : func.parameters) {
                        dump += recur_dump(*param, indentCt + 2, verbose) + "\n";
                    }
                } else {
                    dump += indent_guide(indentCt + 1) + "(";
                    for (int i = 0; i < func.parameters.size(); i++) {
                        dump += print_ast(*func.parameters[i]);
                        if (i + 1 < func.parameters.size()) {
                            dump += ", ";
                        }
                    }
                    dump += ")\n";
                }
            }
            if (func.returnType != nullptr) {
                if (verbose) {
                    dump += indent_guide(indentCt + 1) + "<returnType>\n";
                    dump += recur_dump(*func.returnType, indentCt + 2, verbose) + "\n";
                } else {
                    dump += indent_guide(indentCt + 1) + "-> ";
                    dump += print_ast(*func.returnType) + "\n";
                }
            }
            if (verbose) {
                dump += indent_guide(indentCt + 1) + "<blockStmt>\n";
                indentCt++;  // HACK to force an extra indent
            }
            dump += recur_dump(*func.blockOrExpr, indentCt + 1, verbose);
        } break;
        case NodeType::NAME: {
            auto& name = static_cast<const ASTName&>(node);
            dump += print_ast(name);
        } break;
        case NodeType::DOT_OP: {
            auto& dotOp = static_cast<const ASTDotOp&>(node);
            dump += "\n";
            dump += indent_guide(indentCt + 1) + "<base>\n" + recur_dump(*dotOp.base, indentCt + 2, verbose) + "\n";
            dump += indent_guide(indentCt + 1) + "<member>\n" + recur_dump(*dotOp.member, indentCt + 2, verbose);
        } break;
        case NodeType::CALL: {
            auto& call = static_cast<const ASTCall&>(node);
            dump += "\n";
            dump += indent_guide(indentCt + 1);
            dump += "<callRef>\n" + recur_dump(*call.callRef, indentCt + 2, verbose);
            if (!call.arguments.empty()) {
                dump += "\n";
                dump += indent_guide(indentCt + 1) + "<arguments>\n";
                for (auto&& expr : call.arguments) {
                    dump += recur_dump(*expr, indentCt + 2, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            }
        } break;
        case NodeType::TYPE_INIT: {
            auto& typeInit = static_cast<const ASTTypeInit&>(node);
            dump += "\n";
            dump += indent_guide(indentCt + 1) + "<typeRef>\n" + recur_dump(*typeInit.typeRef, indentCt + 2, verbose);
            if (!typeInit.assignments.empty()) {
                dump += "\n";
                dump += indent_guide(indentCt + 1) + "<assignments>\n";
                for (auto&& assignment : typeInit.assignments) {
                    dump += indent_guide(indentCt + 2) + "Assignment\n";
                    dump += indent_guide(indentCt + 3) + "<fieldRef> " + print_ast(*assignment->lvalue) + "\n";
                    dump += indent_guide(indentCt + 3) + "<rvalue>\n";
                    dump += recur_dump(*assignment->rvalue, indentCt + 4, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            }
        } break;
        case NodeType::LIT: {
            auto& lit = static_cast<const ASTLit&>(node);
            dump += print_ast(lit);
        } break;
        case NodeType::UN_OP: {
            auto& unOp = static_cast<const ASTUnOp&>(node);
            dump += "\n";
            dump += indent_guide(indentCt + 1) + "<op> " + token_type_to_str(unOp.op->type) + "\n";
            dump += indent_guide(indentCt + 1) + "<inner>\n" + recur_dump(*unOp.inner, indentCt + 2, verbose);
        } break;
        case NodeType::DEREF: {
            auto& deref = static_cast<const ASTDeref&>(node);
            dump += "\n";
            dump += indent_guide(indentCt + 1) + "<inner>\n" + recur_dump(*deref.inner, indentCt + 2, verbose);
        } break;
        case NodeType::BIN_OP: {
            auto& binOp = static_cast<const ASTBinOp&>(node);
            dump += "\n";
            dump += indent_guide(indentCt + 1) + "<left>\n" + recur_dump(*binOp.left, indentCt + 2, verbose) + "\n";
            dump += indent_guide(indentCt + 1) + "<op> " + token_type_to_str(binOp.op->type) + "\n";
            dump += indent_guide(indentCt + 1) + "<right>\n" + recur_dump(*binOp.right, indentCt + 2, verbose);
        } break;
        default:
            ASSERT(false, "TODO DELETE: Unimplemented AST dump for node");
    }
    return dump;
}

std::string recur_print_ast(const ASTNode& node, int indentCt) {
    std::string str;
    switch (node.nodeType) {
        case NodeType::PROGRAM: {
            auto& prgm = static_cast<const ASTProgram&>(node);
            for (auto&& decl : prgm.declarations) {
                str += recur_print_ast(*decl, indentCt) + "\n";
            }
        } break;
        case NodeType::BLOCK: {
            auto& block = static_cast<const ASTBlock&>(node);
            str += "{\n";
            for (auto& stmt : block.statements) {
                str += indent(indentCt + 1) + recur_print_ast(*stmt, indentCt + 1);
                str += "\n";
            }
            str += indent(indentCt) + "}";
        } break;
        case NodeType::IF: {
            auto& ifStmt = static_cast<const ASTIf&>(node);
            str += "if " + recur_print_ast(*ifStmt.condition, indentCt) + " ";
            str += recur_print_ast(*ifStmt.conseq, indentCt);
            if (ifStmt.alt != nullptr) {
                str += " else ";
                str += recur_print_ast(*ifStmt.alt, indentCt);
            }
        } break;
        case NodeType::FOR: {
            auto& forLoop = static_cast<const ASTFor&>(node);
            str += "for ";
            if (forLoop.initial != nullptr) {
                str += recur_print_ast(*forLoop.initial, indentCt) + ", ";
            }
            if (forLoop.condition != nullptr) {
                str += print_expr(*forLoop.condition);
                if (forLoop.post != nullptr) str += ",";
                str += " ";
            }
            if (forLoop.post != nullptr) {
                str += recur_print_ast(*forLoop.post, indentCt) + " ";
            }
            str += recur_print_ast(*forLoop.blockStmt, indentCt);
        } break;
        case NodeType::RET: {
            auto& ret = static_cast<const ASTRet&>(node);
            str += "return";
            if (ret.retValue != nullptr) {
                str += " " + recur_print_ast(*ret.retValue, indentCt);
            }
        } break;
        case NodeType::DECL: {
            auto& decl = static_cast<const ASTDecl&>(node);
            str += recur_print_ast(*decl.lvalue, indentCt);
            if (decl.type != nullptr) {
                str += ": " + print_expr(*decl.type);
            }
            if (decl.assignType != nullptr) {
                str += " " + token_type_to_str(decl.assignType->type) + " ";
            }
            if (decl.rvalue != nullptr) {
                str += recur_print_ast(*decl.rvalue, indentCt);
            }
        } break;
        case NodeType::MOD: {
            auto& mod = static_cast<const ASTMod&>(node);
            str += "mod {\n";
            for (auto&& decl : mod.declarations) {
                str += indent(indentCt + 1) + recur_print_ast(*decl, indentCt + 1);
                str += "\n";
            }
            str += indent(indentCt) + "}";
        } break;
        case NodeType::TYPE_DEF: {
            auto& typeDef = static_cast<const ASTTy&>(node);
            str += "ty {\n";
            for (auto&& decl : typeDef.declarations) {
                str += indent(indentCt + 1) + recur_print_ast(*decl, indentCt + 1);
                str += "\n";
            }
            str += indent(indentCt) + "}";
        } break;
        case NodeType::FUNC: {
            auto& func = static_cast<const ASTFunc&>(node);
            str = "(";
            for (int i = 0; i < func.parameters.size(); i++) {
                str += print_ast(*func.parameters[i]);
                if (i + 1 < func.parameters.size()) {
                    str += ", ";
                }
            }
            if (func.returnType != nullptr) {
                str += ") -> " + recur_print_ast(*func.returnType, indentCt) + " ";
            } else {
                str += ") ";
            }
            if (func.blockOrExpr->nodeType != NodeType::BLOCK) {
                str += ":: ";
            }
            str += recur_print_ast(*func.blockOrExpr, indentCt);
        } break;
        default:
            return print_expr(static_cast<const ASTExpression&>(node));
    }
    return str;
}

}  // namespace

void dump_ast(const ASTNode& node, bool verbose) { std::cout << recur_dump(node, 0, verbose) << std::endl; }

std::string print_expr(const ASTExpression& expr) {
    std::string str;
    switch (expr.nodeType) {
        case NodeType::TYPE_LIT: {
            auto& typeLit = static_cast<const ASTTypeLit&>(expr);
            return token_type_to_str(typeLit.type);
        }
        case NodeType::FUNC_TYPE: {
            auto& funcType = static_cast<const ASTFuncType&>(expr);
            str += "(";
            for (int i = 0; i < funcType.inTypes.size(); i++) {
                str += print_expr(*funcType.inTypes[i]);
                if (i + 1 < funcType.inTypes.size()) {
                    str += ", ";
                }
            }
            str += ") -> " + print_expr(*funcType.outType);
        } break;
        case NodeType::MOD: {
            auto& mod = static_cast<const ASTTy&>(expr);

            std::string declListStr;
            for (int i = 0; i < mod.declarations.size(); i++) {
                auto&& decl = mod.declarations[i];
                declListStr += print_expr(*decl->lvalue);
                if (decl->type != nullptr) declListStr += ": " + print_expr(*decl->type);
                if (decl->rvalue != nullptr) {
                    declListStr += token_type_to_str(decl->assignType->type);
                    declListStr += print_expr(*decl->rvalue);
                }
                if (i + 1 < mod.declarations.size()) {
                    declListStr += ", ";
                }
            }
            return "mod {" + declListStr + "}";
        }
        case NodeType::TYPE_DEF: {
            auto& typeDef = static_cast<const ASTTy&>(expr);
            std::string declListStr;
            for (int i = 0; i < typeDef.declarations.size(); i++) {
                auto&& decl = typeDef.declarations[i];
                declListStr += print_expr(*decl->lvalue);
                if (decl->type != nullptr) declListStr += ": " + print_expr(*decl->type);
                if (decl->rvalue != nullptr) {
                    declListStr += token_type_to_str(decl->assignType->type);
                    declListStr += print_expr(*decl->rvalue);
                }
                if (i + 1 < typeDef.declarations.size()) {
                    declListStr += ", ";
                }
            }
            return "ty {" + declListStr + "}";
        }
        case NodeType::FUNC: {
            auto& func = static_cast<const ASTFunc&>(expr);
            std::string paramListStr;
            for (int i = 0; i < func.parameters.size(); i++) {
                paramListStr += print_ast(*func.parameters[i]);
                if (i + 1 < func.parameters.size()) {
                    paramListStr += ", ";
                }
            }
            if (func.returnType != nullptr) {
                return "(" + paramListStr + ") -> " + print_expr(*func.returnType) + " {...}";
            } else {
                return "(" + paramListStr + ") {...}";
            }
        }
        case NodeType::NAME: {
            return static_cast<const ASTName&>(expr).ref->get_string_val();
        }
        case NodeType::DOT_OP: {
            auto& dotOp = static_cast<const ASTDotOp&>(expr);
            return "(" + print_expr(*dotOp.base) + "." + print_expr(*dotOp.member) + ")";
        }
        case NodeType::CALL: {
            auto& call = static_cast<const ASTCall&>(expr);
            str = print_expr(*call.callRef) + "(";
            for (int i = 0; i < call.arguments.size(); i++) {
                str += print_expr(*call.arguments[i]);
                if (i + 1 < call.arguments.size()) {
                    str += ", ";
                }
            }
            str += ")";
        } break;
        case NodeType::TYPE_INIT: {
            auto& typeInit = static_cast<const ASTTypeInit&>(expr);
            str = print_expr(*typeInit.typeRef) + ".{";
            for (int i = 0; i < typeInit.assignments.size(); i++) {
                str += print_expr(*typeInit.assignments[i]->lvalue);
                str += "=";
                str += print_expr(*typeInit.assignments[i]->rvalue);
                if (i + 1 < typeInit.assignments.size()) {
                    str += ", ";
                }
            }
            str += "}";
        } break;
        case NodeType::LIT: {
            auto& lit = static_cast<const ASTLit&>(expr);
            switch (lit.value->type) {
                case TokenType::INT_LITERAL:
                    return std::to_string(lit.value->longVal);
                case TokenType::DOUBLE_LITERAL:
                    return std::to_string(lit.value->doubleVal);
                case TokenType::STRING_LITERAL:
                    return lit.value->get_string_val();
                case TokenType::TRUE:
                    return "true";
                case TokenType::FALSE:
                    return "false";
                default:
                    ASSERT(false, "Not a literal: " + token_type_to_str(lit.value->type));
            }
        }
        case NodeType::UN_OP: {
            auto& unOp = static_cast<const ASTUnOp&>(expr);
            return token_type_to_str(unOp.op->type) + "(" + print_expr(*unOp.inner) + ")";
        }
        case NodeType::DEREF: {
            auto& deref = static_cast<const ASTDeref&>(expr);
            return "(" + print_expr(*deref.inner) + ").*";
        }
        case NodeType::BIN_OP: {
            auto& binOp = static_cast<const ASTBinOp&>(expr);
            return "(" + print_expr(*binOp.left) + " " + token_type_to_str(binOp.op->type) + " " +
                   print_expr(*binOp.right) + ")";
        }
        case NodeType::UNKNOWN:
            return "Unknown";
        default:
            ASSERT(false, "Node is not an expression");
    }
    return str;
}

std::string print_ast(const ASTNode& node) { return recur_print_ast(node, 0); }

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
