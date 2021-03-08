#include "sym_tab.hpp"

namespace {

int hash(const Token& identifier) {
    const char* offset = identifier.src->c_str() + identifier.index;
    int hash = 0;
    for (int i = 0; i < identifier.cLen; i++) {
        hash = hash * 31 + *(offset + i);
    }
    return (hash & 0x7fffffff) % SymTable::NUM_BUCKETS;
}

// Compare the identifier names as present in the source file string
bool cmpIdentifier(const Token& first, const Token& second) {
    if (first.cLen != second.cLen) return false;
    const char* firstStr = first.src->c_str() + first.index;
    const char* secondStr = second.src->c_str() + second.index;

    for (int i = 0; i < first.cLen; i++) {
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
    int searchIndex = hash(*identifier);

    TableEntry* chain = table[searchIndex];
    while (chain != nullptr) {
        if (cmpIdentifier(*chain->identifier, *identifier)) return chain;
        chain = chain->next;
    }
    return nullptr;
}