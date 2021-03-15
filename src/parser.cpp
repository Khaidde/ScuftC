#include "parser.hpp"

#include <iostream>

int Parser::get_precedence(TokenType type) {
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

void Parser::assert_token(TokenType type, const std::string& msg) {
    auto actualTkn = lexer.peekToken();
    if (actualTkn->type != type) {
        std::string expectedTknStr = tokenTypeToStr(type);
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
                                        *lexer.lastToken());
        if (!msg.empty()) error->note(std::move(msg));
    }
}

std::unique_ptr<ASTProgram> Parser::parse_program() {
    auto prgm = std::make_unique<ASTProgram>();

    while (!check_token(TokenType::END)) {
        auto decl = assert_parse_decl();
        if (decl->nodeType == NodeType::UNKNOWN) {
            dx.last_err()->note("Statements are never executed in global scope");
        } else {
            prgm->declarations.push_back(std::move(decl));
        }
        if (dx.has_errors()) return prgm;
    }
    if (!prgm->declarations.empty()) {
        prgm->endI = prgm->declarations.back()->endI;
    }
    return prgm;
}

std::unique_ptr<ASTNode> Parser::parse_statement() {
    switch (lexer.peekToken()->type) {
        case TokenType::LEFT_CURLY:
            return parse_block();
        case TokenType::IF:
            return parse_if();
        case TokenType::WHILE:
            return parse_while();
        case TokenType::FOR:
            return parse_for();
        case TokenType::BREAK: {
            auto brk = make_node<ASTBreak>(*lexer.peekToken());
            lexer.nextToken();  // Consume break
            if (!check_token(TokenType::RIGHT_CURLY)) {
                dx.err_token("Unreachable statement following break", *lexer.peekToken());
                dx.err_node("Break statement found here", *brk)->tag(ErrorMsg::EMPTY);
            }
            return brk;
        }
        case TokenType::CONTINUE: {
            auto cont = make_node<ASTCont>(*lexer.peekToken());
            lexer.nextToken();  // Consume continue
            if (!check_token(TokenType::RIGHT_CURLY)) {
                dx.err_token("Unreachable statement following continue", *lexer.peekToken());
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
                dx.err_node("Expected a statement keyword like if, while, for, etc", *declOrExpr);
            }
            return declOrExpr;
        }
    }
}

std::unique_ptr<ASTBlock> Parser::parse_block() {
    ASSERT(lexer.peekToken()->type == TokenType::LEFT_CURLY, "Block must start with {");
    auto block = make_node<ASTBlock>(*lexer.nextToken());  // Consume {

    while (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
        block->statements.push_back(parse_statement());
        if (dx.has_errors()) return block;
    }
    if (check_token(TokenType::END)) {
        dx.err_node("Mismatched curly brackets. Start of block found here", *block);
        dx.err_after_token("Reached end of file before finding a closing }", *lexer.lastToken())
            ->tag(ErrorMsg::EMPTY)
            ->fix("Add a closing }");
    } else {
        block->endI = lexer.nextToken()->endI;  // Consume }
    }

    return block;
}

std::unique_ptr<ASTIf> Parser::parse_if() {
    ASSERT(lexer.peekToken()->type == TokenType::IF, "If statement must start with an if token");
    auto ifStmt = make_node<ASTIf>(*lexer.nextToken());  // Consume if
    ifStmt->condition = parse_expr();
    if (ifStmt->condition->nodeType == NodeType::UNKNOWN) return ifStmt;

    if (check_token(TokenType::LEFT_CURLY)) {
        ifStmt->conseq = parse_block();
    } else {
        ifStmt->conseq = parse_statement();
        if (dx.has_errors()) return ifStmt;
    }
    if (check_token(TokenType::ELSE)) {
        lexer.nextToken();  // Consume else
        if (check_token(TokenType::END)) {
            dx.err_after_token("Unterminated code at end of file. Expected if keyword or {", *lexer.lastToken());
            return ifStmt;
        } else if (check_token(TokenType::IF)) {
            ifStmt->alt = parse_if();
        } else {
            if (check_token(TokenType::LEFT_CURLY)) {
                ifStmt->alt = parse_block();
            } else {
                ifStmt->alt = parse_statement();
            }
        }
    }
    ifStmt->endI = lexer.lastToken()->endI;

    return ifStmt;
}

