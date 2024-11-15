#include "parser.hpp"
#include "iostream"
#include <memory>
#include <vector>
#include <string>
#include <unordered_set>
#include <fstream>  
int errorCount = 0;
string fallo = "vacio";
int token_num = 0;

void printParserNode(const unique_ptr<ParserNode>& node, int level = 0) {
    if (!node) return; 
    cout << string(level * 2, ' ') << "[" << level << "] " << node->token_name << endl;
    for (size_t i = 0; i < node->children.size(); ++i) {
        printParserNode(node->children[i], level + 1);
    }
}

void exportToDot(const unique_ptr<ParserNode>& node, ofstream& dotFile, int& nodeId) {
    if (!node) return;

    int currentId = nodeId++;
    dotFile << "    node" << currentId << " [label=\"" << node->token_name << "\"];" << endl;

    for (const auto& child : node->children) {
        int childId = nodeId;
        exportToDot(child, dotFile, nodeId);
        dotFile << "    node" << currentId << " -> node" << childId << ";" << endl;
    }
}

void generateDotFile(const unique_ptr<ParserNode>& root, const string& filename) {
    ofstream dotFile(filename);
    if (!dotFile.is_open()) {
        cerr << "Error al abrir el archivo DOT." << endl;
        return;
    }

    dotFile << "digraph AST {" << endl;
    dotFile << "    node [shape=box];" << endl;

    int nodeId = 0;
    exportToDot(root, dotFile, nodeId);

    dotFile << "}" << endl;
    dotFile.close();
    cout << "Árbol exportado al archivo DOT: " << filename << endl;
}

Parser::Parser(const vector<token>& tokens) : tokens(tokens), current(0) {
}
bool Parser::parse(unique_ptr<ParserNode>& root) {
    bool success = Program(root);
    if (errorCount > 0) {
        cerr << "Parsing completed with " << errorCount << " errors." << endl;
        return false;
    }
    //unique_ptr<ASTNode> astree=ConvertParserTreeToAST(root);
    printParserNode(root);
    generateDotFile(root, "tree.dot");
    system("dot -Tpng tree.dot -o astree.png");
    return success;
}
