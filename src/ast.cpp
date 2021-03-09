#include "ast.hpp"

#include <iostream>
#include <string>

constexpr int DUMP_INDENT_LENGTH = 2;
namespace {

std::string indentGuide(int count) {
    std::string base = "|" + std::string(DUMP_INDENT_LENGTH - 1, ' ');
    std::string res = base;
    for (int i = 1; i < count; i++) {
        res += base;
    }
    return res;
}

inline std::string indent(int count) { return std::string(Lexer::TAB_WIDTH * count, ' '); }

std::string recurDump(const ASTNode& node, int indentCt, bool verbose) {
    std::string dump;
    switch (node.nodeType) {
        case NodeType::PROGRAM: {
            auto& prgm = static_cast<const ASTProgram&>(node);
            dump += "Program\n";
            for (auto&& decl : prgm.declarations) {
                dump += recurDump(*decl, indentCt + 1, verbose) + "\n";
            }
        } break;
        case NodeType::BLOCK: {
            auto& block = static_cast<const ASTBlock&>(node);
            dump += indentGuide(indentCt) + "Block";
            if (!block.statements.empty()) {
                dump += "\n";
                for (auto&& stmt : block.statements) {
                    dump += recurDump(*stmt, indentCt + 1, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            } else {
                dump += " {EMPTY}";
            }
        } break;
        case NodeType::IF: {
            auto& ifStmt = static_cast<const ASTIf&>(node);
            dump += indentGuide(indentCt) + "If ";
            if (verbose) {
                dump += "\n";
                dump += indentGuide(indentCt + 1) + "<condition>\n";
                dump += recurDump(*ifStmt.condition, indentCt + 2, verbose) + "\n";
                dump += indentGuide(indentCt + 1) + "<conseq>\n";
                dump += recurDump(*ifStmt.conseq, indentCt + 2, verbose);
            } else {
                dump += printExpr(*ifStmt.condition) + "\n";
                dump += recurDump(*ifStmt.conseq, indentCt + 1, verbose);
            }
            if (ifStmt.alt != nullptr) {
                dump += "\n";
                dump += indentGuide(indentCt + 1) + "<alt>\n";
                dump += recurDump(*ifStmt.alt, indentCt + 2, verbose);
            }
        } break;
        case NodeType::WHILE: {
            auto& whileLoop = static_cast<const ASTWhile&>(node);
            dump += indentGuide(indentCt) + "While ";
            if (verbose) {
                dump += "\n";
                dump += indentGuide(indentCt + 1) + "<condition>\n";
                dump += recurDump(*whileLoop.condition, indentCt + 2, verbose) + "\n";
                dump += indentGuide(indentCt + 1) + "<blockStmt>\n";
                dump += recurDump(*whileLoop.blockStmt, indentCt + 2, verbose);
            } else {
                dump += printExpr(*whileLoop.condition) + "\n";
                dump += recurDump(*whileLoop.blockStmt, indentCt + 1, verbose);
            }
        } break;
        case NodeType::FOR: {
            auto& forLoop = static_cast<const ASTFor&>(node);
            dump += indentGuide(indentCt) + "For\n";
            if (verbose) {
                dump += indentGuide(indentCt + 1) + "<blockStmt>\n";
                dump += recurDump(*forLoop.blockStmt, indentCt + 2, verbose);
            } else {
                dump += recurDump(*forLoop.blockStmt, indentCt + 1, verbose);
            }
        } break;
        case NodeType::BREAK:
            dump += "Break";
            break;
        case NodeType::CONT:
            dump += "Continue";
            break;
        case NodeType::RET: {
            auto& ret = static_cast<const ASTRet&>(node);
            dump += indentGuide(indentCt) + "Return";
            if (ret.retValue != nullptr) {
                dump += "\n";
                dump += recurDump(*ret.retValue, indentCt + 1, verbose);
            }
        } break;
        case NodeType::DECL: {
            auto& decl = static_cast<const ASTDecl&>(node);
            dump += indentGuide(indentCt) + "Decl ";
            if (verbose) {
                dump += "\n";
                dump += indentGuide(indentCt + 1) + "<lvalue>\n";
                dump += recurDump(*decl.lvalue, indentCt + 2, verbose) + "\n";
            } else {
                dump += printExpr(*decl.lvalue) + "\n";
            }
            if (decl.type != nullptr) {
                if (verbose) {
                    dump += indentGuide(indentCt + 1) + "<type>\n";
                    dump += recurDump(*decl.type, indentCt + 2, verbose);
                } else {
                    dump += indentGuide(indentCt + 1) + ":" + printExpr(*decl.type);
                }
                if (decl.rvalue != nullptr) dump += "\n";
            }
            if (decl.rvalue != nullptr) {
                if (verbose) {
                    dump += indentGuide(indentCt + 1);
                    dump += "<assignType> " + tokenTypeToStr(decl.assignType->type) + "\n";
                    dump += indentGuide(indentCt + 1) + "<rvalue>\n";
                    dump += recurDump(*decl.rvalue, indentCt + 2, verbose);
                } else {
                    dump += indentGuide(indentCt + 1);
                    dump += tokenTypeToStr(decl.assignType->type) + " ";
                    if (decl.rvalue->nodeType == NodeType::MOD || decl.rvalue->nodeType == NodeType::TYPE_DEF ||
                        decl.rvalue->nodeType == NodeType::FUNC) {
                        dump += "\n";
                        dump += recurDump(*decl.rvalue, indentCt + 1, verbose);
                    } else {
                        dump += printExpr(*decl.rvalue);
                    }
                }
            }
        } break;
        case NodeType::TYPE_LIT: {
            auto& typeLit = static_cast<const ASTTypeLit&>(node);
            dump += indentGuide(indentCt) + "TypeLit " + printAST(typeLit);
        } break;
        case NodeType::FUNC_TYPE: {
            auto& funcType = static_cast<const ASTFuncType&>(node);
            dump += indentGuide(indentCt) + "FunctionType\n";
            if (!funcType.inTypes.empty()) {
                dump += indentGuide(indentCt + 1) + "<inTypes>\n";
                for (auto&& type : funcType.inTypes) {
                    dump += recurDump(*type, indentCt + 3, verbose) + "\n";
                }
            }
            dump += indentGuide(indentCt + 1) + "<outType>\n";
            dump += indentGuide(indentCt + 2) + printAST(*funcType.outType);
        } break;
        case NodeType::MOD: {
            auto& mod = static_cast<const ASTMod&>(node);
            dump += indentGuide(indentCt) + "Module";
            if (!mod.declarations.empty()) {
                dump += "\n";
                if (verbose) {
                    dump += indentGuide(indentCt + 1);
                    dump += "<declarations>\n";
                    indentCt++;  // HACK to force an extra indent
                }
                for (auto&& decl : mod.declarations) {
                    dump += recurDump(*decl, indentCt + 1, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            } else {
                dump += " {EMPTY}";
            }
        } break;
        case NodeType::TYPE_DEF: {
            auto& typeDef = static_cast<const ASTTy&>(node);
            dump += indentGuide(indentCt) + "TypeDef";
            if (!typeDef.declarations.empty()) {
                dump += "\n";
                if (verbose) {
                    dump += indentGuide(indentCt + 1);
                    dump += "<declarations>\n";
                    indentCt++;  // HACK to force an extra indent
                }
                for (auto&& decl : typeDef.declarations) {
                    dump += recurDump(*decl, indentCt + 1, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            } else {
                dump += " {EMPTY}";
            }
        } break;
        case NodeType::FUNC: {
            auto& func = static_cast<const ASTFunc&>(node);
            dump += indentGuide(indentCt) + "Function\n";
            if (!func.parameters.empty()) {
                if (verbose) {
                    dump += indentGuide(indentCt + 1) + "<parameters>\n";
                    for (auto&& param : func.parameters) {
                        dump += indentGuide(indentCt + 2) + "Param\n";
                        dump += indentGuide(indentCt + 3) + "<name>\n";
                        dump += recurDump(*param->paramRef, indentCt + 4, verbose) + "\n";
                        dump += indentGuide(indentCt + 3) + "<type>\n";
                        dump += recurDump(*param->type, indentCt + 4, verbose) + "\n";
                    }
                } else {
                    dump += indentGuide(indentCt + 1) + "(";
                    for (int i = 0; i < func.parameters.size(); i++) {
                        auto& param = func.parameters[i];
                        dump += printAST(*param->paramRef) + ":" + printAST(*param->type);
                        if (i + 1 < func.parameters.size()) {
                            dump += ", ";
                        }
                    }
                    dump += ")\n";
                }
            }
            if (func.returnType != nullptr) {
                if (verbose) {
                    dump += indentGuide(indentCt + 1) + "<returnType>\n";
                    dump += recurDump(*func.returnType, indentCt + 2, verbose) + "\n";
                } else {
                    dump += indentGuide(indentCt + 1) + "-> ";
                    dump += printAST(*func.returnType) + "\n";
                }
            }
            if (verbose) {
                dump += indentGuide(indentCt + 1) + "<blockStmt>\n";
                indentCt++;  // HACK to force an extra indent
            }
            dump += recurDump(*func.blockOrExpr, indentCt + 1, verbose);
        } break;
        case NodeType::NAME: {
            auto& name = static_cast<const ASTName&>(node);
            dump += indentGuide(indentCt) + "Name " + printAST(name);
        } break;
        case NodeType::DOT_OP: {
            auto& dotOp = static_cast<const ASTDotOp&>(node);
            dump += indentGuide(indentCt) + "DotOp\n";
            dump += indentGuide(indentCt + 1) + "<base>\n" + recurDump(*dotOp.base, indentCt + 2, verbose) + "\n";
            dump += indentGuide(indentCt + 1) + "<member>\n" + recurDump(*dotOp.member, indentCt + 2, verbose);
        } break;
        case NodeType::CALL: {
            auto& call = static_cast<const ASTCall&>(node);
            dump += indentGuide(indentCt) + "Call\n";
            dump += indentGuide(indentCt + 1);
            dump += "<callRef>\n" + recurDump(*call.callRef, indentCt + 2, verbose);
            if (!call.arguments.empty()) {
                dump += "\n";
                dump += indentGuide(indentCt + 1) + "<arguments>\n";
                for (auto&& expr : call.arguments) {
                    dump += recurDump(*expr, indentCt + 2, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            }
        } break;
        case NodeType::TYPE_INIT: {
            auto& typeInit = static_cast<const ASTTypeInit&>(node);
            dump += indentGuide(indentCt) + "TypeInit\n";
            dump += indentGuide(indentCt + 1) + "<typeRef>\n" + recurDump(*typeInit.typeRef, indentCt + 2, verbose);
            if (!typeInit.assignments.empty()) {
                dump += "\n";
                dump += indentGuide(indentCt + 1) + "<assignments>\n";
                for (auto&& assignment : typeInit.assignments) {
                    dump += indentGuide(indentCt + 2) + "Assignment\n";
                    dump += indentGuide(indentCt + 3) + "<fieldRef> " + printAST(*assignment->lvalue) + "\n";
                    dump += indentGuide(indentCt + 3) + "<rvalue>\n";
                    dump += recurDump(*assignment->rvalue, indentCt + 4, verbose) + "\n";
                }
                dump = dump.substr(0, dump.length() - 1);
            }
        } break;
        case NodeType::LIT: {
            auto& lit = static_cast<const ASTLit&>(node);
            dump += indentGuide(indentCt) + "Lit " + printAST(lit);
        } break;
        case NodeType::UN_OP: {
            auto& unOp = static_cast<const ASTUnOp&>(node);
            dump += indentGuide(indentCt) + "UnOp\n";
            dump += indentGuide(indentCt + 1) + "<op> " + tokenTypeToStr(unOp.op->type) + "\n";
            dump += indentGuide(indentCt + 1) + "<inner>\n" + recurDump(*unOp.inner, indentCt + 2, verbose);
        } break;
        case NodeType::DEREF: {
            auto& deref = static_cast<const ASTDeref&>(node);
            dump += indentGuide(indentCt) + "Deref\n";
            dump += indentGuide(indentCt + 1) + "<inner>\n" + recurDump(*deref.inner, indentCt + 2, verbose);
        } break;
        case NodeType::BIN_OP: {
            auto& binOp = static_cast<const ASTBinOp&>(node);
            dump += indentGuide(indentCt) + "BinOp\n";
            dump += indentGuide(indentCt + 1) + "<left>\n" + recurDump(*binOp.left, indentCt + 2, verbose) + "\n";
            dump += indentGuide(indentCt + 1) + "<op> " + tokenTypeToStr(binOp.op->type) + "\n";
            dump += indentGuide(indentCt + 1) + "<right>\n" + recurDump(*binOp.right, indentCt + 2, verbose);
        } break;
        default:
            ASSERT(false, "TODO DELETE: Unimplemented AST dump for node");
    }
    return dump;
}

std::string recurPrintExpr(const ASTExpression& expr) {
    std::string str;
    switch (expr.nodeType) {
        case NodeType::TYPE_LIT: {
            auto& typeLit = static_cast<const ASTTypeLit&>(expr);
            return tokenTypeToStr(typeLit.type);
        }
        case NodeType::FUNC_TYPE: {
            auto& funcType = static_cast<const ASTFuncType&>(expr);
            str += "(";
            for (int i = 0; i < funcType.inTypes.size(); i++) {
                str += recurPrintExpr(*funcType.inTypes[i]);
                if (i + 1 < funcType.inTypes.size()) {
                    str += ", ";
                }
            }
            str += ") -> " + recurPrintExpr(*funcType.outType);
        } break;
        case NodeType::MOD: {
            auto& mod = static_cast<const ASTTy&>(expr);

            std::string declListStr;
            for (int i = 0; i < mod.declarations.size(); i++) {
                auto&& decl = mod.declarations[i];
                declListStr += recurPrintExpr(*decl->lvalue);
                if (decl->type != nullptr) declListStr += ":" + recurPrintExpr(*decl->type);
                if (decl->rvalue != nullptr) {
                    declListStr += tokenTypeToStr(decl->assignType->type);
                    declListStr += recurPrintExpr(*decl->rvalue);
                }
                if (i + 1 < mod.declarations.size()) {
                    declListStr += ", ";
                }
            }
            return "mod {" + declListStr + "}";
        } break;
        case NodeType::TYPE_DEF: {
            auto& typeDef = static_cast<const ASTTy&>(expr);
            std::string declListStr;
            for (int i = 0; i < typeDef.declarations.size(); i++) {
                auto&& decl = typeDef.declarations[i];
                declListStr += printExpr(*decl->lvalue);
                if (decl->type != nullptr) declListStr += ":" + printExpr(*decl->type);
                if (decl->rvalue != nullptr) {
                    declListStr += tokenTypeToStr(decl->assignType->type);
                    declListStr += printExpr(*decl->rvalue);
                }
                if (i + 1 < typeDef.declarations.size()) {
                    declListStr += ", ";
                }
            }
            return "ty {" + declListStr + "}";
        } break;
        case NodeType::FUNC: {
            auto& func = static_cast<const ASTFunc&>(expr);
            std::string paramListStr;
            for (int i = 0; i < func.parameters.size(); i++) {
                auto&& param = func.parameters[i];
                paramListStr += printExpr(*param->paramRef) + ":" + printExpr(*param->type);
                if (i + 1 < func.parameters.size()) {
                    paramListStr += ", ";
                }
            }
            if (func.returnType != nullptr) {
                return "(" + paramListStr + ") -> " + printExpr(*func.returnType) + " {...}";
            } else {
                return "(" + paramListStr + ") {...}";
            }
        }
        case NodeType::NAME: {
            return static_cast<const ASTName&>(expr).ref->toStr();
        }
        case NodeType::DOT_OP: {
            auto& dotOp = static_cast<const ASTDotOp&>(expr);
            return "(" + recurPrintExpr(*dotOp.base) + "." + recurPrintExpr(*dotOp.member) + ")";
        }
        case NodeType::CALL: {
            auto& call = static_cast<const ASTCall&>(expr);
            std::string argStr;
            str = recurPrintExpr(*call.callRef) + "(";
            for (int i = 0; i < call.arguments.size(); i++) {
                str += recurPrintExpr(*call.arguments[i]);
                if (i + 1 < call.arguments.size()) {
                    str += ", ";
                }
            }
            str += ")";
        } break;
        case NodeType::TYPE_INIT: {
            auto& typeInit = static_cast<const ASTTypeInit&>(expr);
            str = recurPrintExpr(*typeInit.typeRef) + ".{";
            for (int i = 0; i < typeInit.assignments.size(); i++) {
                str += recurPrintExpr(*typeInit.assignments[i]->lvalue);
                str += "=";
                str += recurPrintExpr(*typeInit.assignments[i]->rvalue);
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
                    return lit.value->toStr();
                case TokenType::TRUE:
                    return "true";
                case TokenType::FALSE:
                    return "false";
                default:
                    ASSERT(false, "Not a literal: " + tokenTypeToStr(lit.value->type));
            }
        }
        case NodeType::UN_OP: {
            auto& unOp = static_cast<const ASTUnOp&>(expr);
            return tokenTypeToStr(unOp.op->type) + "(" + recurPrintExpr(*unOp.inner) + ")";
        }
        case NodeType::DEREF: {
            auto& deref = static_cast<const ASTDeref&>(expr);
            return "(" + recurPrintExpr(*deref.inner) + ").*";
        }
        case NodeType::BIN_OP: {
            auto& binOp = static_cast<const ASTBinOp&>(expr);
            return "(" + printExpr(*binOp.left) + " " + tokenTypeToStr(binOp.op->type) + " " + printExpr(*binOp.right) +
                   ")";
        }
        default:
            ASSERT(false, "Node is not an expression");
    }
    return str;
}

std::string recurPrintAST(const ASTNode& node, int indentCt) {
    std::string str;
    switch (node.nodeType) {
        case NodeType::PROGRAM: {
            auto& prgm = static_cast<const ASTProgram&>(node);
            for (auto&& decl : prgm.declarations) {
                str += recurPrintAST(*decl, indentCt) + "\n";
            }
        } break;
        case NodeType::BLOCK: {
            auto& block = static_cast<const ASTBlock&>(node);
            str += "{\n";
            for (auto& stmt : block.statements) {
                str += indent(indentCt + 1) + recurPrintAST(*stmt, indentCt + 1);
                str += "\n";
            }
            str += indent(indentCt) + "}";
        } break;
        case NodeType::IF: {
            auto& ifStmt = static_cast<const ASTIf&>(node);
            str += "if " + recurPrintAST(*ifStmt.condition, indentCt) + " ";
            str += recurPrintAST(*ifStmt.conseq, indentCt);
            if (ifStmt.alt != nullptr) {
                str += " else ";
                str += recurPrintAST(*ifStmt.alt, indentCt);
            }
        } break;
        case NodeType::WHILE: {
            auto& whileLoop = static_cast<const ASTWhile&>(node);
            str += "while " + recurPrintAST(*whileLoop.condition, indentCt) + " ";
            str += recurPrintAST(*whileLoop.blockStmt, indentCt);
        } break;
        case NodeType::FOR: {
            auto& forLoop = static_cast<const ASTFor&>(node);
            str += "for ";
            str += recurPrintAST(*forLoop.blockStmt, indentCt);
        } break;
        case NodeType::RET: {
            auto& ret = static_cast<const ASTRet&>(node);
            if (ret.retValue == nullptr) {
                return "return";
            } else {
                return "return " + recurPrintAST(*ret.retValue, indentCt);
            }
        }
        case NodeType::DECL: {
            auto& decl = static_cast<const ASTDecl&>(node);
            str += recurPrintAST(*decl.lvalue, indentCt);
            if (decl.type != nullptr) {
                str += ": " + recurPrintAST(*decl.type, indentCt);
            }
            if (decl.assignType != nullptr) {
                str += " " + tokenTypeToStr(decl.assignType->type) + " ";
            }
            if (decl.rvalue != nullptr) {
                str += recurPrintAST(*decl.rvalue, indentCt);
            }
        } break;
        case NodeType::MOD: {
            auto& mod = static_cast<const ASTMod&>(node);
            str += "mod {\n";
            for (auto&& decl : mod.declarations) {
                str += indent(indentCt + 1) + recurPrintAST(*decl, indentCt + 1);
                str += "\n";
            }
            str += indent(indentCt) + "}";
        } break;
        case NodeType::TYPE_DEF: {
            auto& typeDef = static_cast<const ASTTy&>(node);
            str += "ty {\n";
            for (auto&& decl : typeDef.declarations) {
                str += indent(indentCt + 1) + recurPrintAST(*decl, indentCt + 1);
                str += "\n";
            }
            str += indent(indentCt) + "}";
        } break;
        case NodeType::FUNC: {
            auto& func = static_cast<const ASTFunc&>(node);
            str = "(";
            for (int i = 0; i < func.parameters.size(); i++) {
                auto&& param = func.parameters[i];
                str += recurPrintAST(*param->paramRef, indentCt) + ":" + recurPrintAST(*param->type, indentCt);
                if (i + 1 < func.parameters.size()) {
                    str += ", ";
                }
            }
            if (func.returnType != nullptr) {
                str += ") -> " + recurPrintAST(*func.returnType, indentCt) + " ";
            } else {
                str += ") ";
            }
            str += recurPrintAST(*func.blockOrExpr, indentCt);
        } break;
        default:
            return recurPrintExpr(static_cast<const ASTExpression&>(node));
    }
    return str;
}

}  // namespace

void dumpAST(const ASTNode& node, bool verbose) { std::cout << recurDump(node, 0, verbose) << std::endl; }

std::string printExpr(const ASTExpression& expr) { return recurPrintExpr(expr); }
std::string printAST(const ASTNode& node) { return recurPrintAST(node, 0); }

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