std::unique_ptr<ASTWhile> Parser::parse_while() {
    ASSERT(lexer.peekToken()->type == TokenType::WHILE, "While statement must start with an while token");
    auto whileLoop = make_node<ASTWhile>(*lexer.nextToken());  // Consume while
    whileLoop->condition = parse_expr();
    if (dx.has_errors()) return whileLoop;

    if (check_token(TokenType::LEFT_CURLY)) {
        whileLoop->blockStmt = parse_block();
    } else {
        whileLoop->blockStmt = parse_statement();
    }
    whileLoop->endI = lexer.lastToken()->endI;
    return whileLoop;
}

std::unique_ptr<ASTFor> Parser::parse_for() {
    ASSERT(lexer.peekToken()->type == TokenType::FOR, "For statement must start with an for token");
    auto forLoop = make_node<ASTFor>(*lexer.nextToken());  // Consume for

    if (check_token(TokenType::LEFT_CURLY)) {
        forLoop->blockStmt = parse_block();
    } else {
        ASSERT(false, "Unimplemented for loop parsing for for loops not of the form for {...}");
    }
    forLoop->endI = forLoop->blockStmt->endI;

    return forLoop;
}

std::unique_ptr<ASTRet> Parser::parse_return() {
    auto ret = make_node<ASTRet>(*lexer.nextToken());  // Consume return
    if (!check_token(TokenType::RIGHT_CURLY)) {
        ret->retValue = parse_expr();
        ret->endI = ret->retValue->endI;
    } else {
        ret->retValue = nullptr;
        ret->endI = lexer.lastToken()->endI;
    }
    if (!check_token(TokenType::RIGHT_CURLY)) {
        dx.err_token("Unreachable statement following return", *lexer.peekToken());
        dx.err_node("Return statement found here", *ret)->tag(ErrorMsg::EMPTY);
    }
    return ret;
}

std::unique_ptr<ASTDecl> Parser::assert_parse_decl() {
    int beginI = lexer.peekToken()->beginI;
    switch (lexer.peekToken()->type) {
        case TokenType::IF: {
            dx.set_recover_mode(true);
            auto ifStmt = parse_if();
            dx.set_recover_mode(false);
            dx.err_node("If statement is not allowed here", *ifStmt);
            break;
        }
        case TokenType::WHILE: {
            dx.set_recover_mode(true);
            auto whileLoop = parse_while();
            dx.set_recover_mode(false);
            dx.err_node("While loop is not allowed here", *whileLoop);
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
            dx.err_token(tokenTypeToStr(lexer.peekToken()->type) + " is not allowed here", *lexer.peekToken());
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
                return castNodePtr<ASTDecl>(declOrExpr);
            } else if (declOrExpr->nodeType != NodeType::UNKNOWN) {
                dx.err_node(nodeTypeToStr(declOrExpr->nodeType) + " is not allowed here", *declOrExpr);
            }
    }
    return unknownNode<ASTDecl>(beginI, lexer.lastToken()->endI);
}

std::unique_ptr<ASTNode> Parser::parse_decl_expr() {
    auto decl = make_node<ASTDecl>(*lexer.peekToken());

    decl->lvalue = parse_expr();

    if (check_token(TokenType::COLON)) {
        lexer.nextToken();  // Consume :
        decl->type = parse_expr();
    }

    if (check_token(TokenType::ASSIGNMENT) || check_token(TokenType::CONST_ASSIGNMENT)) {
        decl->assignType = lexer.nextToken();
        decl->rvalue = parse_expr();
        decl->endI = decl->rvalue->endI;
    } else if (decl->type == nullptr) {
        // if no type or assignment is detected, then the statement is a free floating expression
        return castNodePtr<ASTNode>(decl->lvalue);
    } else {
        decl->assignType = nullptr;
        decl->endI = decl->type->endI;
    }

    if (decl->lvalue->nodeType == NodeType::NAME) {
        auto name = static_cast<ASTName&>(*decl->lvalue);
        if (!globalTable.find(name.ref)) {
            globalTable.insert(name.ref, decl.get());
        }
    }

    return decl;
}

