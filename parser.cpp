#include "parser.hpp"
#include "iostream"
#include <memory>
#include <vector>
#include <string>
#include <unordered_set>
#include <fstream>  // Para trabajar con archivos
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

token Parser::currToken() {
  return tokens[current];
}

token Parser::nextToken() {
  return tokens[current++];
}

void Parser::fail(string message) {
   // cerr << "Error at token " << current << ": " << message << endl;
    if (fallo == "vacio") { fallo = message; token_num = current;}
    
}

void Parser::syncToDelimiter() {
    static const unordered_set<string> delimiters = { "TOKEN_;", "TOKEN_]", "TOKEN_)" };

    while (current < tokens.size() && delimiters.find(currToken().token_name) == delimiters.end()) {
        nextToken();
    }

    if (current < tokens.size()) {
        nextToken();
    }
}

bool Parser::nonTerminal(string name) {
  if (currToken().token_name == name) {
    nextToken();
    return true;
  }
  return false;
}

/*
  Type' -> [ ] Type'
  Type' -> ε
*/
/*
  Type' -> [ ] Type'
  Type' -> ε
*/
bool Parser::TypePrime(unique_ptr<ParserNode>& node) {
    if (nonTerminal("TOKEN_[")) {
        // Crear el nodo ArrayType si aún no existe
        if (!node) {
            node = make_unique<ParserNode>("ArrayType");
        }

        // Añadir un hijo '[]' para representar los corchetes
        node->addChild(make_unique<ParserNode>("[]"));

        // Continuar verificando más corchetes
        if (nonTerminal("TOKEN_]") && TypePrime(node)) {
            return true;
        }

        fail("Array type mal formado");
        return false;
    }
    return true; // No hay más corchetes
}




/*
  BasicType -> IntType
  BasicType -> BoolType
  BasicType -> CharType
  BasicType -> StringType
  BasicType -> VoidType
*/
bool Parser::BasicType(unique_ptr<ParserNode>& node) {
    string name_temp = currToken().token_name;
    if (nonTerminal("TOKEN_IntType") || nonTerminal("TOKEN_BoolType") || 
        nonTerminal("TOKEN_CharType") || nonTerminal("TOKEN_StringType") || 
        nonTerminal("TOKEN_VoidType")) {

        node = make_unique<ParserNode>(name_temp); 
        return true;
    }
    fail("Tipo básico no válido");
    return false;
}



/*
  Type -> BasicType Type'
*/
/*
  Type -> BasicType Type'
*/
bool Parser::Type(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> basicTypeNode;
    unique_ptr<ParserNode> arrayTypeNode;

    // Verificamos el tipo básico y luego los corchetes (arrays)
    if (BasicType(basicTypeNode) && TypePrime(arrayTypeNode)) {
        node = move(basicTypeNode);
        // Si hay nodos de tipo array, los agregamos como hijos
        if (arrayTypeNode) {
            node->addChild(move(arrayTypeNode));
        }
        return true;
    }
    fail("No es un tipo válido");
    return false;
}




/*
  Function -> Type Identifier (Params) { StmtList }
*/
bool Parser::Function(unique_ptr<ParserNode>& node) {
    node = make_unique<ParserNode>("Function");

    unique_ptr<ParserNode> typeNode;
    unique_ptr<ParserNode> paramsNode;
    unique_ptr<ParserNode> compoStmt;

    if (Type(typeNode) &&
        nonTerminal("TOKEN_ID") && 
        nonTerminal("TOKEN_(") && 
        Params(paramsNode) &&
        nonTerminal("TOKEN_)") && 
        CompoundStmt(compoStmt)) {
        
        node->addChild(move(typeNode)); // Tipo de la función
        node->addChild(make_unique<ParserNode>("Identifier")); // Nombre de la función
        //node->addChild(make_unique<ParserNode>("Identifier", currToken().token_value));
        if (paramsNode) { 
            node->addChild(move(paramsNode)); // Parámetros
        }

        node->addChild(move(compoStmt)); // Cuerpo de la función
        return true;
    }
    fail("Función mal declarada");
    return false;
}



