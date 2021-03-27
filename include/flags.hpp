#pragma once

#include <string.h>

#include <iostream>

namespace Flags {

const char* filePath;

bool sourcePrint = false;
bool fmtPrint = false;

bool dwSemiColons = false;  //-dw-semi-colons

bool parse_flags(int argc, char** argv) {
    filePath = argv[1];
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-ast-print") == 0) {
            fmtPrint = true;
        } else if (strcmp(argv[i], "-ast-src-print") == 0) {
            sourcePrint = true;
        } else if (strcmp(argv[i], "-dw-semi-colons") == 0) {
            dwSemiColons = true;
        } else {
            std::cerr << "Unknown command line argument: " << argv[i] << std::endl;
            return false;
        }
    }
    return true;
}

}  // namespace Flags
