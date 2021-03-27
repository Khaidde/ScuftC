#include "parser.hpp"

#include <iostream>

#include "ast_printer.hpp"

namespace internal {
enum { LOWEST_PRECEDENCE = 0, HIGHEST_PRECEDENCE = 100 };

int get_precedence(TokenType type) {
    switch (type) {
        case TokenType::COND_OR:
            return 1;
        case TokenType::COND_XOR:
            return 2;
        case TokenType::COND_AND:
            return 3;

        case TokenType::BIT_OR:
            return 4;
        case TokenType::BIT_XOR:
            return 5;
        case TokenType::BIT_AND:
            return 6;

        case TokenType::COND_EQUALS:
        case TokenType::COND_NOT_EQUALS:
            return 7;
        case TokenType::COND_GREATER_EQUAL:
        case TokenType::COND_LESS_EQUAL:
        case TokenType::COND_GREATER:
        case TokenType::COND_LESS:
            return 8;

        case TokenType::BIT_SHIFT_LEFT:
        case TokenType::BIT_SHIFT_RIGHT:
            return 9;

        case TokenType::OP_ADD:
        case TokenType::OP_SUBTR:
            return 10;
        case TokenType::OP_MULT:
        case TokenType::OP_DIV:
        case TokenType::OP_MOD:
            return 11;
        case TokenType::OP_CARROT:
            return 12;

        case TokenType::DEREF:
        case TokenType::LEFT_PARENS:
        case TokenType::DOT:
            return 13;

        default:
            return LOWEST_PRECEDENCE;
    }
}
}  // namespace internal

void Parser::assert_token(TokenType type, const std::string& msg) {
    auto actualTkn = lexer.peek_token();
    if (actualTkn->type != type) {
        std::string expectedTknStr = token_type_to_str(type);
        std::string actualTknStr;
        switch (actualTkn->type) {
            case TokenType::IDENTIFIER:
                actualTknStr = "\"" + actualTkn->get_string_val() + "\"";
                break;
            case TokenType::INT_LITERAL:
                actualTknStr = std::to_string(actualTkn->longVal);
                break;
            case TokenType::DOUBLE_LITERAL:
                actualTknStr = std::to_string(actualTkn->doubleVal);
                break;
            case TokenType::STRING_LITERAL:
                actualTknStr = "String literal";
                break;
            case TokenType::END:
                break;
            default:
                actualTknStr = std::move(actualTkn->get_string_val());
        }
        auto error = dx.err_after_token("Expected " + expectedTknStr + " but found " + actualTknStr + " instead",
                                        *lexer.last_token());
        if (!msg.empty()) error->note(std::move(msg));
    }
}

std::unique_ptr<ASTNode> Parser::parse_program() {
    auto baseNode = std::make_unique<ASTNode>();
    baseNode->nodeType = NodeType::PROGRAM;
    auto prgm = &baseNode->prgm;

    while (!check_token(TokenType::END)) {
        auto decl = assert_parse_decl();
        if (decl->nodeType == NodeType::UNKNOWN) {
            dx.last_err()->note("Statements are never executed in global scope");
        } else {
            prgm->declarations.push_back(std::move(decl));
        }
        if (dx.has_errors()) return baseNode;
    }
    if (!prgm->declarations.empty()) {
        baseNode->endI = prgm->declarations.back()->endI;
    }
    return baseNode;
}