/*
  VarDecl' -> ;
  VarDecl' -> = Expression ;
*/
bool Parser::VarDeclPrime(unique_ptr<ParserNode>& node, unique_ptr<ParserNode> idNode) {
    unique_ptr<ParserNode> exprNode;

    if (nonTerminal("TOKEN_;")) {
        // Nodo sin inicialización, se omite en el AST final.
        return true;
    }
    else if (nonTerminal("TOKEN_=") && Expression(exprNode) && nonTerminal("TOKEN_;")) {
        node = make_unique<ParserNode>("="); // Nodo de asignación
        node->addChild(move(idNode)); // Agrega el identificador como hijo
        node->addChild(move(exprNode)); // Agrega la expresión como hijo
        return true;
    }
    fail("Error en la declaración de variable");
    return false;
}




/*
  VarDecl -> Type Identifier VarDecl'
*/
/*
  VarDecl -> Type Identifier VarDecl'
*/
bool Parser::VarDecl(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> typeNode;
    unique_ptr<ParserNode> idNode;
    unique_ptr<ParserNode> varDeclPrimeNode;

    if (Type(typeNode) && nonTerminal("TOKEN_ID")) {
        // Crear nodo para el identificador
        idNode = make_unique<ParserNode>("Identifier");
        //idNode = make_unique<ParserNode>("Identifier", currToken().token_value);

        // Llamar a VarDeclPrime con el nodo del identificador
        if (VarDeclPrime(varDeclPrimeNode, move(idNode))) {
            node = make_unique<ParserNode>("VarDecl"); // Nodo para declaración de variable
            node->addChild(move(typeNode)); // Agregar tipo de la variable

            // Si hay inicialización, agregar el nodo de asignación
            if (varDeclPrimeNode) {
                node->addChild(move(varDeclPrimeNode));
            } else {
                // Agregar el identificador si no hay inicialización
                node->addChild(make_unique<ParserNode>("Identifier"));
                //node->addChild(make_unique<ParserNode>("Identifier", currToken().token_value));
            }
            return true;
        }
    }
    fail("Error en declaración de variable: se esperaba un identificador o un punto y coma");
    return false;
}


/*
  Declaration -> [ Function ]
  Declaration -> VarDecl
*/
bool Parser::Declaration(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> functionNode;
    unique_ptr<ParserNode> varDeclNode;

    if (nonTerminal("TOKEN_[") && Function(functionNode) && nonTerminal("TOKEN_]")) {
        node = move(functionNode); // Conserva la función como nodo principal
        return true;
    }
    else if (VarDecl(varDeclNode)) {
        node = move(varDeclNode); // Conserva la declaración de variable como nodo principal
        return true;
    }
    fail("Error en declaración: se esperaba una función o declaración de variable");
    return false;
}


/*
  Program' -> Declaration Program'
  Program' -> ε
*/
bool Parser::ProgramPrime(unique_ptr<ParserNode>& node) {
    if (current >= tokens.size()) {
        return true; // Fin del programa
    }

    unique_ptr<ParserNode> declNode;
    if (Declaration(declNode)) {
        if (!node) {
            node = make_unique<ParserNode>("Program");
        }
        node->addChild(move(declNode)); // Agrega la declaración al nodo del programa

        // Procesa recursivamente las siguientes declaraciones
        return ProgramPrime(node);
    }

    fail("Error en declaración");
    errorCount++;
    cerr << "Error en el token " << token_num << ": " << fallo << endl;
    syncToDelimiter();
    fallo = "vacio";
    return ProgramPrime(node);
}


/*
  Program -> Declaration Program'
*/
bool Parser::Program(unique_ptr<ParserNode>& root) {
    unique_ptr<ParserNode> declNode;
    if (Declaration(declNode)) {
        root = make_unique<ParserNode>("Program");
        root->addChild(move(declNode));

        // Agregar declaraciones adicionales con `ProgramPrime`
        unique_ptr<ParserNode> programPrimeNode;
        ProgramPrime(programPrimeNode);
        for (auto& child : programPrimeNode->children) {
            root->addChild(move(child));
        }

        return true;
    }
    fail("Error en el programa: inicio no válido");
    return false;
}


////////////////////////////////////////////////////////////

/*
  Params -> ParamList
  Params -> ε
*/
bool Parser::Params(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> paramListNode;
    if (ParamList(paramListNode)) {
        node = make_unique<ParserNode>("Params"); // Nodo para la lista de parámetros
        node->addChild(std::move(paramListNode));
        return true;
    }
    return true; // Si es vacío, no agregamos nodo al AST
}


