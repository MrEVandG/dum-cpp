#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
#include <string>

#include "./generation.hpp"

int main(int argc, char* argv[])
{
    if (std::string(argv[1]) =="--help" || argc < 2) {
        std::cout << "=-----------------------------------------=" << std::endl;
        std::cout << "|              DUMB Help Menu             |" << std::endl;
        std::cout << "| A dumb language for dumber programmers. |" << std::endl;
        std::cout << "=-----------------------------------------=" << std::endl;
        std::cout << "\033[0;32m-tk \033[0;mor \033[0;32m--tokenization \033[0;m- Tokenizes the file and pretty prints it to the console." << std::endl;
        std::cout << "\033[0;32m-ast \033[0;mor \033[0;32m--syntax-tree \033[0;m- Tokenize and parse the file, then pretty print the AST to the console." << std::endl;
        std::cout << "\033[0;31m-asm \033[0;mor \033[0;31m--assembly \033[0;mor \033[0;31m--no-link \033[0;m- Tokenizes, parses, and compiles the file into assembly, as 'out.asm', without linking the file into an executable. NOTE: This is the default behavior when if mo flags are passed in." << std::endl;
        std::cout << "\033[0;31m-h \033[0;mor \033[0;31m--help \033[0;m- Shows this help menu. NOTE: This is the default if no arguments are passed in." << std::endl;
        std::cout << "\033[0;32m-a \033[0;mor \033[0;32m--all \033[0;m- Tokenizes, parses, compiles, and links the file into a Linux executable. NOTE: This file does need to be 'chmod'ed. However, if you can't run it, run: \033[0;1m $ chmod +x ./out" << std::endl;
        return EXIT_SUCCESS;
    }

    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        if (input.fail()) {
            std::cerr << "File not found: `" << argv[1] << "`." << std::endl;
            return EXIT_FAILURE;
        }
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();

    if (std::string(argv[2]) == "-tk"||std::string(argv[2])=="--tokenization") {
        std::cout << "[";
        for (int i = 0; i < tokens.size(); i++) {
            std::cout << TokenTypes[int(tokens.at(i).type)] << ", ";
        }
        std::cout << "]" << std::endl;
        return EXIT_SUCCESS;
    }

    Parser parser(std::move(tokens));
    std::optional<NodeProg> prog = parser.parse_prog();
    if (!prog.has_value()) {
        std::cerr << "Parser error" << std::endl;
        return EXIT_FAILURE;
    }

    if (std::string(argv[2]) == "-ast" || std::string(argv[2]) == "--syntax-tree") {
        std::cout << parser.prog_to_string(prog.value()) << std::endl;
        return EXIT_SUCCESS;
    }

    Generator generator(prog.value());
    {
        std::fstream file("out.asm", std::ios::out);
        file << generator.gen_prog();
    }

    if (std::string(argv[2])=="-a"||std::string(argv[2])=="--all") {
        system("nasm -felf64 out.asm");
        system("ld -o out out.o");
    }

    return EXIT_SUCCESS;
}