std::unique_ptr<ASTExpression> Parser::parse_left_paren_expr() {
    ASSERT(lexer.peekToken()->type == TokenType::LEFT_PARENS, "Left parenthesis expression must start with (");
    auto func = make_node<ASTFunc>(*lexer.nextToken());  // Consume (

    bool isFunction = false;
    while (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END)) {
        auto funcParam = parse_decl_expr();
        if (funcParam->nodeType == NodeType::DECL) {
            func->parameters.push_back(castNodePtr<ASTDecl>(funcParam));
        } else {
            if (funcParam->nodeType != NodeType::UNKNOWN) {
                if (isFunction) {
                    dx.err_node("Expression is not allowed in function parameter list", *funcParam);
                } else {
                    return parse_function_type(castNodePtr<ASTExpression>(funcParam));
                }
            }
            dx.last_err()->note("Function parameter must declare a variable");
        }
        if (check_token(TokenType::COMMA)) {
            lexer.nextToken();  // Consume ,
            if (check_token(TokenType::RIGHT_PARENS)) {
                dx.err_after_token("Expected another function parameter after the comma", *lexer.lastToken());
            }
        } else if (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END)) {
            dx.err_after_token("Expected either , or ) in function parameter list", *lexer.lastToken());
        }
        isFunction = true;
    }
    if (check_token(TokenType::END)) {
        dx.err_after_token("Unterminated code at end of file. Expected another function parameter", *lexer.lastToken())
            ->fix("Add ) or another function parameter");
    } else {
        lexer.nextToken();  // Consume )
    }

    if (check_token(TokenType::ARROW)) {
        lexer.nextToken();  // Consume ->
        func->returnType = parse_expr();
    } else {
        func->returnType = nullptr;
    }

    if (check_token(TokenType::SINGLE_RETURN)) {
        lexer.nextToken();  // Consume ::
        if (check_token(TokenType::RETURN)) {
            dx.err_token("Return is not allowed here", *lexer.nextToken())
                ->fix("Delete return keyword")
                ->note("Function shorthand must be in the form (...) :: [expression]");  // Consume return
        }
        func->blockOrExpr = parse_expr();
        if (func->blockOrExpr->nodeType == NodeType::UNKNOWN) {
            dx.last_err()->note("Must have an expression immediately after ::");
        }
    } else if (check_token(TokenType::END) && func->returnType == nullptr) {
        dx.err_after_token("Expected a return type or function block", *lexer.lastToken());
    } else if (!check_token(TokenType::LEFT_CURLY)) {
        if (func->returnType == nullptr) {
            dx.err_after_token("Function body must start with { and end with }", *lexer.lastToken())->fix("Add {");
        } else {
            auto funcType = make_node<ASTFuncType>(*func);
            funcType->outType = std::move(func->returnType);
            funcType->endI = funcType->outType->endI;
            return funcType;
        }
    } else {
        func->blockOrExpr = parse_block();
    }
    func->endI = lexer.lastToken()->endI;
    return func;
}

std::unique_ptr<ASTExpression> Parser::parse_function_type(std::unique_ptr<ASTExpression> reduceExpr) {
    auto funcType = make_node<ASTFuncType>(*reduceExpr);

    do {
        if (funcType->inTypes.empty()) {
            funcType->inTypes.push_back(std::move(reduceExpr));
        } else {
            funcType->inTypes.push_back(parse_expr());
        }

        if (check_token(TokenType::COMMA)) {
            lexer.nextToken();  // Consume ,
            if (check_token(TokenType::RIGHT_PARENS)) {
                dx.err_after_token("Expected another type after the comma", *lexer.lastToken());
            }
        } else if (!check_token(TokenType::RIGHT_PARENS)) {
            if (funcType->inTypes.size() == 1) {
                if (unbalancedParenErrI != lexer.lastToken()->endI) {
                    dx.err_after_token("Not enough parenthesis", *lexer.lastToken())
                        ->fix("Add " + std::to_string(exprDepth) + " more )");
                    unbalancedParenErrI = lexer.lastToken()->endI;
                }
                return std::move(funcType->inTypes.back());
            } else if (!check_token(TokenType::END)) {
                dx.err_after_token("Expected either , or ) in function input type list", *lexer.lastToken());
            }
        }
    } while (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END));

    if (check_token(TokenType::END)) {
        dx.err_after_token("Unterminated code at end of file. Expected another type", *lexer.lastToken())
            ->fix("Add ) or another type");
    } else {
        lexer.nextToken();  // Consume )

        if (check_token(TokenType::ARROW)) {
            lexer.nextToken();  // Consume ->
            funcType->outType = parse_expr();
        } else {
            if (funcType->inTypes.size() == 1) {
                return std::move(funcType->inTypes.front());
            } else {
                dx.err_after_token("Function type must explicitly declare a return type", *lexer.lastToken())
                    ->note("Use the format ([input type 1], [input type 2], ...) -> [return type]");
            }
        }
    }
    funcType->endI = lexer.lastToken()->endI;
    return funcType;
}

std::unique_ptr<ASTExpression> Parser::parse_expr() { return recur_expr(LOWEST_PRECEDENCE); }