/*
  ParamList -> Type Identifier ParamList'
*/
bool Parser::ParamList(unique_ptr<ParserNode>& node) {
    node = make_unique<ParserNode>("ParamList");

    unique_ptr<ParserNode> typeNode;
    unique_ptr<ParserNode> paramListPrimeNode;

    if (Type(typeNode) && nonTerminal("TOKEN_ID") && ParamListPrime(paramListPrimeNode)) {
        node->addChild(std::move(typeNode)); // Agrega el tipo del parámetro

        // Agregar el identificador del parámetro
        node->addChild(make_unique<ParserNode>("Identifier"));
        //node->addChild(make_unique<ParserNode>("Identifier", currToken().token_value));

        // Agregar los siguientes parámetros (si existen)
        for (auto& child : paramListPrimeNode->children) {
            node->addChild(std::move(child));
        }
        return true;
    }
    fail("Error en lista de parámetros: se esperaba tipo e identificador");
    return false;
}


/*
  ParamList' -> , Type Identifier ParamList'
  ParamList' -> ε
*/
bool Parser::ParamListPrime(unique_ptr<ParserNode>& node) {
    node = make_unique<ParserNode>("ParamListPrime");

    unique_ptr<ParserNode> typeNode;
    unique_ptr<ParserNode> paramListPrimeNode;

    if (nonTerminal("TOKEN_,") && Type(typeNode) && nonTerminal("TOKEN_ID") && ParamListPrime(paramListPrimeNode)) {
        node->addChild(std::move(typeNode)); // Agrega el tipo del parámetro

        // Agregar el identificador del parámetro
        node->addChild(make_unique<ParserNode>("Identifier"));
        //node->addChild(make_unique<ParserNode>("Identifier", currToken().token_value));

        // Agregar cualquier parámetro adicional en la lista
        for (auto& child : paramListPrimeNode->children) {
            node->addChild(std::move(child));
        }
        return true;
    }
    return true; // Nodo vacío si no hay parámetros adicionales
}


/*
  StmtList -> Statement StmtList'
*/
bool Parser::StmtList(unique_ptr<ParserNode>& node) {
    node = make_unique<ParserNode>("StmtList");

    unique_ptr<ParserNode> stmtNode;
    unique_ptr<ParserNode> stmtListPrimeNode;

    if (Statement(stmtNode) && StmtListPrime(stmtListPrimeNode)) {
        node->addChild(std::move(stmtNode)); // Agregar la primera sentencia

        // Agregar todas las sentencias adicionales
        for (auto& child : stmtListPrimeNode->children) {
            node->addChild(std::move(child));
        }
        return true;
    }
    fail("Error en lista de sentencias: sentencia no válida");
    return false;
}



/*
  StmtList' -> Statement StmtList'
  StmtList' -> ε
*/
bool Parser::StmtListPrime(unique_ptr<ParserNode>& node) {
    node = make_unique<ParserNode>("StmtListPrime");

    unique_ptr<ParserNode> stmtNode;
    unique_ptr<ParserNode> stmtListPrimeNode;

    if (Statement(stmtNode) && StmtListPrime(stmtListPrimeNode)) {
        node->addChild(std::move(stmtNode)); // Agregar la sentencia

        // Agregar todas las sentencias adicionales
        for (auto& child : stmtListPrimeNode->children) {
            node->addChild(std::move(child));
        }
        return true;
    }
    return true; // Nodo vacío si no hay sentencias adicionales
}


/*
  CompoundStmt -> { StmtList }
*/
bool Parser::CompoundStmt(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> stmtListNode;

    if (nonTerminal("TOKEN_{") && StmtList(stmtListNode) && nonTerminal("TOKEN_}")) {
        node = make_unique<ParserNode>("CompoundStmt"); // Nodo para el bloque de sentencias
        node->addChild(std::move(stmtListNode)); // Agrega las sentencias del bloque
        return true;
    }
    fail("Error en bloque de sentencias: se esperaba '{' o '}'");
    return false;
}