std::unique_ptr<ASTNode> Parser::parse_statement() {
    switch (lexer.peek_token()->type) {
        case TokenType::LEFT_CURLY:
            return parse_block();
        case TokenType::IF:
            return parse_if();
        case TokenType::FOR:
            return parse_for();
        case TokenType::BREAK: {
            auto brk = make_node(NodeType::BREAK, lexer.peek_token());
            lexer.next_token();  // Consume break
            if (!check_token(TokenType::RIGHT_CURLY)) {
                dx.err_token("Unreachable statement following break", *lexer.peek_token());
                dx.err_node("Break statement found here", *brk)->tag(ErrorMsg::EMPTY);
            }
            return brk;
        }
        case TokenType::CONTINUE: {
            auto cont = make_node(NodeType::CONT, lexer.peek_token());
            lexer.next_token();  // Consume continue
            if (!check_token(TokenType::RIGHT_CURLY)) {
                dx.err_token("Unreachable statement following continue", *lexer.peek_token());
                dx.err_node("Continue statement found here", *cont)->tag(ErrorMsg::EMPTY);
            }
            return cont;
        }
        case TokenType::RETURN:
            return parse_return();
        default: {
            auto declOrExpr = parse_decl_expr();
            if (declOrExpr->nodeType == NodeType::UNKNOWN) {
                dx.pop_last_err();
                dx.err_node("Expected a statement keyword like if, for, etc", *declOrExpr);
            }
            return declOrExpr;
        }
    }
}

std::unique_ptr<ASTNode> Parser::parse_block() {
    ASSERT(lexer.peek_token()->type == TokenType::LEFT_CURLY, "Block must start with {");
    auto baseNode = make_node(NodeType::BLOCK, lexer.next_token());
    auto block = &baseNode->block;  // Consume {

    while (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
        block->statements.push_back(parse_statement());
        if (dx.has_errors()) return baseNode;
    }
    if (check_token(TokenType::END)) {
        dx.err_node("Mismatched curly brackets. Start of block found here", *baseNode);
        dx.err_after_token("Reached end of file before finding a closing }", *lexer.last_token())
            ->tag(ErrorMsg::EMPTY)
            ->fix("Add a closing }");
    } else {
        baseNode->endI = lexer.next_token()->endI;  // Consume }
    }

    return baseNode;
}

std::unique_ptr<ASTNode> Parser::parse_if() {
    ASSERT(lexer.peek_token()->type == TokenType::IF, "If statement must start with an if token");
    auto baseNode = make_node(NodeType::IF, lexer.next_token());  // Consume if
    auto ifNode = &baseNode->ifNode;

    ifNode->condition = parse_expr();
    if (ifNode->condition->nodeType == NodeType::UNKNOWN) return baseNode;

    if (check_token(TokenType::LEFT_CURLY)) {
        ifNode->conseq = parse_block();
    } else {
        ifNode->conseq = parse_statement();
        if (dx.has_errors()) return baseNode;
    }
    if (check_token(TokenType::ELSE)) {
        lexer.next_token();  // Consume else
        if (check_token(TokenType::END)) {
            dx.err_after_token("Unterminated code at end of file. Expected if keyword or {", *lexer.last_token());
            return baseNode;
        } else if (check_token(TokenType::IF)) {
            ifNode->alt = parse_if();
        } else {
            if (check_token(TokenType::LEFT_CURLY)) {
                ifNode->alt = parse_block();
            } else {
                ifNode->alt = parse_statement();
            }
        }
    }
    baseNode->endI = lexer.last_token()->endI;

    return baseNode;
}

