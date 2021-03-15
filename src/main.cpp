#include <fstream>
#include <iostream>

#include "flags.hpp"
#include "lexer.hpp"
#include "parser.hpp"

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cerr << "Usage: scft [filePath.scft] -[options...]" << std::endl;
        return EXIT_FAILURE;
    }
    if (!Flags::parseFlags(argc, argv)) return EXIT_FAILURE;

    try {
        Parser parser;
        parser.lexer.fromFilePath(Flags::filePath);
        auto astTree = parser.parse_program();
        std::cout << parser.dx.emit() << std::endl;
        if (parser.dx.has_errors()) return EXIT_FAILURE;

        if (Flags::dumpInfo.print) dumpAST(*astTree, Flags::dumpInfo.verbose);
        if (Flags::sourceFmt) std::cout << printAST(*astTree) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::ifstream::failure& e) {
        std::cerr << "Couldn't find file: " << argv[1] << std::endl;
        return EXIT_FAILURE;
    } catch (const char* e) {
        std::cerr << "Warning: DON'T THROW STRINGS\n\t" << e << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