/*
ExprStmt ::= Expression ;
ExprStmt ::= ;
*/
bool Parser::ExprStmt(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprNode;
    if (Expression(exprNode) && nonTerminal("TOKEN_;")) {
        node = make_unique<ParserNode>("ExprStmt");
        node->addChild(std::move(exprNode)); // Agregar solo la expresión
        return true;
    }
    else if (nonTerminal("TOKEN_;")) {
        node = make_unique<ParserNode>("EmptyExprStmt");
        return true;
    }
    fail("Error en sentencia de expresión: falta punto y coma");
    return false;
}


/*
PrintStmt -> print ( ExprList ) ;
*/
bool Parser::PrintStmt(unique_ptr<ParserNode>& node) {
    node = make_unique<ParserNode>("PrintStmt");

    unique_ptr<ParserNode> exprListNode;
    if (nonTerminal("TOKEN_print") &&
        nonTerminal("TOKEN_(") &&
        ExprList(exprListNode) &&
        nonTerminal("TOKEN_)") &&
        nonTerminal("TOKEN_;")) {
        
        node->addChild(std::move(exprListNode)); // Agregar solo ExprList
        return true;
    }
    fail("Error en sentencia de impresión: se esperaba '(' o ')'");
    return false;
}


/*
ReturnStmt -> return Expression ;
*/
bool Parser::ReturnStmt(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprNode;
    if (nonTerminal("TOKEN_return") && Expression(exprNode) && nonTerminal("TOKEN_;")) {
        node = make_unique<ParserNode>("ReturnStmt");
        node->addChild(std::move(exprNode)); // Agregar solo la expresión de retorno
        return true;
    }
    fail("Error en sentencia de retorno: falta expresión o punto y coma");
    return false;
}


/*
ForStmt -> for ( ExprStmt Expression ; ) Statement
*/
bool Parser::ForStmt(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprStmt1, exprStmt2, exprStmt3, stmtNode;
    if (nonTerminal("TOKEN_for") &&
        nonTerminal("TOKEN_(") &&
        ExprStmt(exprStmt1) &&
        ExprStmt(exprStmt2) &&
        ExprStmt(exprStmt3) &&
        nonTerminal("TOKEN_)") &&
        Statement(stmtNode)) {

        node = make_unique<ParserNode>("ForStmt");
        node->addChild(std::move(exprStmt1)); // Inicialización
        node->addChild(std::move(exprStmt2)); // Condición
        node->addChild(std::move(exprStmt3)); // Incremento
        node->addChild(std::move(stmtNode));  // Cuerpo del for
        return true;
    }
    fail("Error en sentencia for: se esperaba ';' o ')'");
    return false;
}

/*
AuxIf -> else {Statement}
AuxIf -> ‘’
*/
bool Parser::AuxIf(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> stmtListNode;
    if (nonTerminal("TOKEN_else") &&
        nonTerminal("TOKEN_{") &&
        StmtList(stmtListNode) &&
        nonTerminal("TOKEN_}")) {
        
        node = make_unique<ParserNode>("AuxIf");
        node->addChild(std::move(stmtListNode)); // Agregar solo las sentencias del else
        return true;
    }
    return true; // Nodo vacío si no hay else
}


/*
IfStmt -> if ( Expression ) {Statement} AuxIf
*/
bool Parser::IfStmt(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprNode, stmtListNode, auxIfNode;
    if (nonTerminal("TOKEN_if") &&
        nonTerminal("TOKEN_(") &&
        Expression(exprNode) &&
        nonTerminal("TOKEN_)") &&
        nonTerminal("TOKEN_{") &&
        StmtList(stmtListNode) &&
        nonTerminal("TOKEN_}") &&
        AuxIf(auxIfNode)) {

        node = make_unique<ParserNode>("IfStmt");
        node->addChild(std::move(exprNode)); // Condición
        node->addChild(std::move(stmtListNode)); // Cuerpo del if
        if (auxIfNode) {
            node->addChild(std::move(auxIfNode)); // Bloque else si está presente
        }
        return true;
    }
    fail("Error en sentencia if: se esperaba '(' o '{'");
    return false;
}


