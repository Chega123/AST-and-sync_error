#include "parser.hpp"
#include "iostream"
#include <memory>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <fstream> 

unordered_map<std::string, std::string> tokenMap = {
        {"TOKEN_IntType", "int"},
        {"TOKEN_BoolType", "bool"},
        {"TOKEN_CharType", "char"},
        {"TOKEN_StringType", "string"},
        {"TOKEN_VoidType", "void"}
    };


string fallo = "vacio";
int token_num(0), errorCount(0);

void printAST(const ASTNode& node, int level = 0) {
  // Imprime el nombre del nodo junto con su nivel
  cout << string(level * 2, ' ') << "[" << level << "] " << node.token_name << endl;

  // Recurre sobre los hijos del nodo
  for (const auto& child : node.children) {
    printAST(child, level + 1);
  }
}

void exportToDot(const ASTNode& node, ofstream& dotFile, int& nodeId, int parentId = -1) {
  int currentId = nodeId++;
  dotFile << "    node" << currentId << " [label=\"" << node.token_name << "\"];" << endl;

  if (parentId != -1) {
    dotFile << "    node" << parentId << " -> node" << currentId << ";" << endl;
  }

  for (const auto& child : node.children) {
    exportToDot(child, dotFile, nodeId, currentId);
  }
}