std::unique_ptr<ASTNode> Parser::parse_for() {
    ASSERT(lexer.peek_token()->type == TokenType::FOR, "For statement must start with an for token");
    auto baseNode = make_node(NodeType::FOR, lexer.next_token());  // Consume for
    auto forLoop = &baseNode->forLoop;

    if (!check_token(TokenType::LEFT_CURLY)) {
        if (check_token(TokenType::COMMA)) {
            dx.err_token("Expected a statement", *lexer.peek_token())
                ->fix("Add _ to declare an empty initial statement");
        } else {
            forLoop->initial = parse_statement();
            if (check_token(TokenType::COMMA)) {
                lexer.next_token();  // Consume ,
                if (check_token(TokenType::LEFT_CURLY)) {
                    dx.err_node("Conditional expression can't be a block", *parse_block());
                } else {
                    forLoop->condition = parse_expr();
                    if (forLoop->condition->nodeType == NodeType::UNKNOWN) {
                        dx.last_err()->note("For loop condition must be an expression");
                    }
                    if (check_token(TokenType::COMMA)) {
                        lexer.next_token();  // Consume ,
                        if (check_token(TokenType::LEFT_CURLY)) {
                            dx.err_token("Block statement is not allowed after the conditional expression",
                                         *lexer.peek_token())
                                ->note("_ can be used to declare an empty post statement");
                            parse_block();
                        } else {
                            forLoop->post = parse_statement();
                        }
                    } else if (check_token(TokenType::LEFT_CURLY)) {
                        dx.err_after_token("Expected another statement after the condition", *lexer.last_token())
                            ->note("_ can be used to declare an empty post statement");
                    } else {
                        dx.err_after_token("Expected a comma after the conditional", *lexer.last_token());
                        if (check_token(TokenType::ASSIGNMENT)) {
                            dx.last_err()
                                ->fix("Replace = with ==")
                                ->note("= (assignment) may have been confused for == (equivalence operator)");
                        } else {
                            dx.last_err()->fix("Add ,");
                        }
                    }
                }
            } else if (check_token(TokenType::LEFT_CURLY)) {
                if (is_expression_type(forLoop->initial->nodeType)) {
                    forLoop->condition = std::move(forLoop->initial);
                } else {
                    dx.err_node("For loop condition must be an expression", *forLoop->initial);
                }
            } else {
                dx.err_after_token("Expected a comma after the initial statement", *lexer.last_token());
            }
        }
    }
    if (check_token(TokenType::LEFT_CURLY)) forLoop->blockStmt = parse_block();
    baseNode->endI = lexer.last_token()->endI;

    return baseNode;
}

std::unique_ptr<ASTNode> Parser::parse_return() {
    auto baseNode = make_node(NodeType::RET, lexer.next_token());  // Consume return
    auto ret = &baseNode->ret;

    if (check_token(TokenType::RIGHT_CURLY)) {
        ret->retExpr = nullptr;
    } else {
        ret->retExpr = parse_expr();
    }
    baseNode->endI = lexer.last_token()->endI;
    if (!check_token(TokenType::RIGHT_CURLY)) {
        dx.err_token("Unreachable statement following return", *lexer.peek_token());
        dx.err_node("Return statement found here", *baseNode)->tag(ErrorMsg::EMPTY);
    }
    return baseNode;
}

std::unique_ptr<ASTNode> Parser::assert_parse_decl() {
    int beginI = lexer.peek_token()->beginI;
    switch (lexer.peek_token()->type) {
        case TokenType::IF: {
            dx.set_recover_mode(true);
            auto ifStmt = parse_if();
            dx.set_recover_mode(false);
            dx.err_node("If statement is not allowed here", *ifStmt);
            break;
        }
        case TokenType::FOR: {
            dx.set_recover_mode(true);
            auto forLoop = parse_for();
            dx.set_recover_mode(false);
            dx.err_node("For loop is not allowed here", *forLoop);
            break;
        }
        case TokenType::BREAK:
        case TokenType::CONTINUE:
            dx.err_token(token_type_to_str(lexer.peek_token()->type) + " is not allowed here", *lexer.peek_token());
            break;
        case TokenType::RETURN: {
            dx.set_recover_mode(true);
            auto ret = parse_return();
            dx.set_recover_mode(false);
            dx.err_node("Return is not allowed here", *ret);
            break;
        }
        default:
            auto declOrExpr = parse_decl_expr();
            if (declOrExpr->nodeType == NodeType::DECL) {
                return declOrExpr;
            } else if (declOrExpr->nodeType != NodeType::UNKNOWN) {
                dx.err_node(std::string(node_type_to_str(declOrExpr->nodeType)) + " is not allowed here", *declOrExpr);
            }
    }
    return unknown_node(beginI, lexer.last_token()->endI);
}