/*
  Statement -> VarDecl
  Statement -> IfStmt
  Statement -> ForStmt
  Statement -> ReturnStmt
  Statement -> ExprStmt
  Statement -> PrintStmt
  Statement -> { StmtList }
  Statement -> PrintStmt
*/
bool Parser::Statement(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> stmtNode;

    if (VarDecl(stmtNode) || IfStmt(stmtNode) || ForStmt(stmtNode) || ReturnStmt(stmtNode) ||
        ExprStmt(stmtNode) || CompoundStmt(stmtNode) || PrintStmt(stmtNode)) {

        node = std::move(stmtNode); // Asigna directamente el nodo correspondiente
        return true;
    }

    fail("Error en declaración: no se encontró una declaración válida");
    return false;
}


/*
auxPrimary ::= ( ExprList )
auxPrimary ::= ''
*/
bool Parser::AuxPrimary(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprListNode;
    if (nonTerminal("TOKEN_(") && ExprList(exprListNode) && nonTerminal("TOKEN_)")) {
        node = std::move(exprListNode); // Solo conservar ExprList
        return true;
    }
    return true; // Nodo vacío si no hay llamada de función
}


/*
Primary ::= Identifier auxPrimary
Primary ::= IntegerLiteral -->num
Primary ::= CharLiteral  --> 'char'
Primary ::= StringLiteral --> "text_string"
Primary ::= BooleanLiteral --> True o false
Primary ::= ( Expression )
*/

/*fallo del TOKEN_ID, no lo deberia imprimir, revisar mañana*/

bool Parser::Primary(unique_ptr<ParserNode>& node) {
    string bool_name = currToken().token_name;
    unique_ptr<ParserNode> exprNode;

    // Identificador
    if (nonTerminal("TOKEN_ID")) {
        unique_ptr<ParserNode> auxPrimaryNode;
        
        // Intentar procesar el auxiliar
        if (AuxPrimary(auxPrimaryNode)) {
            if (auxPrimaryNode) {
                // Si AuxPrimary devuelve un nodo, no añadimos TOKEN_ID
                node = std::move(auxPrimaryNode);
            } else {
                // Solo creamos el nodo para TOKEN_ID si no forma parte de una expresión compleja
                node = make_unique<ParserNode>("Identifier");
                //node = make_unique<ParserNode>("Identifier", currToken().token_value);
            }
        }
        return true;
    }

    // Número entero
    if (nonTerminal("TOKEN_Num")) {
        node = make_unique<ParserNode>("TOKEN_Num");
        return true;
    }

    // Booleano (true/false)
    if (nonTerminal("TOKEN_True") || nonTerminal("TOKEN_False")) {
        node = make_unique<ParserNode>(bool_name);
        return true;
    }

    // Cadena de texto
    if (nonTerminal("TOKEN_Comilla_doble") &&
        nonTerminal("TOKEN_Text_string") &&
        nonTerminal("TOKEN_Comilla_doble")) {
        node = make_unique<ParserNode>("TOKEN_Text_string");
        return true;
    }

    // Carácter
    if (nonTerminal("TOKEN_Comilla") &&
        nonTerminal("TOKEN_ID") &&
        nonTerminal("TOKEN_Comilla")) {
        node = make_unique<ParserNode>("TOKEN_Char");
        return true;
    }

    // Expresión entre paréntesis
    if (nonTerminal("TOKEN_(") && Expression(exprNode) && nonTerminal("TOKEN_)")) {
        node = std::move(exprNode); // Retorna directamente la expresión
        return true;
    }

    fail("Error en Primary: se esperaba un identificador, número, booleano, cadena de texto, o una expresión entre paréntesis.");
    return false;
}




/*
Factor' ::= [ Expression ] Factor'
Factor' ::= ''
*/
bool Parser::FactorPrime(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprNode, factorPrimeNode;
    if (nonTerminal("TOKEN_[") && Expression(exprNode) && nonTerminal("TOKEN_]") && FactorPrime(factorPrimeNode)) {
        node = make_unique<ParserNode>("ArrayIndex");
        node->addChild(std::move(exprNode));
        if (factorPrimeNode) {
            node->addChild(std::move(factorPrimeNode));
        }
        return true;
    }
    return true; // Nodo vacío si no hay índice de arreglo
}


/*
Factor ::= Primary Factor'
*/
bool Parser::Factor(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> primaryNode, factorPrimeNode;
    if (Primary(primaryNode) && FactorPrime(factorPrimeNode)) {
        node = std::move(primaryNode); // Asigna el nodo de Primary como raíz

        if (factorPrimeNode) {
            node->addChild(std::move(factorPrimeNode));
        }
        return true;
    }
    fail("Error en Factor: se esperaba una expresion primaria seguida de un operador o índice opcional.");
    return false;
}


