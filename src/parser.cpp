#include "parser.hpp"

int Parser::getPrecedence(TokenType type) {
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

bool Parser::assertToken(TokenType type, const std::string& msg) {
    auto actualTkn = lexer.peekToken();
    if (actualTkn->type != type) {
        std::string expectedTknStr = tokenTypeToStr(type);
        DX.err().after("Expected " + expectedTknStr + " but found a different token instead", *lexer.lastToken());
        if (!msg.empty()) {
            DX.note(msg);
        }
        std::string actualTknStr;
        switch (actualTkn->type) {
            case TokenType::IDENTIFIER:
                actualTknStr = "\"" + actualTkn->toStr() + "\"";
                break;
            case TokenType::STRING_LITERAL:
                actualTknStr = "String literal";
                break;
            case TokenType::END:
                break;
            default:
                actualTknStr = std::move(actualTkn->toStr());
        }
        DX.at(actualTknStr + " is not allowed here", *actualTkn);
        return false;
    }
    return true;
}

std::unique_ptr<ASTProgram> Parser::parseProgram() {
    auto prgm = std::make_unique<ASTProgram>();

    while (!checkToken(TokenType::END)) {
        auto decl = assertParseDeclaration();
        if (decl->nodeType == NodeType::UNKNOWN) {
            DX.note("Statements are never executed in global scope");
        } else {
            prgm->declarations.push_back(std::move(decl));
        }
        if (DX.hasErrors()) return prgm;
    }
    if (!prgm->declarations.empty()) {
        prgm->endIndex = prgm->declarations.back()->endIndex;
    }
    return prgm;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    switch (lexer.peekToken()->type) {
        case TokenType::END:
            panic(DX.err().after("Expected a statement but got nothing", *lexer.lastToken()));
        case TokenType::LEFT_CURLY:
            return parseBlock();
        case TokenType::IF:
            return parseIf();
        case TokenType::WHILE:
            return parseWhile();
        case TokenType::FOR:
            return parseFor();
        case TokenType::BREAK: {
            auto brk = makeNode<ASTBreak>(lexer.peekToken()->index, lexer.peekToken()->cLen);
            lexer.nextToken();  // Consume break
            if (!checkToken(TokenType::RIGHT_CURLY)) {
                panic(DX.err()
                          .at("Statements following a break are unreachable", *lexer.peekToken())
                          .at("Break statement found here", *brk)
                          .fix("Delete the " + std::string(lexer.peekToken()->toStr()) +
                               " and any other following statements in the block"));
                return brk;
            }
            return brk;
        }
        case TokenType::CONTINUE: {
            auto cont = makeNode<ASTCont>(lexer.peekToken()->index, lexer.peekToken()->cLen);
            lexer.nextToken();  // Consume continue
            if (!checkToken(TokenType::RIGHT_CURLY)) {
                panic(DX.err()
                          .at("Statements following a continue are unreachable", *lexer.peekToken())
                          .at("Continue statement found here", *cont)
                          .fix("Delete the " + lexer.peekToken()->toStr() +
                               " and any other following statements in the block"));
            }
            return cont;
        }
        case TokenType::RETURN: {
            auto ret = makeNode<ASTRet>(lexer.nextToken()->index);  // Consume return
            if (!checkToken(TokenType::RIGHT_CURLY)) {
                ret->retValue = parseExpression();
                ret->endIndex = ret->retValue->endIndex;
            } else {
                ret->retValue = nullptr;
                ret->endIndex = ret->locIndex + lexer.lastToken()->cLen;
            }
            if (!checkToken(TokenType::RIGHT_CURLY)) {
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
    ASSERT(lexer.peekToken()->type == TokenType::LEFT_CURLY, "Block must start with {");
    auto block = makeNode<ASTBlock>(lexer.nextToken()->index);  // Consume {

    while (!checkToken(TokenType::RIGHT_CURLY) && !checkToken(TokenType::END)) {
        block->statements.push_back(parseStatement());
    }
    if (checkToken(TokenType::END)) {
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
    ASSERT(lexer.peekToken()->type == TokenType::IF, "If statement must start with an if token");
    auto ifStmt = makeNode<ASTIf>(lexer.nextToken()->index);  // Consume if
    ifStmt->condition = parseExpression();

    if (checkToken(TokenType::RIGHT_CURLY)) {
        panic(DX.err().at("Unbalanced parenthesis. Expected {", *lexer.peekToken()).fix("Replace } with {"));
    }

    ifStmt->conseq = checkToken(TokenType::LEFT_CURLY) ? parseBlock() : parseStatement();
    if (checkToken(TokenType::ELSE)) {
        lexer.nextToken();  // Consume else
        if (checkToken(TokenType::END)) {
            panic(DX.err()
                      .at("Error in if statement", *ifStmt)
                      .after("Unterminated code at end of file. Expected if keyword or {", *lexer.lastToken()));
        } else if (checkToken(TokenType::IF)) {
            ifStmt->alt = parseIf();
        } else {
            ifStmt->alt = checkToken(TokenType::LEFT_CURLY) ? parseBlock() : parseStatement();
        }
        ifStmt->endIndex = ifStmt->alt->endIndex;
    } else {
        ifStmt->endIndex = ifStmt->conseq->endIndex;
    }

    return ifStmt;
}

std::unique_ptr<ASTWhile> Parser::parseWhile() {
    ASSERT(lexer.peekToken()->type == TokenType::WHILE, "While statement must start with an while token");
    auto whileLoop = makeNode<ASTWhile>(lexer.nextToken()->index);  // Consume while
    whileLoop->condition = parseExpression();
    whileLoop->blockStmt = checkToken(TokenType::LEFT_CURLY) ? parseBlock() : parseStatement();
    return whileLoop;
}

std::unique_ptr<ASTFor> Parser::parseFor() {
    ASSERT(lexer.peekToken()->type == TokenType::FOR, "For statement must start with an for token");
    auto forLoop = makeNode<ASTFor>(lexer.nextToken()->index);  // Consume for

    if (checkToken(TokenType::LEFT_CURLY)) {
        forLoop->blockStmt = parseBlock();
    } else {
        ASSERT(false, "Unimplemented for loop parsing for for loops not of the form for {...}");
    }

    return forLoop;
}

std::unique_ptr<ASTDecl> Parser::assertParseDeclaration() {
    switch (lexer.peekToken()->type) {
        case TokenType::IF: {
            auto ifStmt = parseIf();
            DX.err().at("If statement is not allowed here", *ifStmt);  // Consume if
            break;
        }
        case TokenType::WHILE:
            DX.err().at("While loop is not allowed here", *lexer.nextToken());  // Consume while
            break;
        case TokenType::FOR:
            DX.err().at("For loop is not allowed here", *lexer.nextToken());  // Consume for
            break;
            // TODO add error checking for break, continue and return statements
        default:
            auto declOrExpr = parseDeclOrExpr();
            if (declOrExpr->nodeType != NodeType::DECL) {
                if (declOrExpr->nodeType != NodeType::UNKNOWN) {
                    DX.err().at(nodeTypeToStr(declOrExpr->nodeType) + " is not allowed here", *declOrExpr);
                }
            } else {
                return castNodePtr<ASTDecl>(declOrExpr);
            }
    }
    return unknownNode<ASTDecl>();
}

std::unique_ptr<ASTNode> Parser::parseDeclOrExpr() {
    auto decl = makeNode<ASTDecl>(lexer.peekToken()->index);

    decl->lvalue = parseExpression();
    decl->locIndex = decl->lvalue->locIndex;

    if (checkToken(TokenType::COLON)) {
        lexer.nextToken();  // Consume :
        decl->type = parseExpression();
    }

    if (checkToken(TokenType::ASSIGNMENT) || checkToken(TokenType::CONST_ASSIGNMENT)) {
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

    if (decl->lvalue->nodeType == NodeType::NAME) {
        auto name = static_cast<ASTName&>(*decl->lvalue);
        if (!globalTable.find(name.ref)) {
            globalTable.insert(name.ref, decl.get());
        }
    }

    return decl;
}

std::unique_ptr<ASTExpression> Parser::parseLeftParenExpression(int depth) {
    ASSERT(lexer.peekToken()->type == TokenType::LEFT_PARENS, "Left parenthesis expression must start with (");
    auto func = makeNode<ASTFunc>(lexer.nextToken()->index);  // Consume (

    bool isFunction = false;
    while (!checkToken(TokenType::RIGHT_PARENS) && !checkToken(TokenType::END)) {
        auto funcParam = parseDeclOrExpr();
        if (funcParam->nodeType == NodeType::DECL) {
            func->parameters.push_back(castNodePtr<ASTDecl>(funcParam));
        } else {
            if (funcParam->nodeType == NodeType::UNKNOWN) {
                DX.note("Function parameter must be a variable declaration");
                break;
            }
            if (isFunction) {
                DX.err()
                    .at("Error in function", *func)
                    .at("Expression is not allowed here", *funcParam)
                    .note("Function parameter must declare a variable");
            } else {
                return parseFunctionType(castNodePtr<ASTExpression>(funcParam), depth);
            }
        }
        if (checkToken(TokenType::COMMA)) {
            lexer.nextToken();  // Consume ,
            if (checkToken(TokenType::RIGHT_PARENS)) {
                DX.err().after("Expected another function parameter after the comma", *lexer.lastToken());
                break;
            }
        } else if (!checkToken(TokenType::RIGHT_PARENS) && !checkToken(TokenType::END)) {
            DX.err().after("Expected either , or ) in function parameter list", *lexer.lastToken());
            break;
        }
        isFunction = true;
    }
    if (checkToken(TokenType::END)) {
        DX.err()
            .at("Error in function", *func)
            .after("Unterminated code at end of file. Expected another function parameter", *lexer.lastToken());
    } else {
        lexer.nextToken();  // Consume )
    }
    if (DX.hasErrors()) {
        func->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
        return func;
    }

    if (checkToken(TokenType::ARROW)) {
        lexer.nextToken();  // Consume ->
        func->returnType = parseExpression();
    } else {
        func->returnType = nullptr;
    }

    if (checkToken(TokenType::SINGLE_RETURN)) {
        lexer.nextToken();  // Consume ::
        func->returnType = nullptr;
        func->blockOrExpr = parseExpression();
        if (func->blockOrExpr->nodeType == NodeType::UNKNOWN) {
            DX.note("Function shorthand must be in the form (...) :: [return expression]");
            func->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
        }
    } else {
        if (checkToken(TokenType::END) && func->returnType == nullptr) {
            DX.err().after("Expected a return type or function block", *lexer.lastToken());
            func->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
        } else {
            if (!checkToken(TokenType::LEFT_CURLY) && func->returnType != nullptr) {
                auto funcType = makeNode<ASTFuncType>(func->locIndex, func->returnType->endIndex);
                funcType->outType = std::move(func->returnType);
            } else if (assertToken(TokenType::LEFT_CURLY, "Function body must be start with { and end with }")) {
                func->blockOrExpr = parseBlock();
                func->endIndex = func->blockOrExpr->endIndex;
            }
        }
    }
    return func;
}

std::unique_ptr<ASTExpression> Parser::parseFunctionType(std::unique_ptr<ASTExpression> reduceExpr, int depth) {
    auto funcType = makeNode<ASTFuncType>(reduceExpr->locIndex);

    do {
        if (funcType->inTypes.empty()) {
            funcType->inTypes.push_back(std::move(reduceExpr));
        } else {
            funcType->inTypes.push_back(parseExpression());
        }

        if (checkToken(TokenType::COMMA)) {
            lexer.nextToken();  // Consume ,
            if (checkToken(TokenType::RIGHT_PARENS) || checkToken(TokenType::END)) {
                panic(DX.err().after("Expected another type after the comma", *lexer.lastToken()));
            }
        } else if (!checkToken(TokenType::RIGHT_PARENS)) {
            DX.err();
            if (funcType->inTypes.size() == 1) {
                DX.after("Unbalanced parenthesis. Expected " + std::to_string(depth) + " more )", *lexer.lastToken());
            }
            panic(DX.note("Expected either , or )"));
        }
    } while (!checkToken(TokenType::RIGHT_PARENS) && !checkToken(TokenType::END));

    if (checkToken(TokenType::END)) {
        panic(DX.err()
                  .at("Error in function type", *funcType)
                  .after("Unterminated code at end of file. Expected another type", *lexer.lastToken()));
    } else {
        lexer.nextToken();  // Consume )
    }

    if (!checkToken(TokenType::ARROW)) {
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
        case TokenType::MOD: {
            // parseMod
            auto mod = makeNode<ASTMod>(tkn->index, tkn->cLen);
            lexer.nextToken();  // Consume mod

            if (!assertToken(TokenType::LEFT_CURLY, "Module must be of the form mod {...}")) {
                return unknownExpr();
            }
            lexer.nextToken();  // Consume {

            while (!checkToken(TokenType::RIGHT_CURLY) && !checkToken(TokenType::END)) {
                auto decl = assertParseDeclaration();
                if (decl->nodeType == NodeType::UNKNOWN) {
                    DX.note("Statements are never executed in the declaration of a module");
                } else {
                    mod->declarations.push_back(std::move(decl));
                }
            }
            if (checkToken(TokenType::END)) {
                DX.err()
                    .at("Error in module", *mod)
                    .after("Unterminated code at end of file. Expected closing }", *lexer.lastToken());
                mod->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
            } else {
                mod->endIndex = lexer.nextToken()->index + 1;  // Consume }
            }
            return mod;
        }
        case TokenType::TY: {
            // parseTy
            auto ty = makeNode<ASTTy>(tkn->index, tkn->cLen);
            lexer.nextToken();  // Consume ty

            if (!assertToken(TokenType::LEFT_CURLY, "Type definition must be of the form ty {...}")) {
                return unknownExpr();
            }
            lexer.nextToken();  // Consume {

            while (!checkToken(TokenType::RIGHT_CURLY) && !checkToken(TokenType::END)) {
                auto declOrExpr = parseDeclOrExpr();
                if (declOrExpr->nodeType == NodeType::DECL) {
                    ty->declarations.push_back(castNodePtr<ASTDecl>(declOrExpr));
                } else {
                    if (declOrExpr->nodeType != NodeType::UNKNOWN) {
                        DX.err()
                            .at("Error in type definition", *ty)
                            .at("Expression is not allowed here", *declOrExpr)
                            .note("A free-floating expression in a type definition is never executed");
                    }
                }
            }
            if (checkToken(TokenType::END)) {
                DX.err()
                    .at("Error in type definition", *ty)
                    .after("Unterminated code at end of file. Expected closing }", *lexer.lastToken());
                ty->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
            } else {
                ty->endIndex = lexer.nextToken()->index + 1;  // Consume }
            }
            return ty;
        }
        case TokenType::IDENTIFIER: {
            auto name = makeNode<ASTName>(tkn->index, tkn->cLen);
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
            auto typeLit = makeNode<ASTTypeLit>(tkn->index, tkn->cLen);
            typeLit->type = lexer.nextToken()->type;  // Consume [type token]
            return typeLit;
        }
        case TokenType::INT_LITERAL:
        case TokenType::DOUBLE_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::TRUE:
        case TokenType::FALSE: {
            auto lit = makeNode<ASTLit>(tkn->index, tkn->cLen);
            lit->value = lexer.nextToken();  // Consume [literal token]
            return lit;
        }
        case TokenType::COND_NOT:
        case TokenType::BIT_NOT:
        case TokenType::OP_SUBTR:
        case TokenType::OP_MULT: {
            auto unOp = makeNode<ASTUnOp>(tkn->index);
            unOp->op = lexer.nextToken();
            unOp->inner = recurExpression(HIGHEST_PRECEDENCE, depth);
            unOp->endIndex = unOp->inner->endIndex;
            return unOp;
        }
        case TokenType::LEFT_PARENS: {
            return parseLeftParenExpression(depth + 1);
        }
        default:
            if (checkToken(TokenType::END)) {
                DX.err().after("Expected an expression but instead reached the end of the file", *lexer.lastToken());
            } else {
                // TODO figure out a way to create more descriptive error messages
                DX.err().at(
                    "Expected an expression but found '" + tokenTypeToStr(lexer.peekToken()->type) + "' instead",
                    *lexer.peekToken());
                lexer.nextToken();  // Consume [unknown expression]
            }
            return unknownExpr();
    }
}

std::unique_ptr<ASTExpression> Parser::recurExpression(int prec, int depth) {
    std::unique_ptr<ASTExpression> expr = parseOperand(depth);

    for (;;) {
        int peekedPrec = getPrecedence(lexer.peekToken()->type);
        if (peekedPrec > prec) {
            if (checkToken(TokenType::LEFT_PARENS)) {
                // parseCall
                auto call = makeNode<ASTCall>(expr->locIndex, expr->endIndex - expr->locIndex);
                call->callRef = std::move(expr);

                lexer.nextToken();  // Consume (
                while (!checkToken(TokenType::RIGHT_PARENS) && !checkToken(TokenType::END)) {
                    auto expr = parseExpression();
                    if (expr->nodeType != NodeType::UNKNOWN) {
                        call->arguments.push_back(std::move(expr));
                    }
                    if (checkToken(TokenType::COMMA)) {
                        lexer.nextToken();  // Consume ,
                        if (checkToken(TokenType::RIGHT_PARENS)) {
                            DX.err().after("Expected another expression after the comma", *lexer.lastToken());
                        }
                    } else if (!checkToken(TokenType::RIGHT_PARENS) && !checkToken(TokenType::END)) {
                        DX.err().after("Expected either , or ) in call argument list", *lexer.lastToken());
                        break;
                    }
                }
                if (checkToken(TokenType::END)) {
                    DX.err().after("Unterminated code at end of file. Expected another expression", *lexer.lastToken());
                } else if (checkToken(TokenType::RIGHT_PARENS)) {
                    lexer.nextToken();  // Consume )
                }
                call->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
                expr = std::move(call);
            } else if (checkToken(TokenType::DEREF)) {
                auto deref = makeNode<ASTDeref>(lexer.nextToken()->index);  // Consume .*
                deref->inner = std::move(expr);
                deref->endIndex = deref->inner->endIndex;
                expr = std::move(deref);
            } else if (checkToken(TokenType::DOT)) {
                lexer.nextToken();  // Consume .
                if (checkToken(TokenType::LEFT_CURLY)) {
                    // parseTypeInit
                    auto typeInit = makeNode<ASTTypeInit>(expr->locIndex, expr->endIndex - expr->locIndex);
                    lexer.nextToken();  // Consume {
                    typeInit->typeRef = std::move(expr);

                    while (!checkToken(TokenType::RIGHT_CURLY) && !checkToken(TokenType::END)) {
                        auto declOrExpr = parseDeclOrExpr();
                        if (declOrExpr->nodeType == NodeType::DECL) {
                            auto assignmentDecl = castNodePtr<ASTDecl>(declOrExpr);
                            if (assignmentDecl->type != nullptr) {
                                DX.err()
                                    .at("Error in type initializer", *typeInit)
                                    .at("Variable declaration is not allowed here", *assignmentDecl);
                            }
                            typeInit->assignments.push_back(std::move(assignmentDecl));
                        } else {
                            if (declOrExpr->nodeType != NodeType::UNKNOWN) {
                                DX.err()
                                    .at("Error in type initializer", *typeInit)
                                    .at("Expression is not allowed here", *declOrExpr);
                            }
                        }
                        if (checkToken(TokenType::COMMA)) {
                            lexer.nextToken();  // Consume ,
                        } else if (!checkToken(TokenType::RIGHT_CURLY) && !checkToken(TokenType::END)) {
                            DX.err()
                                .at("Error in type initializer", *typeInit)
                                .after("Expected either , or }", *lexer.lastToken());
                            break;
                        }
                    }
                    if (checkToken(TokenType::END)) {
                        DX.err()
                            .at("Error in type initializer", *typeInit)
                            .after("Unterminated code at end of file. Expected closing }", *lexer.lastToken());
                    } else if (checkToken(TokenType::RIGHT_CURLY)) {
                        lexer.nextToken();  // Consume }
                    }
                    typeInit->endIndex = lexer.lastToken()->index + lexer.lastToken()->cLen;
                    expr = std::move(typeInit);
                } else {
                    auto dotOp = makeNode<ASTDotOp>(expr->locIndex);
                    dotOp->base = std::move(expr);
                    auto member = recurExpression(peekedPrec, depth);
                    if (member->nodeType != NodeType::NAME) {
                        DX.err().at("Expected a member variable of " + printExpr(*dotOp->base) + " but found " +
                                        printExpr(*member) + " instead",
                                    *member);
                        return unknownExpr();
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