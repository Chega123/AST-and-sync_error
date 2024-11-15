#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <memory>
using namespace std;

struct ASTNode {
    std::string token_name;
    std::vector<ASTNode> children;

    // Constructor por defecto que inicializa token_name vacío
    ASTNode() : token_name("") {}

    // Constructor que acepta un nombre de token
    ASTNode(std::string token) : token_name(std::move(token)) {}

    void addChild(ASTNode child) {
        children.push_back(std::move(child));
    }

    // Función isEmpty para verificar si el nodo tiene hijos
    bool isEmpty() const {
        return children.empty();  // Retorna true si no tiene hijos
    }
};




struct token {
    string token_name;
    string text;
    bool errorCount;
    int line;
    int col;
    token(string N_, string T_, int L_, int C_) { token_name = N_; text = T_; line = L_; col = C_; }
    token(string N_) : token_name(N_), text("test") {}
};

class Parser {
public:
    Parser(const std::vector<token>& tokens);
    bool parse(ASTNode& node);

private:
    std::vector<token> tokens;
    size_t current;

    token currToken();
    void syncToDelimiter();
    token nextToken();
    void fail(string message);
    bool nonTerminal(std::string name);
    bool Program(ASTNode& node);
    bool ProgramPrime(ASTNode& node);
    bool Declaration(ASTNode& node);
    bool VarDecl(ASTNode& node);
    bool VarDeclPrime(ASTNode& node, ASTNode identifierNode);
    bool Type(ASTNode& node);
    bool TypePrime(ASTNode& node);
    bool BasicType(ASTNode& node);
    bool Function(ASTNode& node);
    bool Params(ASTNode& node);
    bool ParamList(ASTNode& node);
    bool ParamListPrime(ASTNode& node);
    bool CompoundStmt(ASTNode& node);
    bool StmtList(ASTNode& node);
    bool StmtListPrime(ASTNode& node);
    bool Statement(ASTNode& node);
    bool IfStmt(ASTNode& node);
    bool ForStmt(ASTNode& node);
    bool AuxIf(ASTNode& node);
    bool ReturnStmt(ASTNode& node);
    bool PrintStmt(ASTNode& node);
    bool ExprStmt(ASTNode& node);
    bool ExprList(ASTNode& node);
    bool ExprListPrime(ASTNode& node);
    bool Expression(ASTNode& node);
    bool AuxExpression(ASTNode& node);
    bool OrExpr(ASTNode& node);
    bool OrExprPrime(ASTNode& node);
    bool AndExpr(ASTNode& node);
    bool AndExprPrime(ASTNode& node);
    bool EqExpr(ASTNode& node);
    bool EqExprprime(ASTNode& node);
    bool RelExpr(ASTNode& node);
    bool RelExprPrime(ASTNode& node);
    bool Expr(ASTNode& node);
    bool ExprPrime(ASTNode& node);
    bool Term(ASTNode& node);
    bool TermPrime(ASTNode& node);
    bool Unary(ASTNode& node);
    bool Factor(ASTNode& node);
    bool FactorPrime(ASTNode& node);
    bool Primary(ASTNode& node);
    bool AuxPrimary(ASTNode& node);
};

#endif // PARSER_HPP