/*
Unary ::= ! Unary
Unary ::= - Unary
Unary ::= Factor
*/
bool Parser::Unary(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> unaryNode, factorNode;
    string temp_neg = currToken().token_name;

    if ((nonTerminal("TOKEN_!") || nonTerminal("TOKEN_-")) && Unary(unaryNode)) {
        node = make_unique<ParserNode>("UnaryOp");
        node->addChild(make_unique<ParserNode>(temp_neg)); // Agregar el operador
        node->addChild(std::move(unaryNode)); // Agregar el operando
        return true;
    }
    else if (Factor(factorNode)) {
        node = std::move(factorNode); // Solo conservar el Factor
        return true;
    }
    fail("Error en Unary: se esperaba un operador de negación '!' o '-' o una expresion de factor.");
    return false;
}

/////////////////////////////////////////////////////////////////////


/*
TermPrime ::= * Unary TermPrime
TermPrime ::= / Unary TermPrime
TermPrime ::= % Unary TermPrime
TermPrime ::= ''
*/

bool Parser::TermPrime(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> unaryNode, termPrimeNode;
    string temp_op = currToken().token_name;

    // Verificamos si hay un operador multiplicativo (*, /, %)
    if ((nonTerminal("TOKEN_*") || nonTerminal("TOKEN_/") || nonTerminal("TOKEN_%")) &&
        Unary(unaryNode) && TermPrime(termPrimeNode)) {
        
        node = make_unique<ParserNode>(temp_op);  // Solo agregamos el operador
        node->addChild(std::move(unaryNode));     // Agregar operando izquierdo
        node->addChild(std::move(termPrimeNode)); // Agregar término derecho
        return true;
    }
    // Si no hay más operadores, retornamos el nodo vacío
    return true;
}


/*
Term ::= Unary TermPrime
*/

bool Parser::Term(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> unaryNode, termPrimeNode;

    // Aseguramos que hay un nodo Unary y un nodo TermPrime (opcional)
    if (Unary(unaryNode) && TermPrime(termPrimeNode)) {
        node = std::move(unaryNode);  // El nodo raíz de Term será Unary

        // Si hay operadores multiplicativos, los agregamos
        if (termPrimeNode) {
            node->addChild(std::move(termPrimeNode));
        }
        return true;
    }

    fail("Error en Term: se esperaba una expresión unaria seguida de un operador de multiplicación, división o módulo.");
    return false;
}


/*
ExprPrime ::= + Term ExprPrime
ExprPrime ::= - Term ExprPrime
ExprPrime ::= ''
*/

bool Parser::ExprPrime(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> termNode, exprPrimeNode;
    string temp_op = currToken().token_name;

    // Verificamos si hay un operador de adición (+ o -)
    if ((nonTerminal("TOKEN_+") || nonTerminal("TOKEN_-")) &&
        Term(termNode) && ExprPrime(exprPrimeNode)) {

        // El nodo raíz de ExprPrime será el operador de adición o sustracción
        node = make_unique<ParserNode>(temp_op);  
        node->addChild(std::move(termNode));  // Agregar operando de la derecha
        node->addChild(std::move(exprPrimeNode));  // Recursión para más operadores
        return true;
    }
    
    // Si no hay más operadores, retornamos el nodo vacío
    return true;
}

/*
Expr ::= Term Expr'
*/

bool Parser::Expr(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> termNode, exprPrimeNode;

    // Verificamos si hay un término y un ExprPrime
    if (Term(termNode) && ExprPrime(exprPrimeNode)) {
        node = std::move(termNode);  // El nodo raíz de Expr será el Term

        // Si ExprPrime tiene algún operador, lo agregamos
        if (exprPrimeNode) {
            node->addChild(std::move(exprPrimeNode));
        }
        return true;
    }

    fail("Error en Expr: se esperaba un término seguido de un operador de suma o resta.");
    return false;
}


/*
RelExpr' ::= < Expr RelExpr'
RelExpr' ::= > Expr RelExpr'
RelExpr' ::= <= Expr RelExpr'
RelExpr' ::= >= Expr RelExpr'
RelExpr' ::= ''
*/

