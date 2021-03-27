#pragma once

#include "ast.hpp"

const char* node_type_to_str(NodeType type);

std::string ast_to_src(ASTNode* node);
std::string ast_to_line(ASTNode* node);

void print_fmt_ast(ASTNode* node);