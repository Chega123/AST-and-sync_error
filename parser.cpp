#include "parser.hpp"
#include "iostream"
#include <memory>
#include <vector>
#include <string>
#include <unordered_set>

int errorCount = 0;
string fallo = "vacio";
int token_num = 0;


void printAST(const unique_ptr<ASTNode>& node, int level = 0) {
    if (!node) return; 
    cout << string(level * 2, ' ') << "[" << level << "] " << node->token_name << endl;
    for (size_t i = 0; i < node->children.size(); ++i) {
        printAST(node->children[i], level + 1);
    }
}

Parser::Parser(const vector<token>& tokens) : tokens(tokens), current(0) {
}

bool Parser::parse(unique_ptr<ASTNode>& root) {
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
bool Parser::TypePrime(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("TypePrime");
    unique_ptr<ASTNode> typePrimeNode;
  if (nonTerminal("TOKEN_[") && nonTerminal("TOKEN_]") && TypePrime(typePrimeNode)) {
      node->addChild(make_unique<ASTNode>("["));
      node->addChild(make_unique<ASTNode>("]"));
      node->addChild(move(typePrimeNode));
    return true;
  }
  return true;
}

/*
  BasicType -> IntType
  BasicType -> BoolType
  BasicType -> CharType
  BasicType -> StringType
  BasicType -> VoidType
*/
bool Parser::BasicType(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("BasicType");
    string name_temp = currToken().token_name;
    if (nonTerminal("TOKEN_IntType") || nonTerminal("TOKEN_BoolType") || 
        nonTerminal("TOKEN_CharType") || nonTerminal("TOKEN_StringType") || 
        nonTerminal("TOKEN_VoidType")) {
        node->addChild(make_unique<ASTNode>(name_temp));
        return true;
    }
    fail("Tipo básico no valido");
    return false ;
}

/*
  Type->BasicType Type'
*/
bool Parser::Type(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("Type");

    unique_ptr<ASTNode> basicTypeNode;
    unique_ptr<ASTNode> typePrimeNode;
    if (BasicType(basicTypeNode) && TypePrime(typePrimeNode)) {
        node->addChild(move(basicTypeNode)); 
        node->addChild(move(typePrimeNode));
        return true;
    }
    fail("No es un tipo valido");
    return false;
}

/*
  Function -> Type Identifier (Params) { StmtList }
*/
bool Parser::Function(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("Function");

    unique_ptr<ASTNode> typeNode;
    unique_ptr<ASTNode> paramsNode;
    unique_ptr<ASTNode> compoStmt;

    if (Type(typeNode) &&
        nonTerminal("TOKEN_ID") && 
        nonTerminal("TOKEN_(") && 
        Params(paramsNode) &&
        nonTerminal("TOKEN_)") && 
        CompoundStmt(compoStmt)) {
        node->addChild(move(typeNode));
        node->addChild(make_unique<ASTNode>("TOKEN_ID"));
        node->addChild(make_unique<ASTNode>("("));
        node->addChild(move(paramsNode));
        node->addChild(make_unique<ASTNode>(")"));
        node->addChild(move(compoStmt));
        return true;
    }
    fail("Función mal declarada");
    return false;
}

/*
  VarDecl' -> ;
  VarDecl' -> = Expression ;
*/
bool Parser::VarDeclPrime(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("VarDeclPrime");
    unique_ptr<ASTNode> exprNode;
    if (nonTerminal("TOKEN_;")) {
        node->addChild(make_unique<ASTNode>(";"));
        return true;
    }
    else if (nonTerminal("TOKEN_=") && Expression(exprNode) && nonTerminal("TOKEN_;")) {
        node->addChild(make_unique<ASTNode>("="));
        node->addChild(move(exprNode));
        node->addChild(make_unique<ASTNode>(";"));
        return true;
    }
    fail("Error en la declaracion de variable");
    return false;
}


/*
  VarDecl -> Type Identifier VarDecl'
*/
bool Parser::VarDecl(unique_ptr<ASTNode>& node) {
    
    node = make_unique<ASTNode>("VarDecl");
    unique_ptr<ASTNode> typeNode;
    unique_ptr<ASTNode> varDeclPrimeNode;

    if (Type(typeNode) && nonTerminal("TOKEN_ID") && VarDeclPrime(varDeclPrimeNode)) {
        node->addChild(move(typeNode));
        node->addChild(make_unique<ASTNode>("TOKEN_ID"));
        node->addChild(move(varDeclPrimeNode));
        return true;
    }
    fail("Error en declaracion de variable: se esperaba un identificador o un punto y coma");
    return false;
}

/*
  Declaration -> [ Function ]
  Declaration -> VarDecl
*/
bool Parser::Declaration(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("Declaration");
    unique_ptr<ASTNode> functionNode;
    unique_ptr<ASTNode> varDeclNode;

    if (nonTerminal("TOKEN_[") && Function(functionNode) && nonTerminal("TOKEN_]")) {
        node->addChild(make_unique<ASTNode>("[")); 
        node->addChild(std::move(functionNode));
        node->addChild(make_unique<ASTNode>("]"));
        return true;
    }
    else if (VarDecl(varDeclNode)) {
        node->addChild(std::move(varDeclNode));
        return true;
    }
    fail("Error en declaracion: se esperaba una función o declaracion de variable");
    return false;
}

/*
  Program' -> Declaration Program'
  Program' -> ε
*/
bool Parser::ProgramPrime(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("ProgramPrime");
    if (current >= tokens.size()) {
        return true; 
    }
    unique_ptr<ASTNode> declNode;
    unique_ptr<ASTNode> programPrimeNode;

    if (Declaration(declNode) && ProgramPrime(programPrimeNode)) {
        node->addChild(std::move(declNode));
        node->addChild(std::move(programPrimeNode));
        return true;
    }
    else {
        fail("Failed to parse Declaration");
        errorCount++;
        cerr << "Error at token " << token_num << ": " << fallo << endl;
        syncToDelimiter();  
        fallo = "vacio";
        return ProgramPrime(node);
    }
    return true;
}

/*
  Program -> Declaration Program'
*/
bool Parser::Program(unique_ptr<ASTNode>& root) {
    root = make_unique<ASTNode>("Program");
    unique_ptr<ASTNode> declNode;
    unique_ptr<ASTNode> ProPrimeNode;
    if (Declaration(declNode) && ProgramPrime(ProPrimeNode)) {
        root->addChild(std::move(declNode));
        root->addChild(std::move(ProPrimeNode));
        return true;
    }
    fail("Error en el programa: inicio no valido");
    return false;
}

////////////////////////////////////////////////////////////

/*
  Params -> ParamList
  Params -> ε
*/
bool Parser::Params(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("Params");
    unique_ptr<ASTNode> paramListNode;
    if (ParamList(paramListNode)) {
        node->addChild(std::move(paramListNode));
        return true;
    }
    return true;
}

/*
  ParamList -> Type Identifier ParamList'
*/
bool Parser::ParamList(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("ParamList");

    unique_ptr<ASTNode> typeNode;
    unique_ptr<ASTNode> paramListPrimeNode;
    if (Type(typeNode) && nonTerminal("TOKEN_ID") && ParamListPrime(paramListPrimeNode)) {
        node->addChild(std::move(typeNode));
        node->addChild(make_unique<ASTNode>("TOKEN_ID")); 
        node->addChild(std::move(paramListPrimeNode));
        return true;
    }
    fail("Error en lista de parámetros: se esperaba tipo e identificador");
    return false;
}

/*
  ParamList' -> , Type Identifier ParamList'
  ParamList' -> ε
*/
bool Parser::ParamListPrime(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("ParamListPrime");

    unique_ptr<ASTNode> typeNode;
    unique_ptr<ASTNode> paramListPrimeNode;
    if (nonTerminal("TOKEN_,") && Type(typeNode) && nonTerminal("TOKEN_ID") && ParamListPrime(paramListPrimeNode)) {
        node->addChild(make_unique<ASTNode>(",")); 
        node->addChild(std::move(typeNode));
        node->addChild(make_unique<ASTNode>("TOKEN_ID"));
        node->addChild(std::move(paramListPrimeNode));
        return true;
    }
    return true;
}

/*
  StmtList -> Statement StmtList'
*/
bool Parser::StmtList(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("StmtList");

    unique_ptr<ASTNode> stmtNode;
    unique_ptr<ASTNode> stmtListPrimeNode;
    if (Statement(stmtNode) && StmtListPrime(stmtListPrimeNode)) {
        node->addChild(std::move(stmtNode));
        node->addChild(std::move(stmtListPrimeNode));
        return true;
    }
    fail("Error en lista de sentencias: sentencia no válida");
    return false;
}

/*
  StmtList' -> Statement StmtList'
  StmtList' -> ε
*/
bool Parser::StmtListPrime(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("StmtListPrime");

    unique_ptr<ASTNode> stmtNode;
    unique_ptr<ASTNode> stmtListPrimeNode;
    if (Statement(stmtNode) && StmtListPrime(stmtListPrimeNode)) {
        node->addChild(std::move(stmtNode));
        node->addChild(std::move(stmtListPrimeNode));
        return true;
    }
    return true;
}

/*
  CompoundStmt -> { StmtList }
*/
bool Parser::CompoundStmt(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("CompoundStmt");

    unique_ptr<ASTNode> stmtListNode;
    if (nonTerminal("TOKEN_{") && StmtList(stmtListNode) && nonTerminal("TOKEN_}")) {
        node->addChild(make_unique<ASTNode>("{")); 
        node->addChild(std::move(stmtListNode));
        node->addChild(make_unique<ASTNode>("}"));
        return true;
    }
    fail("Error en bloque de sentencias: se esperaba '{' o '}'");
    return false;
}

/*
ExprStmt ::= Expression ;
ExprStmt ::= ;
*/
bool Parser::ExprStmt(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("ExprStmt");

    unique_ptr<ASTNode> exprNode;
    if(Expression(exprNode)&& nonTerminal("TOKEN_;")){
        node->addChild(std::move(exprNode));
        node->addChild(make_unique<ASTNode>(";"));
        return true;
    }
    else if(nonTerminal("TOKEN_;")){
        node->addChild(make_unique<ASTNode>(";"));
        return true;
    }
    fail("Error en sentencia de expresion: falta punto y coma");
    return false;
}

/*
PrintStmt -> print ( ExprList ) ;
*/
bool Parser::PrintStmt(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("PrintStmt");

    unique_ptr<ASTNode> exprListNode;
    if (nonTerminal("TOKEN_print")&&
        nonTerminal("TOKEN_(")&&
        ExprList(exprListNode)&&
        nonTerminal("TOKEN_)")&&
        nonTerminal("TOKEN_;")){
        node->addChild(make_unique<ASTNode>("print"));
        node->addChild(make_unique<ASTNode>("("));
        node->addChild(std::move(exprListNode));
        node->addChild(make_unique<ASTNode>(")"));
        node->addChild(make_unique<ASTNode>(";"));
        return true;
    }
    fail("Error en sentencia de impresión: se esperaba '(' o ')'");
    return false;
}

/*
ReturnStmt -> return Expression ;
*/
bool Parser::ReturnStmt(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("ReturnStmt");
    unique_ptr<ASTNode> exprNode;
    if (nonTerminal("TOKEN_return")&&
        Expression(exprNode)&&
        nonTerminal("TOKEN_;")){
        node->addChild(make_unique<ASTNode>("return"));
        node->addChild(std::move(exprNode));
        node->addChild(make_unique<ASTNode>(";"));
        return true;
    }
    fail("Error en sentencia de retorno: falta expresion o punto y coma");
    return false;
}

/*
ForStmt -> for ( ExprStmt Expression ; ) Statement
*/
bool Parser::ForStmt(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("ForStmt");

    unique_ptr<ASTNode> exprStmt1, exprStmt2, exprStmt3, stmtNode; // me acorde que podia hacer esto pero ya me da flojera cambiar los de arriba
    if (nonTerminal("TOKEN_for") &&
        nonTerminal("TOKEN_(") &&
        ExprStmt(exprStmt1) &&
        ExprStmt(exprStmt2) &&
        ExprStmt(exprStmt3) &&
        nonTerminal("TOKEN_)") &&
        Statement(stmtNode)) {
        node->addChild(make_unique<ASTNode>("for"));
        node->addChild(make_unique<ASTNode>("("));
        node->addChild(std::move(exprStmt1));
        node->addChild(std::move(exprStmt2));
        node->addChild(std::move(exprStmt3));
        node->addChild(make_unique<ASTNode>(")"));
        node->addChild(std::move(stmtNode));
        return true;
    }
    fail("Error en sentencia for: se esperaba ';' o ')'");
    return false;
}
/*
AuxIf -> else {Statement}
AuxIf -> ‘’
*/
bool Parser::AuxIf(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("AuxIf");

    unique_ptr<ASTNode> stmtListNode;
    if (nonTerminal("TOKEN_else") &&
        nonTerminal("TOKEN_{") &&
        StmtList(stmtListNode) &&
        nonTerminal("TOKEN_}")) {
        node->addChild(make_unique<ASTNode>("else"));
        node->addChild(make_unique<ASTNode>("{"));
        node->addChild(std::move(stmtListNode));
        node->addChild(make_unique<ASTNode>("}"));
        return true;
    }
    return true;
}

/*
IfStmt -> if ( Expression ) {Statement} AuxIf
*/
bool Parser::IfStmt(unique_ptr<ASTNode>& node) {
    node = make_unique<ASTNode>("IfStmt");

    unique_ptr<ASTNode> exprNode, stmtListNode, auxIfNode;
    if (nonTerminal("TOKEN_if") &&
        nonTerminal("TOKEN_(") &&
        Expression(exprNode) &&
        nonTerminal("TOKEN_)") &&
        nonTerminal("TOKEN_{") &&
        StmtList(stmtListNode) &&
        nonTerminal("TOKEN_}") &&
        AuxIf(auxIfNode)) {
        node->addChild(make_unique<ASTNode>("if"));
        node->addChild(make_unique<ASTNode>("("));
        node->addChild(std::move(exprNode));
        node->addChild(make_unique<ASTNode>(")"));
        node->addChild(make_unique<ASTNode>("{"));
        node->addChild(std::move(stmtListNode));
        node->addChild(make_unique<ASTNode>("}"));
        node->addChild(std::move(auxIfNode));
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
bool Parser::Statement(unique_ptr<ASTNode>& node) {
    unique_ptr<ASTNode> stmtNode;

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
bool Parser::AuxPrimary(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("AuxPrimary");

    unique_ptr<ASTNode> exprListNode;
    if (nonTerminal("TOKEN_(")  &&
        ExprList(exprListNode) &&
        nonTerminal("TOKEN_)")){
        node->addChild(make_unique<ASTNode>("("));
        node->addChild(std::move(exprListNode));
        node->addChild(make_unique<ASTNode>(")"));
        return true;
    }
    return true;
}

/*
Primary ::= Identifier auxPrimary
Primary ::= IntegerLiteral -->num
Primary ::= CharLiteral  --> 'char'
Primary ::= StringLiteral --> "text_string"
Primary ::= BooleanLiteral --> True o false
Primary ::= ( Expression )
*/

bool Parser::Primary(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("Primary");

    unique_ptr<ASTNode> auxPrimaryNode, exprNode;
    string bool_name = currToken().token_name;
    if(nonTerminal("TOKEN_ID") && AuxPrimary(auxPrimaryNode)){
        node->addChild(make_unique<ASTNode>("TOKEN_ID"));
        node->addChild(std::move(auxPrimaryNode));
        return true;
    }
    if(nonTerminal("TOKEN_Num")){
        node->addChild(make_unique<ASTNode>("TOKEN_Num"));
        return true;
    }
    if(nonTerminal("TOKEN_True")||nonTerminal("TOKEN_False")){
        node->addChild(make_unique<ASTNode>(bool_name));
        return true;
    }
    if (nonTerminal("TOKEN_Comilla_doble") &&
        nonTerminal("TOKEN_Text_string") &&
        nonTerminal("TOKEN_Comilla_doble")){
        node->addChild(make_unique<ASTNode>("\""));
        node->addChild(make_unique<ASTNode>("TOKEN_Text_string"));
        node->addChild(make_unique<ASTNode>("\""));
        return true;
    }
    if (nonTerminal("TOKEN_Comilla") &&
        nonTerminal("TOKEN_ID")&&
        nonTerminal("TOKEN_Comilla")){
        node->addChild(make_unique<ASTNode>("'"));
        node->addChild(make_unique<ASTNode>("TOKEN_ID"));
        node->addChild(make_unique<ASTNode>("'"));
        return true;
    }
    if (nonTerminal("TOKEN_(")  &&
        Expression(exprNode) &&
        nonTerminal("TOKEN_)")){
        node->addChild(make_unique<ASTNode>("("));
        node->addChild(std::move(exprNode));
        node->addChild(make_unique<ASTNode>(")"));
        return true;
    }
    fail("Error en Primary: se esperaba un identificador, número, booleano, cadena de texto, o una expresion entre paréntesis.");
    return false;
}

/*
Factor' ::= [ Expression ] Factor'
Factor' ::= ''
*/
bool Parser::FactorPrime(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("FactorPrime");

    unique_ptr<ASTNode> exprNode, factorPrimeNode;
    if (nonTerminal("TOKEN_[")&&
        Expression(exprNode)&&
        nonTerminal("TOKEN_]")&&
        FactorPrime(factorPrimeNode)){
        node->addChild(make_unique<ASTNode>("["));
        node->addChild(std::move(exprNode));     
        node->addChild(make_unique<ASTNode>("]")); 
        node->addChild(std::move(factorPrimeNode));
        return true;
    }
    return true;
}

/*
Factor ::= Primary Factor'
*/
bool Parser::Factor(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("Factor");

    unique_ptr<ASTNode> primaryNode, factorPrimeNode;
    if(Primary(primaryNode) && FactorPrime(factorPrimeNode)){
        node->addChild(std::move(primaryNode)); 
        node->addChild(std::move(factorPrimeNode));
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
bool Parser::Unary(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("Unary");

    unique_ptr<ASTNode> unaryNode,factorNode;
  /*if (nonTerminal("TOKEN_!") || nonTerminal("TOKEN_-") || Factor(factorNode)) {
    return true;
  }*/
    string temp_neg= currToken().token_name;
    if ((nonTerminal("TOKEN_!")|| nonTerminal("TOKEN_-")) && Unary(unaryNode)) {
        node->addChild(make_unique<ASTNode>(temp_neg));
        node->addChild(std::move(unaryNode));
        return true;
    }
    else if (Factor(factorNode)) {
        node->addChild(std::move(factorNode));
        return true;
    }
    fail("Error en Unary: se esperaba un operador de negación '!' o '-' o una expresion de factor.");
  return false;
}

/*
Term' ::= * Unary Term'
Term' ::= / Unary Term'
Term' ::= % Unary Term'
Term' ::= ''
*/
bool Parser::TermPrime(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("TermPrime");

    unique_ptr<ASTNode> unaryNode ,termPrimeNode;
    string temp_op= currToken().token_name;
    cout << "dsadasfasfasfsafsafsafasfsafas: " << temp_op << endl;
  if ((nonTerminal("TOKEN_*") || nonTerminal("TOKEN_/") || nonTerminal("TOKEN_%")) &&
      Unary(unaryNode) &&
      TermPrime(termPrimeNode)){
      node->addChild(make_unique<ASTNode>(currToken().token_name));
      node->addChild(std::move(unaryNode));     
      node->addChild(std::move(termPrimeNode));
    return true;
  }
  return true;
}

/*
Term ::= Unary Term'
*/
bool Parser::Term(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("Term");

    unique_ptr<ASTNode> unaryNode,termPrimeNode;
    if(Unary(unaryNode) && TermPrime(termPrimeNode)){
        node->addChild(std::move(unaryNode)); 
        node->addChild(std::move(termPrimeNode));
        return true;
    }
    fail("Error en Term: se esperaba una expresion unaria seguida de un operador de multiplicación, división o módulo.");
    return false;
}

/*
Expr' ::= + Term Expr'
Expr' ::= - Term Expr'
Expr' ::= ''
*/
bool Parser::ExprPrime(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("ExprPrime");

    unique_ptr<ASTNode> termNode,exprPrimeNode;
    string temp_sn = currToken().token_name;
    if ((nonTerminal("TOKEN_+") || nonTerminal("TOKEN_-") ) &&
        Term(termNode)&&
        ExprPrime(exprPrimeNode)){
        node->addChild(make_unique<ASTNode>(temp_sn));
        node->addChild(std::move(termNode));   
        node->addChild(std::move(exprPrimeNode));
        return true;
    }
    return true;
}

/*
Expr ::= Term Expr'
*/
bool Parser::Expr(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("Expr");

    unique_ptr<ASTNode> termNode,exprPrimeNode;
    if(Term(termNode) && ExprPrime(exprPrimeNode)){
        node->addChild(std::move(termNode));
        node->addChild(std::move(exprPrimeNode));
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
bool Parser::RelExprPrime(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("RelExprPrime");

    unique_ptr<ASTNode> exprNode , relExprPrimeNode;
    string temp_com = currToken().token_name;
    if ((nonTerminal("TOKEN_<") || nonTerminal("TOKEN_>") || nonTerminal("TOKEN_<=")||nonTerminal("TOKEN_>=")) &&
        Expr(exprNode)&&
        RelExprPrime(relExprPrimeNode)){
        node->addChild(make_unique<ASTNode>(temp_com));
        node->addChild(std::move(exprNode));
        node->addChild(std::move(relExprPrimeNode));
        return true;
    }
    return true;
}

/*
RelExpr ::= Expr RelExpr'
*/
bool Parser::RelExpr(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("RelExpr");

    unique_ptr<ASTNode> exprNode, relExprPrimeNode;
    if(Expr(exprNode)&&RelExprPrime(relExprPrimeNode)){
        node->addChild(std::move(exprNode)); 
        node->addChild(std::move(relExprPrimeNode));
        return true;
    }
    fail("Error en RelExpr: se esperaba una expresion relacional con operadores de comparación (<, >, <=, >=).");
    return false;
}

/*
EqExpr' ::= == RelExpr EqExpr'
EqExpr' ::= != RelExpr EqExpr'
EqExpr' ::= ''
*/
bool Parser::EqExprprime(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("EqExprPrime");

    unique_ptr<ASTNode> relExprNode,eqExprPrimeNode;
    string temp_compa = currToken().token_name;
    if ((nonTerminal("TOKEN_==") || nonTerminal("TOKEN_!=")) &&
        RelExpr(relExprNode)&&
        EqExprprime(eqExprPrimeNode)){
        node->addChild(make_unique<ASTNode>(temp_compa));
        node->addChild(std::move(relExprNode));    
        node->addChild(std::move(eqExprPrimeNode));
        return true;
    }
    return true;
}

/*
EqExpr ::= RelExpr EqExpr'
*/
bool Parser::EqExpr(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("EqExpr");

    unique_ptr<ASTNode> relExprNode,eqExprPrimeNode;
    if(RelExpr(relExprNode) && EqExprprime(eqExprPrimeNode)){
        node->addChild(std::move(relExprNode)); 
        node->addChild(std::move(eqExprPrimeNode));
        return true;
    }
    fail("Error en EqExpr: se esperaba una expresion de igualdad con operadores '==' o '!='.");
    return false;
}

/*
AndExpr' ::= && EqExpr AndExpr'
AndExpr' ::= ''
*/
bool Parser:: AndExprPrime(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("AndExprPrime");

    unique_ptr<ASTNode> eqExprNode,andExprPrimeNode;
    if (nonTerminal("TOKEN_&&")&&
        EqExpr(eqExprNode)&&
        AndExprPrime(andExprPrimeNode)){
        node->addChild(make_unique<ASTNode>("&&")); 
        node->addChild(std::move(eqExprNode));     
        node->addChild(std::move(andExprPrimeNode));
        return true;
    }
    return true;
}

/*
AndExpr ::= EqExpr AndExpr'
*/
bool Parser::AndExpr(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("AndExpr");

    unique_ptr<ASTNode> eqExprNode,andExprPrimeNode;
    if(EqExpr(eqExprNode)&&AndExprPrime(andExprPrimeNode)){
        node->addChild(std::move(eqExprNode)); 
        node->addChild(std::move(andExprPrimeNode));
        return true;
    }
    fail("Error en AndExpr: se esperaba una expresion AND con el operador '&&'.");
    return false;
}

/*
OrExpr' ::= || AndExpr OrExpr'
OrExpr' ::= ''
*/
bool Parser::OrExprPrime(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("OrExprPrime");

    unique_ptr<ASTNode> andExprNode,orExprPrimeNode;
    if (nonTerminal("TOKEN_||")&&
        AndExpr(andExprNode)&&
        OrExprPrime(orExprPrimeNode)){
        node->addChild(make_unique<ASTNode>("||")); 
        node->addChild(std::move(andExprNode));    
        node->addChild(std::move(orExprPrimeNode));
        return true;
    }
    return true;
}

/*
OrExpr ::= AndExpr OrExpr'
*/
bool Parser::OrExpr(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("OrExpr");

    unique_ptr<ASTNode> andExprNode,orExprPrimeNode;
    if (AndExpr(andExprNode)&& OrExprPrime(orExprPrimeNode)){
        node->addChild(std::move(andExprNode)); 
        node->addChild(std::move(orExprPrimeNode));
        return true;
    }
    fail("Error en OrExpr: se esperaba una expresion OR con el operador '||'.");
    return false;
}

/*
AuxExpression ::= = Expression
AuxExpression ::= ''
*/
bool Parser::AuxExpression(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("AuxExpression");

    unique_ptr<ASTNode> exprNode;
    if (nonTerminal("TOKEN_=") && Expression(exprNode)){
        node->addChild(make_unique<ASTNode>("="));
        node->addChild(std::move(exprNode));
        return true;
    }
    return true;
}

/*
Expression ::= OrExpr AuxExpression
*/

bool Parser::Expression(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("Expression");

    unique_ptr<ASTNode> orExprNode,auxExprNode;
    if (OrExpr(orExprNode)&&AuxExpression(auxExprNode)){
        node->addChild(std::move(orExprNode)); 
        node->addChild(std::move(auxExprNode));
        return true;
    }
    fail("Error en Expression: se esperaba una expresion OR seguida de una posible asignación.");
    return false;
}

/*
ExprList' ::= , Expression ExprList'
ExprList' ::= ''
*/
bool Parser::ExprListPrime(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("ExprListPrime");

    unique_ptr<ASTNode> exprNode,exprListPrimeNode;
    if (Expression(exprNode)&& ExprListPrime(exprListPrimeNode)){
        node->addChild(make_unique<ASTNode>(",")); // Añadir coma
        node->addChild(std::move(exprNode));       // Añadir expresión
        node->addChild(std::move(exprListPrimeNode));
        return true;
    }
    return true;
}

/*
ExprList ::= Expression ExprList'
*/
bool Parser::ExprList(unique_ptr<ASTNode>& node){
    node = make_unique<ASTNode>("ExprList");

    unique_ptr<ASTNode> exprNode,exprListPrimeNode;
    if (Expression(exprNode)&& ExprListPrime(exprListPrimeNode)){
        node->addChild(std::move(exprNode)); // Añadir expresión inicial
        node->addChild(std::move(exprListPrimeNode));
        return true;
    }
    fail("Error en ExprList: se esperaba una expresion en la lista de expresiones.");
    return false;
}
