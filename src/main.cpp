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

    try {
        Flags flags(argc, argv);
        if (flags.unknownFlag) return EXIT_FAILURE;

        Parser parser(flags);
        auto astTree = parser.parseProgram();
        std::cout << parser.DX.out() << std::endl;

        if (flags.dumpAST.print) dumpAST(*astTree, flags.dumpAST.verbose);
        if (flags.sourceFmt) std::cout << printAST(*astTree) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::ifstream::failure& e) {
        std::cerr << "Couldn't find file: " << argv[1] << std::endl;
        return EXIT_FAILURE;
    } catch (Diagnostics& d) {
        std::cerr << "\nWarning: DON'T THROW DIAGNOSTICS\n\n" << d.out() << std::endl;
        return EXIT_FAILURE;
    } catch (const char* e) {
        std::cerr << "Warning: DON'T THROW STRINGS\n\t" << e << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
