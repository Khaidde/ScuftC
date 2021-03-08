#pragma once

#include <iostream>

struct Flags {
    bool unknownFlag = false;

    const char* filePath;

    struct {
        bool print = false;
        bool verbose = false;
    } dumpAST;
    bool sourceFmt = false;

    bool dwSemiColons = false;  //-dw-semi-colons

    Flags(int argc, char** argv) {
        filePath = argv[1];
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-dump-ast") == 0) {
                dumpAST.print = true;

                if (i + 1 < argc && strcmp(argv[i + 1], "-v") == 0) {
                    dumpAST.verbose = true;
                    i++;
                }
            } else if (strcmp(argv[i], "-src") == 0) {
                sourceFmt = true;
            } else if (strcmp(argv[i], "-dw-semi-colons") == 0) {
                dwSemiColons = true;
            } else {
                std::cerr << "Unknown command line argument: " << argv[i] << std::endl;
                unknownFlag = true;
                return;
            }
        }
    }
};
