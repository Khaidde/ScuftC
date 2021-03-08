#include "parser.hpp"

int Parser::getPrecedence(TokenType type) {
    switch (type) {
        case COND_OR_TKN:
            return 1;
        case COND_XOR_TKN:
            return 2;
        case COND_AND_TKN:
            return 3;

        case BIT_OR_TKN:
            return 4;
        case BIT_XOR_TKN:
            return 5;
        case BIT_AND_TKN:
            return 6;

        case COND_EQUALS_TKN:
        case COND_NOT_EQUALS_TKN:
            return 7;
        case COND_GREATER_EQUAL_TKN:
        case COND_LESS_EQUAL_TKN:
        case COND_GREATER_TKN:
        case COND_LESS_TKN:
            return 8;

        case BIT_SHIFT_LEFT_TKN:
        case BIT_SHIFT_RIGHT_TKN:
            return 9;

        case OP_ADD_TKN:
        case OP_SUBTR_TKN:
            return 10;
        case OP_MULT_TKN:
        case OP_DIV_TKN:
        case OP_MOD_TKN:
            return 11;
        case OP_CARROT_TKN:
            return 12;

        case DEREF_TKN:
        case LEFT_PARENS_TKN:
        case DOT_TKN:
            return 13;

        default:
            return LOWEST_PRECEDENCE;
    }
}

void Parser::assertToken(TokenType type, const std::string& msg) {
    auto actualTkn = lexer.peekToken();
    if (actualTkn->type != type) {
        std::string expectedTknStr = tokenTypeToStr(type);
        if (msg.empty()) {
            DX.err().after("Expected " + expectedTknStr + " but found a different token instead", *lexer.lastToken());
        } else {
            DX.err().after("Expected " + expectedTknStr + " but found a different token instead. " + msg,
                           *lexer.lastToken());
        }
        std::string actualTknStr;
        switch (actualTkn->type) {
            case IDENTIFIER_TKN:
                actualTknStr = "'" + actualTkn->toStr() + "'";
                break;
            case STRING_LITERAL_TKN:
                actualTknStr = "String literal";
                break;
            case END_TKN:
                panic(DX);
            default:
                actualTknStr = std::move(actualTkn->toStr());
        }
        panic(DX.at(actualTknStr + " is not permitted here", *actualTkn));
    }
}

std::unique_ptr<ASTProgram> Parser::parseProgram() {
    auto prgm = std::make_unique<ASTProgram>();

    while (!checkToken(END_TKN)) {
        auto declOrExpr = parseDeclOrExpr();
        if (declOrExpr->nodeType != DECL) {
            panic(DX.err()
                      .at("Expression is not allowed in global scope", *declOrExpr)
                      .note("An expression in global scope is never executed"));
        }
        prgm->declarations.push_back(castNodePtr<ASTDecl>(declOrExpr));
    }
    if (!prgm->declarations.empty()) {
        prgm->endIndex = prgm->declarations.back()->endIndex;
    }
    return prgm;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    switch (lexer.peekToken()->type) {
        case END_TKN:
            panic(DX.err().after("Expected a statement but got nothing", *lexer.lastToken()));
        case LEFT_CURLY_TKN:
            return parseBlock();
        case IF_TKN:
            return parseIf();
        case WHILE_TKN:
            return parseWhile();
        case FOR_TKN:
            return parseFor();
        case BREAK_TKN: {
            auto brk = makeNode<ASTBreak>(lexer.nextToken()->index);  // Consume break
            brk->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
            if (!checkToken(RIGHT_CURLY_TKN)) {
                panic(DX.err()
                          .at("Statements following a break are unreachable", *lexer.peekToken())
                          .at("Break statement found here", *brk)
                          .fix("Delete the " + std::string(lexer.peekToken()->toStr()) +
                               " and any other following statements in the block"));
                return brk;
            }
            return brk;
        }
        case CONTINUE_TKN: {
            auto cont = makeNode<ASTCont>(lexer.nextToken()->index);  // Consume continue
            cont->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
            if (!checkToken(RIGHT_CURLY_TKN)) {
                panic(DX.err()
                          .at("Statements following a continue are unreachable", *lexer.peekToken())
                          .at("Continue statement found here", *cont)
                          .fix("Delete the " + lexer.peekToken()->toStr() +
                               " and any other following statements in the block"));
            }
            return cont;
        }
        case RETURN_TKN: {
            auto ret = makeNode<ASTRet>(lexer.nextToken()->index);  // Consume return
            if (!checkToken(RIGHT_CURLY_TKN)) {
                ret->retValue = parseExpression();
                ret->endIndex = ret->retValue->endIndex;
            } else {
                ret->retValue = nullptr;
                ret->endIndex = ret->locIndex + lexer.lastToken()->cLen;
            }
            if (!checkToken(RIGHT_CURLY_TKN)) {
                panic(DX.err()
                          .at("Statements following a return are unreachable", *lexer.peekToken())
                          .at("Return statement found here", *ret)
                          .fix("Delete the " + lexer.peekToken()->toStr() +
                               " and any other following statements in the block"));
            }
            return ret;
        }
        default:
            return parseDeclOrExpr();
    }
}