bool Parser::RelExprPrime(unique_ptr<ParserNode>& node) {
    string temp_com = currToken().token_name;
    unique_ptr<ParserNode> exprNode, relExprPrimeNode;

    // Verificamos si hay un operador relacional y una expresión
    if ((nonTerminal("TOKEN_<") || nonTerminal("TOKEN_>") || nonTerminal("TOKEN_<=") || nonTerminal("TOKEN_>=")) &&
        Expr(exprNode) && RelExprPrime(relExprPrimeNode)) {

        // Agregamos el operador y la expresión
        node = make_unique<ParserNode>(temp_com);
        node->addChild(std::move(exprNode));  // Expresión de la derecha
        node->addChild(std::move(relExprPrimeNode));  // Recursión si hay más operadores
        return true;
    }

    return true;  // Nodo vacío si no hay operadores adicionales
}


/*
RelExpr ::= Expr RelExpr'
*/

bool Parser::RelExpr(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprNode, relExprPrimeNode;

    // Verificamos que haya una expresión y el RelExprPrime
    if (Expr(exprNode) && RelExprPrime(relExprPrimeNode)) {
        node = std::move(exprNode);  // La raíz de RelExpr será Expr

        // Si RelExprPrime tiene algún operador, lo agregamos
        if (relExprPrimeNode) {
            node->addChild(std::move(relExprPrimeNode));
        }
        return true;
    }

    fail("Error en RelExpr: se esperaba una expresión relacional con operadores de comparación (<, >, <=, >=).");
    return false;
}



/*
EqExpr' ::= == RelExpr EqExpr'
EqExpr' ::= != RelExpr EqExpr'
EqExpr' ::= ''
*/

bool Parser::EqExprprime(unique_ptr<ParserNode>& node) {
    string temp_compa = currToken().token_name;
    unique_ptr<ParserNode> relExprNode, eqExprPrimeNode;

    // Verificamos si hay un operador de igualdad y una RelExpr
    if ((nonTerminal("TOKEN_==") || nonTerminal("TOKEN_!=")) &&
        RelExpr(relExprNode) && EqExprprime(eqExprPrimeNode)) {

        // Agregamos el operador de igualdad y la expresión
        node = make_unique<ParserNode>(temp_compa);
        node->addChild(std::move(relExprNode));  // Expresión de la derecha
        node->addChild(std::move(eqExprPrimeNode));  // Recursión para más operadores
        return true;
    }

    return true;  // Nodo vacío si no hay operadores adicionales
}


/*
EqExpr ::= RelExpr EqExpr'
*/

bool Parser::EqExpr(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> relExprNode, eqExprPrimeNode;

    // Verificamos que haya una RelExpr y un EqExprPrime
    if (RelExpr(relExprNode) && EqExprprime(eqExprPrimeNode)) {
        node = std::move(relExprNode);  // La raíz de EqExpr será RelExpr

        // Si EqExprPrime tiene algún operador, lo agregamos
        if (eqExprPrimeNode) {
            node->addChild(std::move(eqExprPrimeNode));
        }
        return true;
    }

    fail("Error en EqExpr: se esperaba una expresión de igualdad con operadores '==' o '!='.");
    return false;
}


/*
AndExpr' ::= && EqExpr AndExpr'
AndExpr' ::= ''
*/

bool Parser::AndExprPrime(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> eqExprNode, andExprPrimeNode;

    // Verificamos si hay un operador lógico && y una EqExpr
    if (nonTerminal("TOKEN_&&") && EqExpr(eqExprNode) && AndExprPrime(andExprPrimeNode)) {
        node = make_unique<ParserNode>("&&");  // El operador lógico && como nodo raíz
        node->addChild(std::move(eqExprNode));  // Expresión de la derecha
        node->addChild(std::move(andExprPrimeNode));  // Recursión si hay más operadores
        return true;
    }

    return true;  // Nodo vacío si no hay más operadores
}


/*
AndExpr ::= EqExpr AndExpr'
*/

bool Parser::AndExpr(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> eqExprNode, andExprPrimeNode;

    // Verificamos si hay una EqExpr y un AndExprPrime
    if (EqExpr(eqExprNode) && AndExprPrime(andExprPrimeNode)) {
        node = std::move(eqExprNode);  // La raíz de AndExpr será EqExpr

        // Si AndExprPrime tiene algún operador &&, lo agregamos
        if (andExprPrimeNode) {
            node->addChild(std::move(andExprPrimeNode));
        }
        return true;
    }

    fail("Error en AndExpr: se esperaba una expresion AND con el operador '&&'.");
    return false;
}