std::unique_ptr<ASTNode> Parser::parse_decl_expr() {
    auto baseNode = make_node(NodeType::DECL, lexer.peek_token());
    auto decl = &baseNode->decl;

    decl->lvalue = parse_expr();

    if (check_token(TokenType::COLON)) {
        lexer.next_token();  // Consume :
        decl->type = parse_expr();
    }

    if (check_token(TokenType::ASSIGNMENT) || check_token(TokenType::CONST_ASSIGNMENT)) {
        decl->assignType = lexer.next_token();
        decl->rvalue = parse_expr();
        baseNode->endI = decl->rvalue->endI;
    } else if (decl->type == nullptr) {
        // if no type or assignment is detected, then the statement is a free floating expression
        return std::move(decl->lvalue);
    } else {
        decl->assignType = nullptr;
        baseNode->endI = decl->type->endI;
    }

    return baseNode;
}

std::unique_ptr<ASTNode> Parser::parse_left_paren_expr() {
    ASSERT(lexer.peek_token()->type == TokenType::LEFT_PARENS, "Left parenthesis expression must start with (");
    auto baseNode = make_node(NodeType::FUNC, lexer.next_token());  // Consume (
    auto func = &baseNode->func;

    bool isFunction = false;
    while (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END)) {
        auto funcParam = parse_decl_expr();
        if (funcParam->nodeType == NodeType::DECL) {
            func->parameters.push_back(std::move(funcParam));
        } else {
            if (funcParam->nodeType != NodeType::UNKNOWN) {
                if (isFunction) {
                    dx.err_node("Expression is not allowed in function parameter list", *funcParam);
                } else {
                    return parse_function_type(funcParam);
                }
            }
            dx.last_err()->note("Function parameter must declare a variable");
        }
        if (check_token(TokenType::COMMA)) {
            lexer.next_token();  // Consume ,
            if (check_token(TokenType::RIGHT_PARENS)) {
                dx.err_after_token("Expected another function parameter after the comma", *lexer.last_token());
            }
        } else if (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END)) {
            dx.err_after_token("Expected either , or ) in function parameter list", *lexer.last_token());
        }
        isFunction = true;
    }
    if (check_token(TokenType::END)) {
        dx.err_after_token("Unterminated code at end of file. Expected another function parameter", *lexer.last_token())
            ->fix("Add ) or another function parameter");
    } else {
        lexer.next_token();  // Consume )
    }

    if (check_token(TokenType::ARROW)) {
        lexer.next_token();  // Consume ->
        func->returnType = parse_expr();
    } else {
        func->returnType = nullptr;
    }

    if (check_token(TokenType::SINGLE_RETURN)) {
        lexer.next_token();  // Consume ::
        if (check_token(TokenType::RETURN)) {
            dx.err_token("Return is not allowed here", *lexer.next_token())
                ->fix("Delete return keyword")
                ->note("Function shorthand must be in the form (...) :: [expression]");  // Consume return
        }
        func->expr = parse_expr();
        func->isShorthand = true;
        if (func->expr->nodeType == NodeType::UNKNOWN) {
            dx.last_err()->note("Must have an expression immediately after ::");
        }
    } else if (check_token(TokenType::END) && func->returnType == nullptr) {
        dx.err_after_token("Expected a return type or function block", *lexer.last_token());
    } else if (!check_token(TokenType::LEFT_CURLY)) {
        if (func->returnType == nullptr) {
            dx.err_after_token("Function body must start with { and end with }", *lexer.last_token())->fix("Add {");
        } else {
            auto funcTypeNode = make_node(NodeType::FUNC, baseNode);
            funcTypeNode->funcType.outType = std::move(func->returnType);
            funcTypeNode->endI = funcTypeNode->funcType.outType->endI;
            return funcTypeNode;
        }
    } else {
        func->block = parse_block();
        func->isShorthand = false;
    }
    baseNode->endI = lexer.last_token()->endI;
    return baseNode;
}

