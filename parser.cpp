#include "parser.hpp"
#include "iostream"
#include <memory>
#include <vector>
#include <string>
#include <unordered_set>

int errorCount = 0;
string fallo = "vacio";
int token_num = 0;


void printAST(const ASTNode& node, int level = 0) {
    // Imprime el nombre del nodo junto con su nivel
    cout << string(level * 2, ' ') << "[" << level << "] " << node.token_name << endl;

    // Recurre sobre los hijos del nodo
    for (const auto& child : node.children) {
        printAST(child, level + 1);
    }
}


Parser::Parser(const vector<token>& tokens) : tokens(tokens), current(0) {
}

bool Parser::parse(ASTNode& root) {
    bool success = Program(root);
    if (errorCount > 0) {
        cerr << "Parsing completed with " << errorCount << " errors." << endl;
        return false;
    }
    printAST(root);
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
    if (fallo == "vacio") { fallo = message; token_num = current; }

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
    if (nonTerminal("TOKEN_[")) {
        ASTNode arrayNode("Array");
        if (nonTerminal("TOKEN_]") && TypePrime(arrayNode)) {
            node.addChild(arrayNode); // Añadimos el nodo Array
        }
        return true;
    }
    return true; // Caso ε, sin nodos adicionales
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
    if (nonTerminal("TOKEN_IntType") || nonTerminal("TOKEN_BoolType") ||
        nonTerminal("TOKEN_CharType") || nonTerminal("TOKEN_StringType") ||
        nonTerminal("TOKEN_VoidType")) {
        node = ASTNode(name_temp);
        return true;
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
        node = ASTNode("Type");
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
    node = ASTNode("Function");

    ASTNode typeNode, paramsNode, compoStmt;
    if (Type(typeNode) &&
        nonTerminal("TOKEN_ID")) {

        std::string functionName = currToken().token_name;
        ASTNode identifierNode("Identifier");
        identifierNode.addChild(ASTNode(functionName));

        if (nonTerminal("TOKEN_(") && Params(paramsNode) &&
            nonTerminal("TOKEN_)") && CompoundStmt(compoStmt)) {

            node.addChild(typeNode);
            node.addChild(identifierNode);
            node.addChild(paramsNode);
            node.addChild(compoStmt);
            return true;
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

    if (nonTerminal("TOKEN_;")) {
        node = ASTNode("VarDecl");
        node.addChild(identifierNode);
        return true;
    }
    else if (nonTerminal("TOKEN_=") && Expression(exprNode) && nonTerminal("TOKEN_;")) {
        node = ASTNode("=");
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
    if (Type(typeNode) && nonTerminal("TOKEN_ID")) {
        ASTNode identifierNode("Identifier");
        identifierNode.addChild(ASTNode(currToken().token_name));

        ASTNode varDeclPrimeNode;
        if (VarDeclPrime(varDeclPrimeNode, identifierNode)) {
            node = ASTNode("VarDecl");
            node.addChild(typeNode);
            node.addChild(varDeclPrimeNode);
            return true;
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

    if (nonTerminal("TOKEN_[") && Function(functionNode) && nonTerminal("TOKEN_]")) {
        node = ASTNode("FunctionDeclaration");
        node.addChild(functionNode);
        return true;
    }
    else if (VarDecl(varDeclNode)) {
        node = ASTNode("VarDeclaration");
        node.addChild(varDeclNode);
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
    if (current >= tokens.size()) {
        return true;
    }

    ASTNode declNode;
    if (Declaration(declNode)) {
        node.addChild(declNode);
        return ProgramPrime(node);
    }
    else {
        fail("Error en declaración");
        errorCount++;
        cerr << "Error en el token " << token_num << ": " << fallo << endl;
        syncToDelimiter();
        return ProgramPrime(node);
    }
}


/*
  Program -> Declaration Program'
*/
bool Parser::Program(ASTNode& root) {
    root = ASTNode("Program");

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
        node = ASTNode("Params");
        node.addChild(std::move(paramListNode));
    }
    return true;  // Caso ε (sin parámetros)
}



/*
  ParamList -> Type Identifier ParamList'
*/
bool Parser::ParamList(ASTNode& node) {
    node = ASTNode("ParamList");

    ASTNode typeNode;
    if (Type(typeNode) && nonTerminal("TOKEN_ID")) {
        node.addChild(std::move(typeNode));
        node.addChild(ASTNode("Identifier"));  // Nodo para el identificador

        // Procesar más parámetros si están presentes
        while (nonTerminal("TOKEN_,") && Type(typeNode) && nonTerminal("TOKEN_ID")) {
            node.addChild(std::move(typeNode));
            node.addChild(ASTNode("Identifier"));
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
bool Parser::ParamListPrime(ASTNode& node) {
    ASTNode typeNode;
    while (nonTerminal("TOKEN_,") && Type(typeNode) && nonTerminal("TOKEN_ID")) {
        node.addChild(std::move(typeNode));
        node.addChild(ASTNode("Identifier"));
        typeNode = ASTNode();  // Reiniciar para el próximo parámetro
    }
    return true;  // Devuelve true si se procesaron todos los parámetros adicionales
}


/*
  StmtList -> Statement StmtList'
*/
bool Parser::StmtList(ASTNode& node) {
    node = ASTNode("StmtList");

    ASTNode stmtNode;
    while (Statement(stmtNode)) {
        node.addChild(std::move(stmtNode));
        stmtNode = ASTNode();  // Reiniciar para la siguiente sentencia
    }
    return true;
}


/*
  StmtList' -> Statement StmtList'
  StmtList' -> ε
*/
bool Parser::StmtListPrime(ASTNode& node) {
    ASTNode stmtNode;
    while (Statement(stmtNode)) {
        node.addChild(std::move(stmtNode));
        stmtNode = ASTNode();  // Reiniciar para la próxima sentencia
    }
    return true;
}


/*
  CompoundStmt -> { StmtList }
*/
bool Parser::CompoundStmt(ASTNode& node) {
    node = ASTNode("CompoundStmt");

    ASTNode stmtListNode;
    if (nonTerminal("TOKEN_{") && StmtList(stmtListNode) && nonTerminal("TOKEN_}")) {
        node.addChild(std::move(stmtListNode));
        return true;
    }
    fail("Error en bloque de sentencias: se esperaba '{' o '}'");
    return false;
}


/*
ExprStmt ::= Expression ;
ExprStmt ::= ;
*/
bool Parser::ExprStmt(ASTNode& node) {
    node = ASTNode("ExprStmt");

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
    node = ASTNode("PrintStmt");

    ASTNode exprListNode;
    if (nonTerminal("TOKEN_print") &&
        nonTerminal("TOKEN_(") &&
        ExprList(exprListNode) &&
        nonTerminal("TOKEN_)") &&
        nonTerminal("TOKEN_;")) {
        node.addChild(std::move(exprListNode));
        return true;
    }
    fail("Error en sentencia de impresión: se esperaba '(' o ')'");
    return false;
}


/*
ReturnStmt -> return Expression ;
*/
bool Parser::ReturnStmt(ASTNode& node) {
    node = ASTNode("ReturnStmt");

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
    node = ASTNode("ForStmt");

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
        node.addChild(std::move(stmtNode));
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
    node = ASTNode("else");

    ASTNode stmtListNode;
    if (nonTerminal("TOKEN_else") &&
        nonTerminal("TOKEN_{") &&
        StmtList(stmtListNode) &&
        nonTerminal("TOKEN_}")) {
        node.addChild(std::move(stmtListNode));
        return true;
    }
    return true;  // Caso ε, sin nodo adicional
}


/*
IfStmt -> if ( Expression ) {Statement} AuxIf
*/
bool Parser::IfStmt(ASTNode& node) {
    node = ASTNode("IfStmt");

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
        node.addChild(std::move(stmtListNode));
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
    if (nonTerminal("TOKEN_(") && ExprList(exprListNode) && nonTerminal("TOKEN_)")) {
        node = std::move(exprListNode);
        return true;
    }
    return true;  // Caso ε, retorna sin agregar nodos
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
    if (nonTerminal("TOKEN_ID")) {
        node = ASTNode("Identifier");
        ASTNode auxPrimaryNode;
        if (AuxPrimary(auxPrimaryNode)) {
            node.addChild(std::move(auxPrimaryNode));
        }
        return true;
    }
    if (nonTerminal("TOKEN_Num")) {
        node = ASTNode("NumberLiteral");
        return true;
    }
    if (nonTerminal("TOKEN_True") || nonTerminal("TOKEN_False")) {
        node = ASTNode("BooleanLiteral");
        return true;
    }
    if (nonTerminal("TOKEN_Comilla_doble") && nonTerminal("TOKEN_Text_string") && nonTerminal("TOKEN_Comilla_doble")) {
        node = ASTNode("StringLiteral");
        return true;
    }
    if (nonTerminal("TOKEN_Comilla") && nonTerminal("TOKEN_ID") && nonTerminal("TOKEN_Comilla")) {
        node = ASTNode("CharLiteral");
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
        node = ASTNode(operatorSymbol);
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
    if ((nonTerminal("TOKEN_*") || nonTerminal("TOKEN_/") || nonTerminal("TOKEN_%")) && Unary(unaryNode)) {  // Procesamos los operadores *, /, %
        node = ASTNode(currToken().token_name);
        node.addChild(std::move(unaryNode));
        if (TermPrime(termPrimeNode)) {  // Procesamos Term' recursivamente
            node.addChild(std::move(termPrimeNode));
        }
        return true;
    }
    return true;  // Caso vacío, no agregamos nada
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
    if ((nonTerminal("TOKEN_+") || nonTerminal("TOKEN_-")) && Term(termNode)) {  // Procesamos los operadores + o -
        node = ASTNode(currToken().token_name);
        node.addChild(std::move(termNode));
        if (ExprPrime(exprPrimeNode)) {  // Procesamos Expr' recursivamente
            node.addChild(std::move(exprPrimeNode));
        }
        return true;
    }
    return true;  // Caso vacío, no agregamos nada
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
    if ((nonTerminal("TOKEN_<") || nonTerminal("TOKEN_>") || nonTerminal("TOKEN_<=") || nonTerminal("TOKEN_>=")) && Expr(exprNode)) {  // Procesamos los operadores relacionales
        node = ASTNode(currToken().token_name);
        node.addChild(std::move(exprNode));
        if (RelExprPrime(relExprPrimeNode)) {  // Procesamos RelExpr' recursivamente
            node.addChild(std::move(relExprPrimeNode));
        }
        return true;
    }
    return true;  // Caso vacío, no agregamos nada
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
    if ((nonTerminal("TOKEN_==") || nonTerminal("TOKEN_!=")) && RelExpr(relExprNode)) {  // Procesamos los operadores de igualdad
        node = ASTNode(currToken().token_name);
        node.addChild(std::move(relExprNode));
        if (EqExprprime(eqExprPrimeNode)) {  // Procesamos EqExpr' recursivamente
            node.addChild(std::move(eqExprPrimeNode));
        }
        return true;
    }
    return true;  // Caso vacío, no agregamos nada
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
    if (nonTerminal("TOKEN_&&") && EqExpr(eqExprNode)) {  // Procesamos el operador && con EqExpr
        node = ASTNode("&&");
        node.addChild(std::move(eqExprNode));
        if (AndExprPrime(andExprPrimeNode)) {  // Procesamos AndExpr' recursivamente
            node.addChild(std::move(andExprPrimeNode));
        }
        return true;
    }
    return true;  // Caso vacío, no agregamos nada
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
    if (nonTerminal("TOKEN_||") && AndExpr(andExprNode)) {  // Procesamos el operador || con AndExpr
        node = ASTNode("||");
        node.addChild(std::move(andExprNode));
        if (OrExprPrime(orExprPrimeNode)) {  // Procesamos OrExpr' recursivamente
            node.addChild(std::move(orExprPrimeNode));
        }
        return true;
    }
    return true;  // Caso vacío, no agregamos nada
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
        node = ASTNode("=");
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
        node = ASTNode("Expression");
        node.addChild(std::move(orExprNode));
        if (!auxExprNode.isEmpty()) {
            node.addChild(std::move(auxExprNode));
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
bool Parser::ExprListPrime(ASTNode& node) {
    ASTNode exprNode, exprListPrimeNode;
    if (nonTerminal("TOKEN_,") && Expression(exprNode) && ExprListPrime(exprListPrimeNode)) {
        node = ASTNode( ",");
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
        node = ASTNode("ExprList");
        node.addChild(std::move(exprNode));
        if (!exprListPrimeNode.isEmpty()) {
            node.addChild(std::move(exprListPrimeNode));
        }
        return true;
    }
    fail("Error en ExprList: se esperaba una expresion en la lista de expresiones.");
    return false;
}