/*
OrExpr' ::= || AndExpr OrExpr'
OrExpr' ::= ''
*/

bool Parser::OrExprPrime(unique_ptr<ParserNode>& node) {
    string temp_com = currToken().token_name;
    unique_ptr<ParserNode> andExprNode, orExprPrimeNode;

    // Verificamos si hay un operador || y una AndExpr
    if (nonTerminal("TOKEN_||") && AndExpr(andExprNode) && OrExprPrime(orExprPrimeNode)) {
        node = make_unique<ParserNode>(temp_com);  // El operador || como nodo raíz
        node->addChild(std::move(andExprNode));    // Expresión de la derecha
        node->addChild(std::move(orExprPrimeNode));  // Recursión si hay más operadores
        return true;
    }

    return true;  // Nodo vacío si no hay más operadores
}


/*
OrExpr ::= AndExpr OrExpr'
*/

bool Parser::OrExpr(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> andExprNode, orExprPrimeNode;

    // Verificamos que haya una AndExpr y un OrExprPrime
    if (AndExpr(andExprNode) && OrExprPrime(orExprPrimeNode)) {
        node = std::move(andExprNode);  // La raíz de OrExpr será AndExpr

        // Si OrExprPrime tiene algún operador ||, lo agregamos
        if (orExprPrimeNode) {
            node->addChild(std::move(orExprPrimeNode));
        }
        return true;
    }

    fail("Error en OrExpr: se esperaba una expresion OR con el operador '||'.");
    return false;
}


/*
AuxExpression ::= = Expression
AuxExpression ::= ''
*/

bool Parser::AuxExpression(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprNode;

    // Verificamos si hay un operador de asignación y una expresión
    if (nonTerminal("TOKEN_=") && Expression(exprNode)) {
        node = make_unique<ParserNode>("=");
        node->addChild(std::move(exprNode));
        return true;
    }

    // En lugar de retornar un nodo vacío, indicamos explícitamente que no hay asignación
    return true;
}




/*
Expression ::= OrExpr AuxExpression
*/

bool Parser::Expression(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> orExprNode, auxExprNode;

    // Verificamos si hay una OrExpr y un AuxExpression
    if (OrExpr(orExprNode) && AuxExpression(auxExprNode)) {
        node = std::move(orExprNode);  // La raíz de Expression será OrExpr

        // Si AuxExpression tiene alguna asignación, la agregamos
        if (auxExprNode) {
            node->addChild(std::move(auxExprNode));
        }
        return true;
    }

    fail("Error en Expression: se esperaba una expresion OR seguida de una posible asignación.");
    return false;
}


/*
ExprList' ::= , Expression ExprList'
ExprList' ::= ''
*/
bool Parser::ExprListPrime(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprNode, exprListPrimeNode;

    // Verificamos si hay una coma y una expresión
    if (nonTerminal("TOKEN_,") && Expression(exprNode) && ExprListPrime(exprListPrimeNode)) {
        if (!node) {
            node = make_unique<ParserNode>("ExprList");
        }

        // Agregar la expresión a la lista de expresiones
        node->addChild(std::move(exprNode));
        if (exprListPrimeNode) {
            node->addChild(std::move(exprListPrimeNode));
        }
        return true;
    }

    return true;  // Nodo vacío si no hay más expresiones
}



/*
ExprList ::= Expression ExprList'
*/

bool Parser::ExprList(unique_ptr<ParserNode>& node) {
    unique_ptr<ParserNode> exprNode, exprListPrimeNode;

    // Verificamos que haya una expresión y una lista de expresiones adicionales
    if (Expression(exprNode) && ExprListPrime(exprListPrimeNode)) {
        node = make_unique<ParserNode>("ExprList");
        node->addChild(std::move(exprNode));

        // Si hay más expresiones, las agregamos al nodo
        if (exprListPrimeNode) {
            node->addChild(std::move(exprListPrimeNode));
        }
        return true;
    }

    fail("Error en ExprList: se esperaba una expresión en la lista.");
    return false;
}