std::unique_ptr<ASTBlock> Parser::parseBlock() {
    ASSERT(lexer.peekToken()->type == LEFT_CURLY_TKN, "Block must start with {");
    auto block = makeNode<ASTBlock>(lexer.nextToken()->index);  // Consume {

    while (!checkToken(RIGHT_CURLY_TKN) && !checkToken(END_TKN)) {
        block->statements.push_back(parseStatement());
    }
    if (checkToken(END_TKN)) {
        panic(DX.err()
                  .at("Mismatched curly brackets. Start of block found here", block->locIndex)
                  .after("Reached end of file before finding a closing }", *lexer.lastToken()));
    } else {
        auto rightCurly = lexer.nextToken();  // Consume }
        block->endIndex = rightCurly->index + rightCurly->cLen;
    }

    return block;
}

std::unique_ptr<ASTIf> Parser::parseIf() {
    ASSERT(lexer.peekToken()->type == IF_TKN, "If statement must start with an if token");
    auto ifStmt = makeNode<ASTIf>(lexer.nextToken()->index);  // Consume if
    ifStmt->condition = parseExpression();

    if (checkToken(RIGHT_PARENS_TKN)) {
        panic(DX.err().at("Unbalanced parenthesis. Expected {", *lexer.peekToken()));
    }

    ifStmt->conseq = checkToken(LEFT_CURLY_TKN) ? parseBlock() : parseStatement();
    if (checkToken(ELSE_TKN)) {
        lexer.nextToken();  // Consume else
        if (checkToken(END_TKN)) {
            panic(DX.err()
                      .at("Error in if statement", *ifStmt)
                      .after("Unterminated code at end of file. Expected if keyword or {", *lexer.lastToken()));
        } else if (checkToken(IF_TKN)) {
            ifStmt->alt = parseIf();
        } else {
            ifStmt->alt = checkToken(LEFT_CURLY_TKN) ? parseBlock() : parseStatement();
        }
        ifStmt->endIndex = ifStmt->alt->endIndex;
    } else {
        ifStmt->endIndex = ifStmt->conseq->endIndex;
    }

    return ifStmt;
}

std::unique_ptr<ASTWhile> Parser::parseWhile() {
    ASSERT(lexer.peekToken()->type == WHILE_TKN, "While statement must start with an while token");
    auto whileLoop = makeNode<ASTWhile>(lexer.nextToken()->index);  // Consume while
    whileLoop->condition = parseExpression();
    whileLoop->blockStmt = checkToken(LEFT_CURLY_TKN) ? parseBlock() : parseStatement();
    return whileLoop;
}

std::unique_ptr<ASTFor> Parser::parseFor() {
    ASSERT(lexer.peekToken()->type == FOR_TKN, "For statement must start with an for token");
    auto forLoop = makeNode<ASTFor>(lexer.nextToken()->index);  // Consume for

    if (checkToken(LEFT_CURLY_TKN)) {
        forLoop->blockStmt = parseBlock();
    } else {
        ASSERT(false, "Unimplemented for loop parsing for for loops not of the form for {...}");
    }

    return forLoop;
}

