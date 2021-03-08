#pragma once

#include "ast.hpp"
#include "lexer.hpp"

struct TableEntry {
    TableEntry* next;
    TableEntry(TableEntry* next) : next(next) {}

    Token* identifier;
    ASTDecl* decl;
};

struct SymTable {
    static const int NUM_BUCKETS = 32;
    TableEntry** table;

    int id;  // TODO initialize id val
    SymTable* parent;

    SymTable() : parent(nullptr) {
        table = new TableEntry*[NUM_BUCKETS];

        for (int i = 0; i < NUM_BUCKETS; i++) {
            table[i] = nullptr;
        }
    }

    ~SymTable() { delete[] table; }

    void insert(Token* identifier, ASTDecl* decl);
    TableEntry* find(Token* identifier);
};