#include <fstream>
#include <iostream>

#include "lexer.hpp"
#include "parser.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: scft [filePath.scft]" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        // Lexer lexer;
        // lexer.fromFilePath(argv[1]);
        // std::unique_ptr<Token> tkn;
        // do {
        //     tkn = lexer.nextToken();
        //     std::cout << tkn->type << "::" << tkn->index << "::" << tkn->cLen << std::endl;
        // } while (tkn->type != END_TKN);
        Parser parser;
        parser.parseProgram();
    } catch (const CompTimeException& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::ifstream::failure& e) {
        std::cerr << "Couldn't find file: " << argv[1] << std::endl;
        return EXIT_FAILURE;
    } catch (const char* e) {
        std::cerr << e << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