std::unique_ptr<ASTNode> Parser::parse_function_type(std::unique_ptr<ASTNode>& reduceExpr) {
    auto baseNode = make_node(NodeType::FUNC_TYPE, reduceExpr);
    auto funcType = &baseNode->funcType;

    do {
        if (funcType->inTypes.empty()) {
            funcType->inTypes.push_back(std::move(reduceExpr));
        } else {
            funcType->inTypes.push_back(parse_expr());
        }

        if (check_token(TokenType::COMMA)) {
            lexer.next_token();  // Consume ,
            if (check_token(TokenType::RIGHT_PARENS)) {
                dx.err_after_token("Expected another type after the comma", *lexer.last_token());
            }
        } else if (!check_token(TokenType::RIGHT_PARENS)) {
            if (funcType->inTypes.size() == 1) {
                if (unbalancedParenErrI != lexer.last_token()->endI) {
                    dx.err_after_token("Not enough parenthesis", *lexer.last_token())
                        ->fix("Add " + std::to_string(exprDepth) + " more )");
                    unbalancedParenErrI = lexer.last_token()->endI;
                }
                return std::move(funcType->inTypes.back());
            } else if (!check_token(TokenType::END)) {
                dx.err_after_token("Expected either , or ) in function input type list", *lexer.last_token());
            }
        }
    } while (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END));

    if (check_token(TokenType::END)) {
        dx.err_after_token("Unterminated code at end of file. Expected another type", *lexer.last_token())
            ->fix("Add ) or another type");
    } else {
        lexer.next_token();  // Consume )

        if (check_token(TokenType::ARROW)) {
            lexer.next_token();  // Consume ->
            funcType->outType = parse_expr();
        } else {
            if (funcType->inTypes.size() == 1) {
                return std::move(funcType->inTypes.front());
            } else {
                dx.err_after_token("Function type must explicitly declare a return type", *lexer.last_token())
                    ->note("Use the format ([input type 1], [input type 2], ...) -> [return type]");
            }
        }
    }
    baseNode->endI = lexer.last_token()->endI;
    return baseNode;
}

std::unique_ptr<ASTNode> Parser::parse_expr() { return recur_expr(internal::LOWEST_PRECEDENCE); }