std::unique_ptr<ASTNode> Parser::parseDeclOrExpr() {
    auto decl = makeNode<ASTDecl>(lexer.peekToken()->index);

    decl->lvalue = parseExpression();
    decl->locIndex = decl->lvalue->locIndex;

    if (checkToken(COLON_TKN)) {
        lexer.nextToken();  // Consume :
        decl->type = parseExpression();
    }

    if (checkToken(ASSIGNMENT_TKN) || checkToken(CONST_ASSIGNMENT_TKN)) {
        decl->assignType = lexer.nextToken();
        decl->rvalue = parseExpression();
        decl->endIndex = decl->rvalue->endIndex;
    } else if (decl->type == nullptr) {
        // if no type or assignment is detected, then the statement is a free floating expression
        return castNodePtr<ASTNode>(decl->lvalue);
    } else {
        decl->assignType = nullptr;
        decl->endIndex = decl->type->endIndex;
    }

    // TODO test and remove later
    if (decl->lvalue->nodeType == NAME) {
        auto name = static_cast<ASTName&>(*decl->lvalue);
        auto entry = globalScopeTest.find(name.ref);

        if (entry == nullptr) {
            globalScopeTest.insert(name.ref, decl.get());
        } else {
            panic(DX.err().at("GREAT! " + name.ref->toStr(), *decl->lvalue).at("Other", *entry->decl));
        }
    }

    return decl;
}

std::unique_ptr<ASTMod> Parser::parseModule() {
    ASSERT(lexer.peekToken()->type == MOD_TKN, "Module must start with a mod token");
    auto module = makeNode<ASTMod>(lexer.nextToken()->index);  // Consume mod

    assertToken(LEFT_CURLY_TKN, "Module must be of the form mod {...}");
    lexer.nextToken();  // Consume {

    while (!checkToken(RIGHT_CURLY_TKN) && !checkToken(END_TKN)) {
        auto declOrExpr = parseDeclOrExpr();
        if (declOrExpr->nodeType != DECL) {
            panic(DX.err()
                      .at("Error in module", *module)
                      .at("Expression is not allowed here", *declOrExpr)
                      .note("A free-floating expression in a module is never executed"));
        }
        module->declarations.push_back(castNodePtr<ASTDecl>(declOrExpr));
    }
    if (checkToken(END_TKN)) {
        panic(DX.err()
                  .at("Error in module", *module)
                  .after("Unterminated code at end of file. Expected closing }", *lexer.lastToken()));
    } else {
        module->endIndex = lexer.nextToken()->index + 1;  // Consume }
    }
    return module;
}

std::unique_ptr<ASTTy> Parser::parseTy() {
    ASSERT(lexer.peekToken()->type == TY_TKN, "Type definition must start with a ty token");
    auto ty = makeNode<ASTTy>(lexer.nextToken()->index);  // Consume ty

    assertToken(LEFT_CURLY_TKN, "Type definition must be of the form ty {...}");
    lexer.nextToken();  // Consume {

    while (!checkToken(RIGHT_CURLY_TKN) && !checkToken(END_TKN)) {
        auto declOrExpr = parseDeclOrExpr();
        if (declOrExpr->nodeType != DECL) {
            panic(DX.err()
                      .at("Error in type definition", *ty)
                      .at("Expression is not allowed here", *declOrExpr)
                      .note("A free-floating expression in a type definition is never executed"));
        }
        ty->declarations.push_back(castNodePtr<ASTDecl>(declOrExpr));
    }
    if (checkToken(END_TKN)) {
        panic(DX.err()
                  .at("Error in type definition", *ty)
                  .after("Unterminated code at end of file. Expected closing }", *lexer.lastToken()));
    } else {
        ty->endIndex = lexer.nextToken()->index + 1;  // Consume }
    }
    return ty;
}

