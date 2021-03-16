#pragma once

namespace Flags {

extern const char* filePath;

struct DumpInfo {
    bool print = false;
    bool verbose = false;
};
extern DumpInfo dumpInfo;
extern bool sourceFmt;

extern bool dwSemiColons;  //-dw-semi-colons

extern bool parse_flags(int argc, char** argv);

}  // namespace Flags
