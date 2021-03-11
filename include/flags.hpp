#pragma once

#include <iostream>

namespace Flags {

extern const char* filePath;

struct DumpInfo {
    bool print = false;
    bool verbose = false;
};
extern DumpInfo dumpInfo;
extern bool sourceFmt;

extern bool dwSemiColons;  //-dw-semi-colons

extern bool parseFlags(int argc, char** argv);

};  // namespace Flags