std::unique_ptr<ASTNode> Parser::parse_operand() {
    auto tkn = lexer.peek_token();
    switch (tkn->type) {
        case TokenType::MOD: {
            // parse_mod
            auto modNode = make_node(NodeType::MOD, tkn);
            lexer.next_token();  // Consume mod

            if (check_token(TokenType::LEFT_CURLY)) {
                lexer.next_token();  // Consume {
            } else {
                dx.err_after_token("Expected { after mod keyword", *lexer.last_token())
                    ->fix("Add {")
                    ->note("Module must be of the form mod {...}");
            }

            while (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
                auto decl = assert_parse_decl();
                if (decl->nodeType == NodeType::UNKNOWN) {
                    dx.last_err()->note("Statements are never executed in the declaration of a module");
                } else {
                    modNode->mod.declarations.push_back(std::move(decl));
                }
            }
            if (check_token(TokenType::END)) {
                dx.err_after_token("Unterminated code at end of file. Expected closing }", *lexer.last_token())
                    ->fix("Add } or another declaration");
            } else {
                lexer.next_token();  // Consume }
            }
            modNode->endI = lexer.last_token()->endI;
            return modNode;
        }
        case TokenType::TY: {
            // parse_ty
            auto tyNode = make_node(NodeType::TYPE_DEF, tkn);
            lexer.next_token();  // Consume ty

            if (check_token(TokenType::LEFT_CURLY)) {
                lexer.next_token();  // Consume {
            } else {
                dx.err_after_token("Expected { after ty keyword", *lexer.last_token())
                    ->fix("Add {")
                    ->note("Type definition must be of the form ty {...}");
            }

            while (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
                auto decl = assert_parse_decl();
                if (decl->nodeType == NodeType::UNKNOWN) {
                    dx.last_err()->note("Statements are never executed in the declaration of a type");
                } else {
                    tyNode->ty.declarations.push_back(std::move(decl));
                }
            }
            if (check_token(TokenType::END)) {
                dx.err_after_token("Unterminated code at end of file. Expected closing }", *lexer.last_token())
                    ->fix("Add } or another declaration");
            } else {
                lexer.next_token();  // Consume }
            }
            tyNode->endI = lexer.last_token()->endI;
            return tyNode;
        }
        case TokenType::IDENTIFIER: {
            auto nameNode = make_node(NodeType::NAME, tkn);
            nameNode->name.ref = lexer.next_token();  // Consume [identifier]
            return nameNode;
        }
        case TokenType::VOID_TYPE:
        case TokenType::MOD_TYPE:
        case TokenType::TY_TYPE:
        case TokenType::INT_TYPE:
        case TokenType::DOUBLE_TYPE:
        case TokenType::STRING_TYPE:
        case TokenType::BOOL_TYPE: {
            auto typeLitNode = make_node(NodeType::TYPE_LIT, tkn);
            typeLitNode->typeLit.type = lexer.next_token()->type;  // Consume [type token]
            return typeLitNode;
        }
        case TokenType::INT_LITERAL:
        case TokenType::DOUBLE_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::TRUE:
        case TokenType::FALSE: {
            auto litNode = make_node(NodeType::LIT, tkn);
            litNode->lit.value = lexer.next_token();  // Consume [literal token]
            return litNode;
        }
        case TokenType::COND_NOT:
        case TokenType::BIT_NOT:
        case TokenType::OP_SUBTR:
        case TokenType::OP_MULT: {
            auto unOpNode = make_node(NodeType::UN_OP, tkn);
            unOpNode->unOp.op = lexer.next_token();
            unOpNode->unOp.inner = recur_expr(internal::HIGHEST_PRECEDENCE);
            unOpNode->endI = unOpNode->unOp.inner->endI;
            return unOpNode;
        }
        case TokenType::LEFT_PARENS: {
            exprDepth++;
            auto leftParenExpr = parse_left_paren_expr();
            exprDepth--;
            return leftParenExpr;
        }
        default:
            if (check_token(TokenType::END)) {
                dx.err_after_token("Expected an expression but instead reached the end of the file",
                                   *lexer.last_token());
            } else if (check_token(TokenType::RIGHT_PARENS) && exprDepth == 0) {
                dx.err_token("Too many closing parenthesis", *lexer.peek_token())->fix("Delete )");
            } else {
                dx.err_token(
                    "Expected an expression but found " + token_type_to_str(lexer.peek_token()->type) + " instead",
                    *lexer.peek_token());
            }
            lexer.next_token();  // Consume [unknown expr token]
            return unknown_node(tkn->beginI, tkn->endI);
    }
}