std::unique_ptr<ASTExpression> Parser::parseLeftParenExpression(int depth) {
    ASSERT(lexer.peekToken()->type == LEFT_PARENS_TKN, "Left parenthesis expression must start with (");
    auto func = makeNode<ASTFunc>(lexer.nextToken()->index);  // Consume (

    bool isFunction = false;
    while (!checkToken(RIGHT_PARENS_TKN) && !checkToken(END_TKN)) {
        auto paramRef = parseExpression();

        if (paramRef->nodeType != NAME) {
            if (isFunction) {
                panic(DX.err()
                          .after("Expected a name for the function parameter", *lexer.lastToken())
                          .at(lexer.peekToken()->toStr() + " is not allowed here", *lexer.peekToken()));
            } else {
                return parseFunctionType(paramRef, depth);
            }
        }

        if (!checkToken(COLON_TKN)) {
            if (isFunction) {
                panic(
                    DX.err()
                        .after(
                            "Expected a colon. Function parameter must declare a type in the form [param name]: [type]",
                            *lexer.lastToken())
                        .at(lexer.peekToken()->toStr() + " is not allowed here", *lexer.peekToken()));
            } else {
                return parseFunctionType(paramRef, depth);
            }
        }
        lexer.nextToken();  // Consume :

        auto funcParam = std::make_unique<FuncParam>();
        funcParam->paramRef = castNodePtr<ASTName>(paramRef);
        funcParam->type = parseExpression();
        func->parameters.push_back(std::move(funcParam));

        if (checkToken(COMMA_TKN)) {
            lexer.nextToken();  // Consume ,
            if (checkToken(RIGHT_PARENS_TKN) || checkToken(END_TKN)) {
                panic(DX.err().after("Expected another function parameter after the comma but go nothing",
                                     *lexer.lastToken()));
            }
        } else if (!checkToken(RIGHT_PARENS_TKN)) {
            panic(DX.err().after("Expected either , or ) in function parameter list", *lexer.lastToken()));
        }
        isFunction = true;
    }
    if (checkToken(END_TKN)) {
        panic(DX.err()
                  .at("Error in function", *func)
                  .after("Unterminated code at end of file. Expected another function parameter", *lexer.lastToken()));
    } else {
        lexer.nextToken();  // Consume )
    }

    if (checkToken(ARROW_TKN)) {
        lexer.nextToken();  // Consume ->
        func->returnType = parseExpression();
    } else {
        func->returnType = nullptr;
    }

    if (checkToken(SINGLE_RETURN_TKN)) {
        lexer.nextToken();  // Consume ::
        func->returnType = nullptr;
        func->blockOrExpr = parseExpression();
    } else {
        if (checkToken(END_TKN) && func->returnType == nullptr) {
            panic(DX.err().after("Expected a return type or function block but got nothing", *lexer.lastToken()));
        }
        if (!checkToken(LEFT_CURLY_TKN) && func->returnType != nullptr) {
            auto funcType = makeNode<ASTFuncType>(func->locIndex);
            funcType->outType = std::move(func->returnType);
            return funcType;
        }
        assertToken(LEFT_CURLY_TKN, "Function body must be surrounded in curly brackets {}");
        func->blockOrExpr = parseBlock();
    }
    func->endIndex = func->blockOrExpr->endIndex;

    return func;
}

std::unique_ptr<ASTExpression> Parser::parseFunctionType(std::unique_ptr<ASTExpression>& reduceExpr, int depth) {
    auto funcType = makeNode<ASTFuncType>(reduceExpr->locIndex);

    do {
        if (funcType->inTypes.empty()) {
            funcType->inTypes.push_back(std::move(reduceExpr));
        } else {
            funcType->inTypes.push_back(parseExpression());
        }

        if (checkToken(COMMA_TKN)) {
            lexer.nextToken();  // Consume ,
            if (checkToken(RIGHT_PARENS_TKN) || checkToken(END_TKN)) {
                panic(DX.err().after("Expected another type after the comma but go nothing", *lexer.lastToken()));
            }
        } else if (!checkToken(RIGHT_PARENS_TKN)) {
            DX.err();
            if (funcType->inTypes.size() == 1) {
                DX.after("Unbalanced parenthesis. Expected " + std::to_string(depth) + " more )", *lexer.lastToken());
            }
            panic(DX.note("Expected either , or )"));
        }
    } while (!checkToken(RIGHT_PARENS_TKN) && !checkToken(END_TKN));

    if (checkToken(END_TKN)) {
        panic(DX.err()
                  .at("Error in function type", *funcType)
                  .after("Unterminated code at end of file. Expected another type", *lexer.lastToken()));
    } else {
        lexer.nextToken();  // Consume )
    }

    if (!checkToken(ARROW_TKN)) {
        if (funcType->inTypes.size() != 1) {
            panic(DX.err()
                      .after("Function type must explicitly declare a return type", *lexer.lastToken())
                      .fix("Use the format ([input type 1], [input type 2], ...) -> [return type]"));
        } else {
            return std::move(funcType->inTypes.front());
        }
    } else {
        lexer.nextToken();  // Consume ->
    }
    funcType->outType = parseExpression();
    return funcType;
}

