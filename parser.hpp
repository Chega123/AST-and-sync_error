#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <memory>
using namespace std;

struct ASTNode {  //Aqui esta la base son nodos con su nombre y un vector de punteors a ASTNodes
    string token_name;
    vector<unique_ptr<ASTNode>> children;

    ASTNode(string token) : token_name(token) {}
    void addChild(unique_ptr<ASTNode> child) {
        children.push_back(move(child));
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
  bool parse(unique_ptr<ASTNode>& node);

private:
  std::vector<token> tokens;
  size_t current;

  token currToken();
  void syncToDelimiter();
  token nextToken();
  void fail(string message);
  bool nonTerminal(std::string name);
  bool Program(unique_ptr<ASTNode>& node);
  bool ProgramPrime(unique_ptr<ASTNode>& node);
  bool Declaration(unique_ptr<ASTNode>& node);
  bool VarDecl(unique_ptr<ASTNode>& node);
  bool VarDeclPrime(unique_ptr<ASTNode>& node);
  bool Type(unique_ptr<ASTNode>& node);
  bool TypePrime(unique_ptr<ASTNode>& node);
  bool BasicType(unique_ptr<ASTNode>& node);
  bool Function(unique_ptr<ASTNode>& node);
  bool Params(unique_ptr<ASTNode>& node);
  bool ParamList(unique_ptr<ASTNode>& node);
  bool ParamListPrime(unique_ptr<ASTNode>& node);
  bool CompoundStmt(unique_ptr<ASTNode>& node);
  bool StmtList(unique_ptr<ASTNode>& node);
  bool StmtListPrime(unique_ptr<ASTNode>& node);
  bool Statement(unique_ptr<ASTNode>& node);
  bool IfStmt(unique_ptr<ASTNode>& node);
  bool ForStmt(unique_ptr<ASTNode>& node);
  bool AuxIf(unique_ptr<ASTNode>& node);
  bool ReturnStmt(unique_ptr<ASTNode>& node);
  bool PrintStmt(unique_ptr<ASTNode>& node);
  bool ExprStmt(unique_ptr<ASTNode>& node);
  bool ExprList(unique_ptr<ASTNode>& node);
  bool ExprListPrime(unique_ptr<ASTNode>& node);
  bool Expression(unique_ptr<ASTNode>& node);
  bool AuxExpression(unique_ptr<ASTNode>& node);
  bool OrExpr(unique_ptr<ASTNode>& node);
  bool OrExprPrime(unique_ptr<ASTNode>& node);
  bool AndExpr(unique_ptr<ASTNode>& node);
  bool AndExprPrime(unique_ptr<ASTNode>& node);
  bool EqExpr(unique_ptr<ASTNode>& node);
  bool EqExprprime(unique_ptr<ASTNode>& node);
  bool RelExpr(unique_ptr<ASTNode>& node);
  bool RelExprPrime(unique_ptr<ASTNode>& node);
  bool Expr(unique_ptr<ASTNode>& node);
  bool ExprPrime(unique_ptr<ASTNode>& node);
  bool Term(unique_ptr<ASTNode>& node);
  bool TermPrime(unique_ptr<ASTNode>& node);
  bool Unary(unique_ptr<ASTNode>& node);
  bool Factor(unique_ptr<ASTNode>& node);
  bool FactorPrime(unique_ptr<ASTNode>& node);
  bool Primary(unique_ptr<ASTNode>& node);
  bool AuxPrimary(unique_ptr<ASTNode>& node);
};

#endif // PARSER_HPP