std::unique_ptr<ASTNode> Parser::recur_expr(int prec) {
    std::unique_ptr<ASTNode> expr = parse_operand();

    for (;;) {
        int peekedPrec = internal::get_precedence(lexer.peek_token()->type);
        if (peekedPrec > prec) {
            if (check_token(TokenType::LEFT_PARENS)) {
                exprDepth++;
                // parse_call
                auto callNode = make_node(NodeType::CALL, expr);
                auto call = &callNode->call;
                call->callRef = std::move(expr);

                lexer.next_token();  // Consume (
                while (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END)) {
                    call->arguments.push_back(parse_expr());

                    if (check_token(TokenType::COMMA)) {
                        lexer.next_token();  // Consume ,
                        if (check_token(TokenType::RIGHT_PARENS)) {
                            dx.err_after_token("[" + ast_to_line(call->callRef.get()) +
                                                   "]: Expected another expression after the comma",
                                               *lexer.last_token());
                        }
                    } else if (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END)) {
                        dx.err_after_token(
                            "[" + ast_to_line(call->callRef.get()) + "]: Expected either , or ) in call argument list",
                            *lexer.last_token());
                    }
                }
                if (check_token(TokenType::END)) {
                    dx.err_after_token("[" + ast_to_line(call->callRef.get()) +
                                           "]: Unterminated code at end of file. Expected another expression",
                                       *lexer.last_token())
                        ->fix("Add ) or another expression");
                } else if (check_token(TokenType::RIGHT_PARENS)) {
                    lexer.next_token();  // Consume )
                }
                callNode->endI = lexer.last_token()->endI;
                expr = std::move(callNode);
                exprDepth--;
            } else if (check_token(TokenType::DEREF)) {
                auto derefNode = make_node(NodeType::DEREF, lexer.next_token());  // Consume .*
                derefNode->deref.inner = std::move(expr);
                derefNode->endI = derefNode->deref.inner->endI;
                expr = std::move(derefNode);
            } else if (check_token(TokenType::DOT)) {
                lexer.next_token();  // Consume .
                if (check_token(TokenType::LEFT_CURLY)) {
                    // parse_type_init
                    auto typeInitNode = make_node(NodeType::TYPE_INIT, expr);
                    auto typeInit = &typeInitNode->typeInit;
                    lexer.next_token();  // Consume {
                    typeInit->typeRef = std::move(expr);

                    while (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
                        auto declOrExprNode = parse_decl_expr();
                        if (declOrExprNode->nodeType == NodeType::DECL) {
                            auto assignmentDecl = std::move(declOrExprNode->decl);
                            if (assignmentDecl.type != nullptr) {
                                dx.err_node("[" + ast_to_line(typeInit->typeRef.get()) +
                                                "]: Variable declaration is not allowed here",
                                            *declOrExprNode)
                                    ->fix("Delete the type, " + ast_to_line(assignmentDecl.type.get()));
                            }
                            typeInit->assignments.push_back(std::move(declOrExprNode));
                        } else if (declOrExprNode->nodeType != NodeType::UNKNOWN) {
                            dx.err_node(
                                "[" + ast_to_line(typeInit->typeRef.get()) + "]: Expression is not allowed here",
                                *declOrExprNode);
                        }
                        if (check_token(TokenType::COMMA)) {
                            lexer.next_token();  // Consume ,
                        } else if (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
                            dx.err_after_token("[" + ast_to_line(typeInit->typeRef.get()) +
                                                   "]: Expected either , or } in type initialization list",
                                               *lexer.last_token());
                        }
                    }
                    if (check_token(TokenType::END)) {
                        dx.err_after_token("[" + ast_to_line(typeInit->typeRef.get()) +
                                               "]: Unterminated code at end of file. Expected closing }",
                                           *lexer.last_token())
                            ->fix("Add } or another assignment");
                    } else if (check_token(TokenType::RIGHT_CURLY)) {
                        lexer.next_token();  // Consume }
                    }
                    typeInitNode->endI = lexer.last_token()->endI;
                    expr = std::move(typeInitNode);
                } else {
                    // parse_dot_op
                    auto dotOpNode = make_node(NodeType::DOT_OP, expr);
                    auto dotOp = &dotOpNode->dotOp;
                    dotOp->base = std::move(expr);
                    auto member = recur_expr(peekedPrec);
                    if (member->nodeType != NodeType::NAME) {
                        dx.err_node("Expected a member variable of " + ast_to_line(dotOp->base.get()) + " but found " +
                                        ast_to_line(member.get()) + " instead",
                                    *member);
                        dotOp->member = unknown_node(member->beginI, member->endI);
                    } else {
                        dotOp->member = std::move(member);
                    }
                    dotOpNode->endI = dotOp->member->endI;
                    expr = std::move(dotOpNode);
                }
            } else {
                auto binOpNode = make_node(NodeType::BIN_OP, expr);
                auto binOp = &binOpNode->binOp;
                binOp->left = std::move(expr);
                binOp->op = lexer.next_token();
                binOp->right = recur_expr(peekedPrec);
                binOpNode->endI = binOp->right->endI;
                expr = std::move(binOpNode);
            }
        } else {
            return expr;
        }
    }
}