std::unique_ptr<ASTExpression> Parser::parseExpression() { return recurExpression(LOWEST_PRECEDENCE, 0); }

std::unique_ptr<ASTExpression> Parser::parseOperand(int depth) {
    auto tkn = lexer.peekToken();
    switch (tkn->type) {
        case MOD_TKN:
            return parseModule();
        case TY_TKN:
            return parseTy();
        case IDENTIFIER_TKN: {
            auto name = makeNode<ASTName>(tkn->index);
            name->ref = lexer.nextToken();
            name->endIndex = tkn->index + tkn->cLen;
            return name;
        }
        case VOID_TYPE_TKN:
        case MOD_TYPE_TKN:
        case TY_TYPE_TKN:
        case INT_TYPE_TKN:
        case DOUBLE_TYPE_TKN:
        case STRING_TYPE_TKN:
        case BOOL_TYPE_TKN: {
            auto typeLit = makeNode<ASTTypeLit>(tkn->index);
            typeLit->type = lexer.nextToken()->type;
            typeLit->endIndex = tkn->index + tkn->cLen;
            return typeLit;
        }
        case INT_LITERAL_TKN:
        case DOUBLE_LITERAL_TKN:
        case STRING_LITERAL_TKN:
        case TRUE_TKN:
        case FALSE_TKN: {
            auto lit = makeNode<ASTLit>(tkn->index);
            lit->value = lexer.nextToken();
            lit->endIndex = tkn->index + tkn->cLen;
            return lit;
        }
        case COND_NOT_TKN:
        case BIT_NOT_TKN:
        case OP_SUBTR_TKN:
        case OP_MULT_TKN: {
            auto unOp = makeNode<ASTUnOp>(tkn->index);
            unOp->op = lexer.nextToken();
            unOp->inner = recurExpression(HIGHEST_PRECEDENCE, depth);
            unOp->endIndex = unOp->inner->endIndex;
            return unOp;
        }
        case LEFT_PARENS_TKN: {
            return parseLeftParenExpression(depth + 1);
        }
        default:
            if (checkToken(END_TKN)) {
                panic(DX.err().after("Expected an expression but instead reached the end of the file",
                                     *lexer.lastToken()));
            } else {
                // TODO figure out a way to create more descriptive error messages
                panic(DX.err()
                          .after("Expected an expression", *lexer.lastToken())
                          .at("Instead got " + tokenTypeToStr(lexer.peekToken()->type), *lexer.peekToken()));
            }
    }
}

