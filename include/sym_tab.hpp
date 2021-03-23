#pragma once

struct ASTNode;
struct ASTDecl;
struct Token;

struct TableEntry {
    TableEntry* next;
    TableEntry(TableEntry* next) : next(next) {}

    Token* identifier;
    ASTDecl* decl;
};

struct SymTable {
    static constexpr int NUM_BUCKETS = 32;
    TableEntry* table[NUM_BUCKETS];

    int id;  // TODO initialize id val
    SymTable* parent;

    ASTNode* source;

    SymTable() : parent(nullptr) {
        for (int i = 0; i < NUM_BUCKETS; i++) {
            table[i] = nullptr;
        }
    }

    ~SymTable() {
        for (int i = 0; i < NUM_BUCKETS; i++) {
            TableEntry* cur = table[i];
            while (cur != nullptr) {
                TableEntry* next = cur->next;
                delete cur;
                cur = next;
            }
        }
    }

    void insert(Token* identifier, ASTDecl* decl);
    TableEntry* find(Token* identifier);
};