void generateDotFile(const ASTNode& root, const string& filename) {
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

Parser::Parser(const vector<token>& tokens) : tokens(tokens), current(0) { }

bool Parser::parse(ASTNode& root) {
  bool success = Program(root);
  if (errorCount > 0) {
    cerr << "Parsing completed with " << errorCount << " errors." << endl;
    return false;
  }
  printAST(root);
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
  if (fallo == "vacio") { 
    fallo = message; 
    token_num = current;
    cerr << "Linea " << tokens[token_num].line << ", Col " << tokens[token_num].col<< ": " << fallo << endl;
  }
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
bool Parser::TypePrime(ASTNode& node) {
  bool assigned=false;
  if (nonTerminal("TOKEN_[")) {
    if (!nonTerminal("TOKEN_]")) {
      fail("Falta el token de cierre ']'");
      return false;
    }

    // Crear un nuevo nodo para este nivel de array
    ASTNode arrayNode("Array", currToken().text);
    assigned=true;
    // Nodo temporal para los niveles restantes de TypePrime
    ASTNode childNode;

    // Procesar recursivamente los niveles adicionales
    if (TypePrime(childNode)) {
    // Mover el nodo resultante como hijo del nodo actual
      arrayNode.addChild(std::move(childNode));
      assigned=true;
    }

  // Mover el nuevo nodo a `node` para encadenarlo
  node = std::move(arrayNode);

  return true;
  }
  return assigned; // Epsilon (vacío)
}

/*
  BasicType -> IntType
  BasicType -> BoolType
  BasicType -> CharType
  BasicType -> StringType
  BasicType -> VoidType
*/
bool Parser::BasicType(ASTNode& node) {
    std::string name_temp = currToken().token_name;

    // Verificar si el token es un tipo básico válido
    if (nonTerminal("TOKEN_IntType") || nonTerminal("TOKEN_BoolType") ||
        nonTerminal("TOKEN_CharType") || nonTerminal("TOKEN_StringType") ||
        nonTerminal("TOKEN_VoidType")) {

        // Buscar el nombre asociado en el mapa
        auto it = tokenMap.find(name_temp);
        if (it != tokenMap.end()) {
            node = ASTNode(it->second, currToken().text); // Usar la palabra asociada
            return true;
        }
    }
    fail("Tipo básico no válido");
    return false;
    
}
/*
  Type->BasicType Type'
*/
bool Parser::Type(ASTNode& node) {
  ASTNode basicTypeNode;
  if (BasicType(basicTypeNode)) {
    node = ASTNode("Type",currToken().text);
    node.addChild(basicTypeNode);

    ASTNode typePrimeNode;
    if (TypePrime(typePrimeNode) && !typePrimeNode.children.empty()) {
      node.addChild(typePrimeNode);
    }
    return true;
  }
  fail("No es un tipo válido");
  return false;
}

/*
  Function -> Type Identifier (Params) { StmtList }
*/
bool Parser::Function(ASTNode& node) {
  node = ASTNode("Function",currToken().text);

  ASTNode typeNode, paramsNode, compoStmt;
  if (Type(typeNode) ) {
    std::string txt = currToken().text; 
    if(nonTerminal("TOKEN_ID")){
        std::string functionName = currToken().token_name;
        ASTNode identifierNode(txt,txt);
      //  identifierNode.addChild(ASTNode(functionName,currToken().text));

        if (nonTerminal("TOKEN_(") && Params(paramsNode) &&
            nonTerminal("TOKEN_)") && CompoundStmt(compoStmt)) {

        node.addChild(typeNode);
        node.addChild(identifierNode);
        if(paramsNode.word!=""){node.addChild(paramsNode, "Params");}
        node.addChild(compoStmt,"Stmtlist");
        return true;
        }
    }
  }
  fail("Función mal declarada");
  return false;
}

/*
  VarDecl' -> ;
  VarDecl' -> = Expression ;
*/
bool Parser::VarDeclPrime(ASTNode& node, ASTNode identifierNode) {
    ASTNode exprNode;

    // Caso con solo el punto y coma
    if (nonTerminal("TOKEN_;")) {
        node = ASTNode("VarDecl",currToken().text);  // Aquí agregamos un nodo solo si es necesario
        node.addChild(identifierNode);
        return true;
    }
    // Caso con asignación
    else if (nonTerminal("TOKEN_=") && Expression(exprNode) && nonTerminal("TOKEN_;")) {
        node = ASTNode("=",currToken().text);
        node.addChild(identifierNode);
        node.addChild(exprNode);
        return true;
    }

    fail("Error en la declaración de variable");
    return false;
}





/*
  VarDecl -> Type Identifier VarDecl'
*/
bool Parser::VarDecl(ASTNode& node) {
    ASTNode typeNode;
    if (Type(typeNode)) {
        std::string txt = currToken().text; 
        if(nonTerminal("TOKEN_ID")){
        ASTNode identifierNode(txt,txt);
        //identifierNode.addChild(ASTNode(currToken().token_name));

        ASTNode varDeclPrimeNode;
        if (VarDeclPrime(varDeclPrimeNode, identifierNode)) {
            node = ASTNode("VarDecl",currToken().text);
            node.addChild(typeNode);
            node.addChild(varDeclPrimeNode); // Solo se agrega si VarDeclPrime es válido
            return true;
        }
        }
    }

    fail("Error en declaración de variable");
    return false;
}




/*
  Declaration -> [ Function ]
  Declaration -> VarDecl
*/
bool Parser::Declaration(ASTNode& node) {
    ASTNode functionNode, varDeclNode;

    // Si es una función, procesarla
    if (nonTerminal("TOKEN_[") && Function(functionNode) && nonTerminal("TOKEN_]")) {
        node = std::move(functionNode);  // Asignamos directamente el nodo de la función
        return true;
    }
    // Si es una declaración de variable, procesarla
    else if (VarDecl(varDeclNode)) {
        node = std::move(varDeclNode);  // Asignamos directamente el nodo de la variable
        return true;
    }

    fail("Error en declaración");
    return false;
}



/*
  Program' -> Declaration Program'
  Program' -> ε
*/
bool Parser::ProgramPrime(ASTNode& node) {
    fallo = "vacio";
    if (current >= tokens.size()) {
        return true;
    }

    ASTNode declNode;
    if (Declaration(declNode)) {
        node.addChild(declNode);
        //fallo = "vacio";
        return ProgramPrime(node);
    }
    else {
        fail("Error en declaración");
        errorCount++;
        //cerr << "Linea " << tokens[current].line << ", Col " << tokens[current].col<< ": " << fallo << endl;
        syncToDelimiter();
        return ProgramPrime(node);
    }
}


/*
  Program -> Declaration Program'
*/
bool Parser::Program(ASTNode& root) {
    root = ASTNode("Program",currToken().text);

    ASTNode declarationNode;
    if (Declaration(declarationNode)) {
        root.addChild(declarationNode);
        return ProgramPrime(root);
    }
    fail("Error en el programa");
    return false;
}


////////////////////////////////////////////////////////////

/*
  Params -> ParamList
  Params -> ε
*/
bool Parser::Params(ASTNode& node) {
    ASTNode paramListNode;
    if (ParamList(paramListNode)) {
        node = ASTNode("Params",currToken().text);
        node.addChild(std::move(paramListNode));
    }
    return true;  // Caso ε (sin parámetros)
}



/*
  ParamList -> Type Identifier ParamList'
*/
bool Parser::ParamList(ASTNode& node) {
    node = ASTNode("ParamList",currToken().text);

    ASTNode typeNode;
    if (Type(typeNode)) {
        std::string txt = currToken().text; 
        if(nonTerminal("TOKEN_ID")){
            node.addChild(std::move(typeNode));
            node.addChild(ASTNode(txt,txt));  // Nodo para el identificador

            // Procesar más parámetros si están presentes
            while (nonTerminal("TOKEN_,") && Type(typeNode)) {
                std::string txt = currToken().text; 
                if(nonTerminal("TOKEN_ID")){
                    node.addChild(std::move(typeNode));
                    node.addChild(ASTNode(txt,currToken().text));
                }
            }
            return true;
        }
    }
    fail("Error en lista de parámetros: se esperaba tipo e identificador");
    return false;
}



/*
  ParamList' -> , Type Identifier ParamList'
  ParamList' -> ε
*/
bool Parser::ParamListPrime(ASTNode& node) {
    ASTNode typeNode;
    while (nonTerminal("TOKEN_,") && Type(typeNode) && nonTerminal("TOKEN_ID")) {
        node.addChild(std::move(typeNode));
        node.addChild(ASTNode("Identifier",currToken().text));
        typeNode = ASTNode();  // Reiniciar para el próximo parámetro
    }
    return true;  // Devuelve true si se procesaron todos los parámetros adicionales
}


/*
  StmtList -> Statement StmtList'
*/
bool Parser::StmtList(ASTNode& node) {
    bool hasStatements = false;
    ASTNode node_stmt;

    // Procesar cada sentencia sin crear un nodo StmtList
    while (Statement(node_stmt)) {
        node.addChild(std::move(node_stmt));  // Solo agregar las sentencias
        hasStatements = true;
    }
    
    return hasStatements;  // Solo retornamos true si encontramos al menos una sentencia
}


/*
  StmtList' -> Statement StmtList'
  StmtList' -> ε
*/
bool Parser::StmtListPrime(ASTNode& node) {
    bool hasStatements = false;
    ASTNode node_stmt;

    // Procesar sentencias adicionales si las hay
    while (Statement(node_stmt)) {
        node.addChild(std::move(node_stmt));  // Solo agregar las sentencias adicionales
        hasStatements = true;
    }
    
    return hasStatements;  // Solo retornamos true si encontramos al menos una sentencia
}




/*
  CompoundStmt -> { StmtList }
*/
bool Parser::CompoundStmt(ASTNode& node) {
    if (nonTerminal("TOKEN_{")) {
        // Procesar la lista de sentencias dentro de las llaves
        ASTNode stmtListNode;
        if (StmtList(stmtListNode) && nonTerminal("TOKEN_}")) {
            // Solo añadimos el contenido dentro de las llaves sin crear un nodo CompoundStmt
            node = std::move(stmtListNode);  // El nodo recibido contiene ya las sentencias
            return true;
        }
        fail("Error en bloque de sentencias: se esperaba '}'");
        return false;
    }
    
    fail("Error en bloque de sentencias: se esperaba '{'");
    return false;
}




/*
ExprStmt ::= Expression ;
ExprStmt ::= ;
*/
bool Parser::ExprStmt(ASTNode& node) {
    node = ASTNode("ExprStmt",currToken().text);

    ASTNode exprNode;
    if (Expression(exprNode)) {
        node.addChild(std::move(exprNode));
    }
    if (nonTerminal("TOKEN_;")) {
        return true;
    }

    fail("Error en sentencia de expresión: falta punto y coma");
    return false;
}


/*
PrintStmt -> print ( ExprList ) ;
*/
bool Parser::PrintStmt(ASTNode& node) {
    // Nodo raíz de la sentencia de impresión
    ASTNode printStmtNode("PrintStmt");

    ASTNode exprListNode;
    // Verificamos si la estructura completa de la sentencia es válida
    if (nonTerminal("TOKEN_print") &&
        nonTerminal("TOKEN_(") &&
        ExprList(exprListNode) &&
        nonTerminal("TOKEN_)") &&
        nonTerminal("TOKEN_;")) {
        // Si es válida, agregamos el nodo ExprList como hijo del nodo PrintStmt
        printStmtNode.addChild(std::move(exprListNode));
        node = std::move(printStmtNode);  // Movemos el nodo completo a 'node'
        return true;
    }

    fail("Error en sentencia de impresión: se esperaba '(' o ')'");
    return false;
}



/*
ReturnStmt -> return Expression ;
*/
bool Parser::ReturnStmt(ASTNode& node) {
    node = ASTNode("ReturnStmt",currToken().text);

    ASTNode exprNode;
    if (nonTerminal("TOKEN_return") &&
        Expression(exprNode) &&
        nonTerminal("TOKEN_;")) {
        node.addChild(std::move(exprNode));
        return true;
    }
    fail("Error en sentencia de retorno: falta expresión o punto y coma");
    return false;
}


/*
ForStmt -> for ( ExprStmt Expression ; ) Statement
*/
bool Parser::ForStmt(ASTNode& node) {
    node = ASTNode("ForStmt",currToken().text);

    ASTNode exprStmt1, exprStmt2, exprStmt3, stmtNode;
    if (nonTerminal("TOKEN_for") &&
        nonTerminal("TOKEN_(") &&
        ExprStmt(exprStmt1) &&
        ExprStmt(exprStmt2) &&
        ExprStmt(exprStmt3) &&
        nonTerminal("TOKEN_)") &&
        Statement(stmtNode)) {
        node.addChild(std::move(exprStmt1));
        node.addChild(std::move(exprStmt2));
        node.addChild(std::move(exprStmt3));
        node.addChild(std::move(stmtNode),"StmtListFor");
        return true;
    }
    fail("Error en sentencia for: se esperaba ';' o ')'");
    return false;
}

/*
AuxIf -> else {Statement}
AuxIf -> ‘’
*/
bool Parser::AuxIf(ASTNode& node) {
    node = ASTNode("else",currToken().text);

    ASTNode stmtListNode;
    if (nonTerminal("TOKEN_else") &&
        nonTerminal("TOKEN_{") &&
        StmtList(stmtListNode) &&
        nonTerminal("TOKEN_}")) {
        node.addChild(std::move(stmtListNode),"StmtElse");
        return true;
    }
    return true;  // Caso ε, sin nodo adicional
}


/*
IfStmt -> if ( Expression ) {Statement} AuxIf
*/
bool Parser::IfStmt(ASTNode& node) {
    node = ASTNode("IfStmt",currToken().text);

    ASTNode exprNode, stmtListNode, auxIfNode;
    if (nonTerminal("TOKEN_if") &&
        nonTerminal("TOKEN_(") &&
        Expression(exprNode) &&
        nonTerminal("TOKEN_)") &&
        nonTerminal("TOKEN_{") &&
        StmtList(stmtListNode) &&
        nonTerminal("TOKEN_}") &&
        AuxIf(auxIfNode)) {
        node.addChild(std::move(exprNode));
        node.addChild(std::move(stmtListNode),"Stmtlist");
        if (!auxIfNode.isEmpty()) {
            node.addChild(std::move(auxIfNode));
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
bool Parser::Statement(ASTNode& node) {
    ASTNode stmtNode;

    if (VarDecl(stmtNode) || IfStmt(stmtNode) || ForStmt(stmtNode) || ReturnStmt(stmtNode) ||
        ExprStmt(stmtNode) || CompoundStmt(stmtNode) || PrintStmt(stmtNode)) {

        node = std::move(stmtNode);
        return true;
    }

    fail("Error en declaración: no se encontró una declaración válida");
    return false;
}


/*
auxPrimary ::= ( ExprList )
auxPrimary ::= ''
*/
bool Parser::AuxPrimary(ASTNode& node) {
    ASTNode exprListNode;
    bool entro=false;
    if (nonTerminal("TOKEN_(") && ExprList(exprListNode) && nonTerminal("TOKEN_)")) {
        node = std::move(exprListNode);
        bool entro=true;
        return entro;
    }
    return entro;  // Caso ε, retorna sin agregar nodos
}


/*
Primary ::= Identifier auxPrimary
Primary ::= IntegerLiteral -->num
Primary ::= CharLiteral  --> 'char'
Primary ::= StringLiteral --> "text_string"
Primary ::= BooleanLiteral --> True o false
Primary ::= ( Expression )
*/

bool Parser::Primary(ASTNode& node) {
    std::string txt = tokens[current].text;

    string saveBool=tokens[current].token_name;
    unordered_map<std::string, std::string> tokenMap_pri = {
        {"TOKEN_True", "true"},
        {"TOKEN_False", "false"},
    };
    if (nonTerminal("TOKEN_ID")) {
        node = ASTNode(txt,txt);  // Nodo para el identificador
        //node.addChild(ASTNode(currToken().token_name));  // Usamos el nombre del identificador
        ASTNode auxPrimaryNode;
        if (AuxPrimary(auxPrimaryNode)) {
            node.addChild(std::move(auxPrimaryNode));
        }
        return true;
    }
    if (nonTerminal("TOKEN_Num")) {
        node = ASTNode(txt,txt);
        return true;
    }
    if (nonTerminal("TOKEN_True") || nonTerminal("TOKEN_False")) {
        txt=tokenMap_pri[saveBool];
        node = ASTNode(txt,txt);
        return true;
    }
    if (nonTerminal("TOKEN_Comilla_doble") && nonTerminal("TOKEN_Text_string") && nonTerminal("TOKEN_Comilla_doble")) {
        node = ASTNode(txt,txt);
        return true;
    }
    if (nonTerminal("TOKEN_Comilla") && nonTerminal("TOKEN_ID") && nonTerminal("TOKEN_Comilla")) {
        node = ASTNode(txt,txt);
        return true;
    }
    if (nonTerminal("TOKEN_(") && Expression(node) && nonTerminal("TOKEN_)")) {
        return true;
    }
    fail("Error en Primary: se esperaba un identificador, número, booleano, cadena de texto, o una expresion entre paréntesis.");
    return false;
}


/*
Factor' ::= [ Expression ] Factor'
Factor' ::= ''
*/
bool Parser::FactorPrime(ASTNode& node) {
    ASTNode exprNode, factorPrimeNode;
    while (nonTerminal("TOKEN_[") && Expression(exprNode) && nonTerminal("TOKEN_]")) {
        node.addChild(std::move(exprNode));  // Agregar la expresión directamente
        exprNode = ASTNode();  // Reiniciar para el siguiente índice
    }
    return true;
}


/*
Factor ::= Primary Factor'
*/
bool Parser::Factor(ASTNode& node) {
    ASTNode primaryNode;
    if (Primary(primaryNode)) {  // Procesamos Primary
        node = std::move(primaryNode);  // Usamos directamente Primary
        FactorPrime(node);  // Intentamos procesar Factor' si es necesario
        return true;
    }
    fail("Error en Factor: se esperaba una expresión primaria.");
    return false;
}


/*
Unary ::= ! Unary
Unary ::= - Unary
Unary ::= Factor
*/
bool Parser::Unary(ASTNode& node) {
    string operatorSymbol;
    if (nonTerminal("TOKEN_!") || nonTerminal("TOKEN_-")) {
        operatorSymbol = currToken().token_name;
        node = ASTNode(operatorSymbol,currToken().text);
        ASTNode unaryNode;
        if (Unary(unaryNode)) {
            node.addChild(std::move(unaryNode));
            return true;
        }
    }
    else if (Factor(node)) {
        return true;
    }
    fail("Error en Unary: se esperaba un operador de negación '!' o '-' o una expresión de factor.");
    return false;
}


/*
Term' ::= * Unary Term'
Term' ::= / Unary Term'
Term' ::= % Unary Term'
Term' ::= ''
*/
bool Parser::TermPrime(ASTNode& node) {
    ASTNode unaryNode, termPrimeNode;
    ASTNode tempo_name = node;
    bool addedSomething = false;
    string temp=currToken().token_name;
    if ((nonTerminal("TOKEN_*") || nonTerminal("TOKEN_/") || nonTerminal("TOKEN_%")) && Unary(unaryNode)) {  // Procesamos los operadores *, /, %
        node = ASTNode(temp,currToken().text);
        addedSomething = true;
        node.addChild(std::move(tempo_name));
        node.addChild(std::move(unaryNode));
        if (TermPrime(termPrimeNode)) {  // Procesamos Term' recursivamente
            node.addChild(std::move(termPrimeNode));
            addedSomething = true;
        }
        return true;
    }
    return addedSomething;  // Caso vacío, no agregamos nada
}


/*
Term ::= Unary Term'
*/
bool Parser::Term(ASTNode& node) {
    ASTNode unaryNode;
    if (Unary(unaryNode)) {  // Procesamos Unary
        node = std::move(unaryNode);  // Usamos directamente Unary
        TermPrime(node);  // Intentamos procesar Term' si es necesario
        return true;
    }
    fail("Error en Term: se esperaba una expresión unaria.");
    return false;
}


/*
Expr' ::= + Term Expr'
Expr' ::= - Term Expr'
Expr' ::= ''
*/
bool Parser::ExprPrime(ASTNode& node) {
    ASTNode termNode, exprPrimeNode;

    ASTNode tempo_name = node;
    bool addedSomething = false;
    unordered_map<std::string, std::string> tokenMap_expr = {
        {"TOKEN_+", "+"},
        {"TOKEN_-", "-"},
    };
    std::string name_temp = tokenMap_expr[currToken().token_name];
    if ((nonTerminal("TOKEN_+") || nonTerminal("TOKEN_-")) && Term(termNode)) {  // Procesamos + o -
        node = ASTNode(name_temp,currToken().text);
        addedSomething = true;
        node.addChild(std::move(tempo_name));// El nodo para el operador + o -
        node.addChild(std::move(termNode));  // Añadimos el término que contiene el identificador
        if (ExprPrime(exprPrimeNode)) {  // Procesamos Expr' recursivamente
            node.addChild(std::move(exprPrimeNode));
            addedSomething = true;
        }
        return true;
    }
    return addedSomething;  // Caso vacío, no agregamos nada
}



/*
Expr ::= Term Expr'
*/
bool Parser::Expr(ASTNode& node) {
    ASTNode termNode;
    if (Term(termNode)) {  // Procesamos la expresión Term
        node = std::move(termNode);  // Usamos directamente Term
        ExprPrime(node);  // Intentamos procesar Expr' si es necesario
        return true;
    }
    fail("Error en Expr: se esperaba una expresión.");
    return false;
}


/*
RelExpr' ::= < Expr RelExpr'
RelExpr' ::= > Expr RelExpr'
RelExpr' ::= <= Expr RelExpr'
RelExpr' ::= >= Expr RelExpr'
RelExpr' ::= ''
*/
bool Parser::RelExprPrime(ASTNode& node) {
    ASTNode exprNode, relExprPrimeNode;
    ASTNode tempo_name = node;
    bool addedSomething = false;
    

    unordered_map<std::string, std::string> tokenMap_rel = {
        {"TOKEN_<", "<"},
        {"TOKEN_>", ">"},
        {"TOKEN_<=", "<="},
        {"TOKEN_>=", ">="},
    };
    string temp=tokenMap_rel[currToken().token_name];
    if ((nonTerminal("TOKEN_<") || nonTerminal("TOKEN_>") || nonTerminal("TOKEN_<=") || nonTerminal("TOKEN_>=")) && Expr(exprNode)) {  // Procesamos los operadores relacionales
        node = ASTNode(temp,currToken().text);
        addedSomething = true;
        node.addChild(std::move(tempo_name));
        node.addChild(std::move(exprNode));
        if (RelExprPrime(relExprPrimeNode)) {  // Procesamos RelExpr' recursivamente
            node.addChild(std::move(relExprPrimeNode));
            addedSomething = true;
        }
        return true;
    }
    return addedSomething;  // Caso vacío, no agregamos nada
}


/*
RelExpr ::= Expr RelExpr'
*/
bool Parser::RelExpr(ASTNode& node) {
    ASTNode exprNode;
    if (Expr(exprNode)) {  // Procesamos Expr
        node = std::move(exprNode);  // Usamos directamente Expr
        RelExprPrime(node);  // Intentamos procesar RelExpr' si es necesario
        return true;
    }
    fail("Error en RelExpr: se esperaba una expresión relacional.");
    return false;
}


/*
EqExpr' ::= == RelExpr EqExpr'
EqExpr' ::= != RelExpr EqExpr'
EqExpr' ::= ''
*/
bool Parser::EqExprprime(ASTNode& node) {
    ASTNode relExprNode, eqExprPrimeNode;
    ASTNode tempo_name = node;
    bool addedSomething = false;
    

    unordered_map<std::string, std::string> tokenMap_expr = {
        {"TOKEN_==", "=="},
        {"TOKEN_!=", "!="},
    };
    string temp=tokenMap_expr[currToken().token_name];
    if ((nonTerminal("TOKEN_==") || nonTerminal("TOKEN_!=")) && RelExpr(relExprNode)) {  // Procesamos los operadores de igualdad
        node = ASTNode(temp,currToken().text);
        addedSomething = true;
        node.addChild(std::move(tempo_name));
        node.addChild(std::move(relExprNode));
        if (EqExprprime(eqExprPrimeNode)) {  // Procesamos EqExpr' recursivamente
            node.addChild(std::move(eqExprPrimeNode));
            addedSomething = true;
        }
        return true;
    }
    return addedSomething;  // Caso vacío, no agregamos nada
}


/*
EqExpr ::= RelExpr EqExpr'
*/
bool Parser::EqExpr(ASTNode& node) {
    ASTNode relExprNode;
    if (RelExpr(relExprNode)) {  // Procesamos RelExpr
        node = std::move(relExprNode);  // Usamos directamente RelExpr
        EqExprprime(node);  // Intentamos procesar EqExpr' si es necesario
        return true;
    }
    fail("Error en EqExpr: se esperaba una expresión de igualdad.");
    return false;
}


/*
AndExpr' ::= && EqExpr AndExpr'
AndExpr' ::= ''
*/
bool Parser::AndExprPrime(ASTNode& node) {
    ASTNode eqExprNode, andExprPrimeNode;
    ASTNode tempo_name = node;
    bool addedSomething = false;
    if (nonTerminal("TOKEN_&&") && EqExpr(eqExprNode)) {  // Procesamos el operador && con EqExpr
        node = ASTNode("&&",currToken().text);
        addedSomething = true;
        node.addChild(std::move(tempo_name));
        node.addChild(std::move(eqExprNode));
        if (AndExprPrime(andExprPrimeNode)) {  // Procesamos AndExpr' recursivamente
            node.addChild(std::move(andExprPrimeNode));
            addedSomething = true;
        }
        return true;
    }
    return addedSomething;  // Caso vacío, no agregamos nada
}


/*
AndExpr ::= EqExpr AndExpr'
*/
bool Parser::AndExpr(ASTNode& node) {
    ASTNode eqExprNode;
    if (EqExpr(eqExprNode)) {  // Procesamos EqExpr
        node = std::move(eqExprNode);  // Usamos directamente EqExpr
        AndExprPrime(node);  // Intentamos procesar AndExpr' si es necesario
        return true;
    }
    fail("Error en AndExpr: se esperaba una expresión AND.");
    return false;
}


/*
OrExpr' ::= || AndExpr OrExpr'
OrExpr' ::= ''
*/
bool Parser::OrExprPrime(ASTNode& node) {
    ASTNode andExprNode, orExprPrimeNode;
    ASTNode tempo_name = node;
    bool addedSomething = false;
    if (nonTerminal("TOKEN_||") && AndExpr(andExprNode)) {  // Procesamos el operador || con AndExpr
        node = ASTNode("||",currToken().text);
        addedSomething = true;
        node.addChild(std::move(tempo_name));
        node.addChild(std::move(andExprNode));
        if (OrExprPrime(orExprPrimeNode)) {  // Procesamos OrExpr' recursivamente
            node.addChild(std::move(orExprPrimeNode));
            addedSomething = true;
        }
        return true;
    }
    return addedSomething;  // Caso vacío, no agregamos nada
}


/*
OrExpr ::= AndExpr OrExpr'
*/
bool Parser::OrExpr(ASTNode& node) {
    ASTNode andExprNode;
    if (AndExpr(andExprNode)) {  // Procesamos la expresión AndExpr
        node = std::move(andExprNode);  // Usamos directamente AndExpr
        OrExprPrime(node);  // Intentamos procesar OrExpr' si es necesario
        return true;
    }
    fail("Error en OrExpr: se esperaba una expresión OR.");
    return false;
}


/*
AuxExpression ::= = Expression
AuxExpression ::= ''
*/
bool Parser::AuxExpression(ASTNode& node) {
    ASTNode exprNode;
    if (nonTerminal("TOKEN_=") && Expression(exprNode)) {
        node = ASTNode("=",currToken().text);
        node.addChild(std::move(exprNode));  // Agregar directamente la expresión
        return true;
    }
    return true; // Caso ε, retorna sin agregar nodos
}


/*
Expression ::= OrExpr AuxExpression
*/

bool Parser::Expression(ASTNode& node) {
    ASTNode orExprNode, auxExprNode;

    if (OrExpr(orExprNode) && AuxExpression(auxExprNode)) {
        if (!auxExprNode.isEmpty()) {
            // Crear un nuevo nodo raíz para representar la asignación
            ASTNode newRoot = std::move(auxExprNode);

            // Primero mover el nodo `orExprNode` como hijo
            ASTNode temp = std::move(orExprNode);  // Copiar para evitar problemas al mover
            newRoot.children.insert(newRoot.children.begin(), std::move(temp));  // Insertarlo al inicio
            
            // Establecer el nodo final
            node = std::move(newRoot);
        } else {
            // Si no hay asignación, usamos directamente el nodo OR como raíz
            node = std::move(orExprNode);
        }
        return true;
    }

    fail("Error en Expression: se esperaba una expresión OR seguida de una posible asignación.");
    return false;
}







/*
ExprList' ::= , Expression ExprList'
ExprList' ::= ''
*/
bool Parser::ExprListPrime(ASTNode& node) {
    ASTNode exprNode, exprListPrimeNode;
    if (nonTerminal("TOKEN_,") && Expression(exprNode) && ExprListPrime(exprListPrimeNode)) {
        node = ASTNode( ",",currToken().text);
        node.addChild(std::move(exprNode));
        if (!exprListPrimeNode.isEmpty()) {
            node.addChild(std::move(exprListPrimeNode));
        }
        return true;
    }
    return true; // Caso ε, retorna sin agregar nodos
}


/*
ExprList ::= Expression ExprList'
*/
bool Parser::ExprList(ASTNode& node) {
    ASTNode exprNode, exprListPrimeNode;
    if (Expression(exprNode) && ExprListPrime(exprListPrimeNode)) {
        node = std::move(exprNode);
        if (!exprListPrimeNode.isEmpty()) {
            node.addChild(std::move(exprListPrimeNode));
        }
        return true;
    }
    fail("Error en ExprList: se esperaba una expresion en la lista de expresiones.");
    return false;
}