std::unique_ptr<ASTExpression> Parser::recurExpression(int prec, int depth) {
    std::unique_ptr<ASTExpression> expr = parseOperand(depth);

    for (;;) {
        int peekedPrec = getPrecedence(lexer.peekToken()->type);
        if (peekedPrec > prec) {
            if (checkToken(LEFT_PARENS_TKN)) {
                // parseCall
                auto call = makeNode<ASTCall>(expr->locIndex);
                call->callRef = std::move(expr);
                call->arguments = parseExprList();

                call->endIndex = lexer.lastToken()->index + 1;
                expr = std::move(call);
            } else if (checkToken(DEREF_TKN)) {
                auto deref = makeNode<ASTDeref>(lexer.nextToken()->index);  // Consume .*
                deref->inner = std::move(expr);
                deref->endIndex = deref->inner->endIndex;
                expr = std::move(deref);
            } else if (checkToken(DOT_TKN)) {
                lexer.nextToken();  // Consume .
                if (checkToken(LEFT_CURLY_TKN)) {
                    // parseTypeInit
                    auto typeInit = makeNode<ASTTypeInit>(expr->locIndex);
                    lexer.nextToken();  // Consume {
                    typeInit->typeRef = std::move(expr);

                    while (!checkToken(RIGHT_CURLY_TKN) && !checkToken(END_TKN)) {
                        auto declOrExpr = parseDeclOrExpr();
                        if (declOrExpr->nodeType != DECL) {
                            panic(DX.err()
                                      .at("Error in type initializer", *typeInit)
                                      .at("Expressions are not allowed here", *declOrExpr));
                        }
                        auto assignmentDecl = castNodePtr<ASTDecl>(declOrExpr);
                        if (assignmentDecl->type != nullptr) {
                            panic(DX.err()
                                      .at("Error in type initializer", *typeInit)
                                      .at("Variable declaration is not allowed here", *assignmentDecl));
                        }
                        typeInit->assignments.push_back(std::move(assignmentDecl));

                        if (checkToken(COMMA_TKN)) {
                            lexer.nextToken();  // Consume ,
                        } else if (!checkToken(RIGHT_CURLY_TKN)) {
                            panic(DX.err()
                                      .at("Error in type initializer", *typeInit)
                                      .after("Expected either , or }", *lexer.lastToken()));
                        }
                    }
                    if (checkToken(END_TKN)) {
                        panic(DX.err()
                                  .at("Error in type initializer", *typeInit)
                                  .after("Unterminated code at end of file. Expected closing }", *lexer.lastToken()));
                    } else {
                        typeInit->endIndex = lexer.nextToken()->index + 1;  // Consume }
                    }
                    expr = std::move(typeInit);
                } else {
                    auto dotOp = makeNode<ASTDotOp>(expr->locIndex);
                    dotOp->base = std::move(expr);
                    auto member = recurExpression(peekedPrec, depth);
                    if (member->nodeType != NAME) {
                        panic(DX.err().at("Expected a member variable of " + printExpr(*dotOp->base) + " but found " +
                                              printExpr(*member) + " instead",
                                          *member));
                    }
                    dotOp->member = castNodePtr<ASTName>(member);
                    dotOp->endIndex = dotOp->member->endIndex;
                    expr = std::move(dotOp);
                }
            } else {
                auto binOp = makeNode<ASTBinOp>(expr->locIndex);
                binOp->left = std::move(expr);
                binOp->op = lexer.nextToken();
                binOp->right = recurExpression(peekedPrec, depth);
                binOp->endIndex = binOp->right->endIndex;
                expr = std::move(binOp);
            }
        } else {
            return expr;
        }
    }
}

std::vector<std::unique_ptr<ASTExpression>> Parser::parseExprList() {
    std::vector<std::unique_ptr<ASTExpression>> exprVec;

    assertToken(LEFT_PARENS_TKN);
    lexer.nextToken();  // Consume (
    while (!(checkToken(RIGHT_PARENS_TKN) || checkToken(END_TKN))) {
        exprVec.push_back(parseExpression());
        if (checkToken(COMMA_TKN)) {
            lexer.nextToken();  // Consume ,
            if (checkToken(RIGHT_PARENS_TKN) || checkToken(END_TKN)) {
                panic(DX.err().after("Expected another expression after the comma but go nothing", *lexer.lastToken()));
            }
        } else if (!checkToken(RIGHT_PARENS_TKN)) {
            panic(DX.err().after("Expected either , or )", *lexer.lastToken()));
        }
    }
    if (checkToken(END_TKN)) {
        panic(DX.err().after("Unterminated code at end of file. Expected another expression", *lexer.lastToken()));
    } else {
        lexer.nextToken();  // Consume )
    }
    return exprVec;
}