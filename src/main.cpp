#include <fstream>
#include <iostream>

#include "analysis.hpp"
#include "ast_printer.hpp"
#include "flags.hpp"
#include "lexer.hpp"
#include "parser.hpp"

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cerr << "Usage: scft [filePath.scft] -[options...]" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        if (Flags::parse_flags(argc, argv)) {
            Parser parser;
            if (parser.lexer.from_file_path(Flags::filePath)) {
                auto astTree = parser.parse_program();
                std::cout << parser.dx.emit() << std::endl;
                if (!parser.dx.has_errors()) {
                    if (Flags::fmtPrint) print_fmt_ast(astTree.get());
                    if (Flags::sourcePrint) std::cout << ast_to_src(astTree.get()) << std::endl;

                    Analyzer analyzer(parser.dx);
                    analyzer.analyze(astTree.get());
                    return EXIT_SUCCESS;
                }
            } else {
                std::cerr << "Couldn't find file: " << argv[1] << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return EXIT_FAILURE;
}