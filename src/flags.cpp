#include "flags.hpp"

#include <iostream>
#include <string>

namespace Flags {

const char* filePath;

bool sourceFmt = false;

DumpInfo dumpInfo;
bool dwSemiColons = false;  //-dw-semi-colons

bool parse_flags(int argc, char** argv) {
    filePath = argv[1];
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-dump-ast") == 0) {
            dumpInfo.print = true;

            if (i + 1 < argc && strcmp(argv[i + 1], "-v") == 0) {
                dumpInfo.verbose = true;
                i++;
            }
        } else if (strcmp(argv[i], "-src") == 0) {
            sourceFmt = true;
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