std::unique_ptr<ASTExpression> Parser::parse_operand() {
    auto tkn = lexer.peekToken();
    switch (tkn->type) {
        case TokenType::MOD: {
            // parse_mod
            auto mod = make_node<ASTMod>(*tkn);
            lexer.nextToken();  // Consume mod

            if (check_token(TokenType::LEFT_CURLY)) {
                lexer.nextToken();  // Consume {
            } else {
                dx.err_after_token("Expected { after mod keyword", *lexer.lastToken())
                    ->fix("Add {")
                    ->note("Module must be of the form mod {...}");
            }

            while (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
                auto decl = assert_parse_decl();
                if (decl->nodeType == NodeType::UNKNOWN) {
                    dx.last_err()->note("Statements are never executed in the declaration of a module");
                } else {
                    mod->declarations.push_back(std::move(decl));
                }
            }
            if (check_token(TokenType::END)) {
                dx.err_after_token("Unterminated code at end of file. Expected closing }", *lexer.lastToken())
                    ->fix("Add } or another declaration");
            } else {
                lexer.nextToken();  // Consume }
            }
            mod->endI = lexer.lastToken()->endI;
            return mod;
        }
        case TokenType::TY: {
            // parse_ty
            auto ty = make_node<ASTTy>(*tkn);
            lexer.nextToken();  // Consume ty

            if (check_token(TokenType::LEFT_CURLY)) {
                lexer.nextToken();  // Consume {
            } else {
                dx.err_after_token("Expected { after ty keyword", *lexer.lastToken())
                    ->fix("Add {")
                    ->note("Type definition must be of the form ty {...}");
            }

            while (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
                auto decl = assert_parse_decl();
                if (decl->nodeType == NodeType::UNKNOWN) {
                    dx.last_err()->note("Statements are never executed in the declaration of a type");
                } else {
                    ty->declarations.push_back(std::move(decl));
                }
            }
            if (check_token(TokenType::END)) {
                dx.err_after_token("Unterminated code at end of file. Expected closing }", *lexer.lastToken())
                    ->fix("Add } or another declaration");
            } else {
                lexer.nextToken();  // Consume }
            }
            ty->endI = lexer.lastToken()->endI;
            return ty;
        }
        case TokenType::IDENTIFIER: {
            auto name = make_node<ASTName>(*tkn);
            name->ref = lexer.nextToken();  // Consume [identifier]
            return name;
        }
        case TokenType::VOID_TYPE:
        case TokenType::MOD_TYPE:
        case TokenType::TY_TYPE:
        case TokenType::INT_TYPE:
        case TokenType::DOUBLE_TYPE:
        case TokenType::STRING_TYPE:
        case TokenType::BOOL_TYPE: {
            auto typeLit = make_node<ASTTypeLit>(*tkn);
            typeLit->type = lexer.nextToken()->type;  // Consume [type token]
            return typeLit;
        }
        case TokenType::INT_LITERAL:
        case TokenType::DOUBLE_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::TRUE:
        case TokenType::FALSE: {
            auto lit = make_node<ASTLit>(*tkn);
            lit->value = lexer.nextToken();  // Consume [literal token]
            return lit;
        }
        case TokenType::COND_NOT:
        case TokenType::BIT_NOT:
        case TokenType::OP_SUBTR:
        case TokenType::OP_MULT: {
            auto unOp = make_node<ASTUnOp>(*tkn);
            unOp->op = lexer.nextToken();
            unOp->inner = recur_expr(HIGHEST_PRECEDENCE);
            unOp->endI = unOp->inner->endI;
            return unOp;
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
                                   *lexer.lastToken());
            } else if (check_token(TokenType::RIGHT_PARENS) && exprDepth == 0) {
                dx.err_token("Too many closing parenthesis", *lexer.peekToken())->fix("Delete )");
            } else {
                dx.err_token(
                    "Expected an expression but found '" + tokenTypeToStr(lexer.peekToken()->type) + "' instead",
                    *lexer.peekToken());
            }
            lexer.nextToken();  // Consume [unknown expr token]
            return unknownNode<ASTExpression>(tkn->beginI, tkn->endI);
    }
}

