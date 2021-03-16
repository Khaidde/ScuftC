#include "sym_tab.hpp"

#include "lexer.hpp"

namespace {

int hash(const Token& identifier) {
    const char* offset = identifier.sourceStr->c_str() + identifier.beginI;
    int hash = 0;
    int cLen = identifier.endI - identifier.beginI;
    for (int i = 0; i < cLen; i++) {
        hash = hash * 31 + *(offset + i);
    }
    return (hash & 0x7fffffff) % SymTable::NUM_BUCKETS;
}

// Compare the identifier names as present in the source file string
bool cmp_identifier(const Token& first, const Token& second) {
    int firstCLen = first.endI - first.beginI;
    int secondCLen = second.endI - second.beginI;
    if (firstCLen != secondCLen) return false;
    const char* firstStr = first.sourceStr->c_str() + first.beginI;
    const char* secondStr = second.sourceStr->c_str() + second.beginI;

    for (int i = 0; i < firstCLen; i++) {
        if (firstStr[i] != secondStr[i]) return false;
    }
    return true;
}

}  // namespace

void SymTable::insert(Token* identifier, ASTDecl* decl) {
    int index = hash(*identifier);

    // TODO search through linked list for existing identifier

    TableEntry* entry = new TableEntry(table[index]);
    table[index] = entry;

    entry->identifier = identifier;
    entry->decl = decl;
}

TableEntry* SymTable::find(Token* identifier) {
    TableEntry* chain = table[hash(*identifier)];
    while (chain != nullptr) {
        if (cmp_identifier(*chain->identifier, *identifier)) return chain;
        chain = chain->next;
    }
    return nullptr;
}