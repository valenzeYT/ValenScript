#include <iostream>
#include <fstream>
#include <sstream>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/interpreter.h"
#include <iomanip>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Cannot open file: " + filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    std::cout << std::fixed << std::setprecision(17);

    if (argc < 3) {
        std::cerr << "Usage: valen <command> <file>\n";
        return 1;
    }

    std::string command = argv[1];
    std::string filename = argv[2];

    try {
        if (command == "init") {

            std::string input = readFile(filename);

            Lexer lexer(input);
            Parser parser(lexer);
            auto statements = parser.parse();

            Interpreter interpreter;
            interpreter.interpret(statements);

        } else {
            std::cerr << "Unknown command: " << command << "\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