std::unique_ptr<ASTExpression> Parser::recur_expr(int prec) {
    std::unique_ptr<ASTExpression> expr = parse_operand();

    for (;;) {
        int peekedPrec = get_precedence(lexer.peekToken()->type);
        if (peekedPrec > prec) {
            if (check_token(TokenType::LEFT_PARENS)) {
                exprDepth++;
                // parse_call
                auto call = make_node<ASTCall>(*expr);
                call->callRef = std::move(expr);

                lexer.nextToken();  // Consume (
                while (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END)) {
                    call->arguments.push_back(parse_expr());

                    if (check_token(TokenType::COMMA)) {
                        lexer.nextToken();  // Consume ,
                        if (check_token(TokenType::RIGHT_PARENS)) {
                            dx.err_after_token(
                                "[" + printExpr(*call->callRef) + "]: Expected another expression after the comma",
                                *lexer.lastToken());
                        }
                    } else if (!check_token(TokenType::RIGHT_PARENS) && !check_token(TokenType::END)) {
                        dx.err_after_token(
                            "[" + printExpr(*call->callRef) + "]: Expected either , or ) in call argument list",
                            *lexer.lastToken());
                    }
                }
                if (check_token(TokenType::END)) {
                    dx.err_after_token("[" + printExpr(*call->callRef) +
                                           "]: Unterminated code at end of file. Expected another expression",
                                       *lexer.lastToken())
                        ->fix("Add ) or another expression");
                } else if (check_token(TokenType::RIGHT_PARENS)) {
                    lexer.nextToken();  // Consume )
                }
                call->endI = lexer.lastToken()->endI;
                expr = std::move(call);
                exprDepth--;
            } else if (check_token(TokenType::DEREF)) {
                auto deref = make_node<ASTDeref>(*lexer.nextToken());  // Consume .*
                deref->inner = std::move(expr);
                deref->endI = deref->inner->endI;
                expr = std::move(deref);
            } else if (check_token(TokenType::DOT)) {
                lexer.nextToken();  // Consume .
                if (check_token(TokenType::LEFT_CURLY)) {
                    // parse_type_init
                    auto typeInit = make_node<ASTTypeInit>(*expr);
                    lexer.nextToken();  // Consume {
                    typeInit->typeRef = std::move(expr);

                    while (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
                        auto declOrExpr = parse_decl_expr();
                        if (declOrExpr->nodeType == NodeType::DECL) {
                            auto assignmentDecl = castNodePtr<ASTDecl>(declOrExpr);
                            if (assignmentDecl->type != nullptr) {
                                dx.err_node("[" + printExpr(*typeInit->typeRef) +
                                                "]: Variable declaration is not allowed here",
                                            *assignmentDecl)
                                    ->fix("Delete the type, " + printExpr(*assignmentDecl->type));
                            }
                            typeInit->assignments.push_back(std::move(assignmentDecl));
                        } else if (declOrExpr->nodeType != NodeType::UNKNOWN) {
                            dx.err_node("[" + printExpr(*typeInit->typeRef) + "]: Expression is not allowed here",
                                        *declOrExpr);
                        }
                        if (check_token(TokenType::COMMA)) {
                            lexer.nextToken();  // Consume ,
                        } else if (!check_token(TokenType::RIGHT_CURLY) && !check_token(TokenType::END)) {
                            dx.err_after_token("[" + printExpr(*typeInit->typeRef) +
                                                   "]: Expected either , or } in type initialization list",
                                               *lexer.lastToken());
                        }
                    }
                    if (check_token(TokenType::END)) {
                        dx.err_after_token("[" + printExpr(*typeInit->typeRef) +
                                               "]: Unterminated code at end of file. Expected closing }",
                                           *lexer.lastToken())
                            ->fix("Add } or another assignment");
                    } else if (check_token(TokenType::RIGHT_CURLY)) {
                        lexer.nextToken();  // Consume }
                    }
                    typeInit->endI = lexer.lastToken()->endI;
                    expr = std::move(typeInit);
                } else {
                    // parse_dot_op
                    auto dotOp = make_node<ASTDotOp>(*expr);
                    dotOp->base = std::move(expr);
                    auto member = recur_expr(peekedPrec);
                    if (member->nodeType != NodeType::NAME) {
                        dx.err_node("Expected a member variable of " + printExpr(*dotOp->base) + " but found " +
                                        printExpr(*member) + " instead",
                                    *member);
                        dotOp->member = unknownNode<ASTName>(member->beginI, member->endI);
                    } else {
                        dotOp->member = castNodePtr<ASTName>(member);
                    }
                    dotOp->endI = dotOp->member->endI;
                    expr = std::move(dotOp);
                }
            } else {
                auto binOp = make_node<ASTBinOp>(*expr);
                binOp->left = std::move(expr);
                binOp->op = lexer.nextToken();
                binOp->right = recur_expr(peekedPrec);
                binOp->endI = binOp->right->endI;
                expr = std::move(binOp);
            }
        } else {
            return expr;
        }
    }
}