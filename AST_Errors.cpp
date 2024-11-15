#include <iostream>
#include "parser.hpp"
#include "scanner.hpp"

using namespace std;

int main() {

    string filename = "code.txt";
    read_file(filename, text_Arr, sizee);

    cout << endl << "S C A N N E R" << endl << endl;
    scanner(text_Arr);
    for (auto it = tokens.begin(); it != tokens.end(); ) {
        if (it->token_name == "TOKEN_COMLINE" ||
            it->token_name == "TOKEN_COMBLOCKS" ||
            it->token_name == "TOKEN_COMBLOCKE") {
            it = tokens.erase(it);  // Erase and get the next valid iterator
        }
        else {
            ++it;  // Only increment if no erase happened
        }
    }

    cout << endl << "T O K E N S" << endl << endl;
    for (auto tok : tokens) {
        cout << tok.token_name << endl;
    }
    // Crear el objeto Parser con el vector de tokens

    cout << endl << "P A R S E R" << endl << endl;

    Parser parser(tokens);
    std::unique_ptr<ASTNode> ast;
    // Ejecutar el análisis sintáctico
    if (parser.parse(ast)) {
        std::cout << "Parsing exitoso." << std::endl;
    }
    else {
        std::cout << "Error de parsing." << std::endl;
    }

    return 0;
}