#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <memory>
using namespace std;




struct ParserNode {  //Aqui esta la base son nodos con su nombre y un vector de punteors a ASTNodes
    string token_name;
    vector<unique_ptr<ParserNode>> children;

    ParserNode(string token) : token_name(token) {}
    void addChild(unique_ptr<ParserNode> child) {
        children.push_back(move(child));
    }
    string getUniqueName(int& counter) {
        return "n" + to_string(counter++);
    }
};

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
  bool parse(unique_ptr<ParserNode>& node);

private:
  std::vector<token> tokens;
  size_t current;

  token currToken();
  void syncToDelimiter();
  token nextToken();
  void fail(string message);
  bool nonTerminal(std::string name);
  bool Program(unique_ptr<ParserNode>& node);
  bool ProgramPrime(unique_ptr<ParserNode>& node);
  bool Declaration(unique_ptr<ParserNode>& node);
  bool VarDecl(unique_ptr<ParserNode>& node);
  bool VarDeclPrime(unique_ptr<ParserNode>& node, unique_ptr<ParserNode> idNode);
  bool Type(unique_ptr<ParserNode>& node);
  bool TypePrime(unique_ptr<ParserNode>& node);
  bool BasicType(unique_ptr<ParserNode>& node);
  bool Function(unique_ptr<ParserNode>& node);
  bool Params(unique_ptr<ParserNode>& node);
  bool ParamList(unique_ptr<ParserNode>& node);
  bool ParamListPrime(unique_ptr<ParserNode>& node);
  bool CompoundStmt(unique_ptr<ParserNode>& node);
  bool StmtList(unique_ptr<ParserNode>& node);
  bool StmtListPrime(unique_ptr<ParserNode>& node);
  bool Statement(unique_ptr<ParserNode>& node);
  bool IfStmt(unique_ptr<ParserNode>& node);
  bool ForStmt(unique_ptr<ParserNode>& node);
  bool AuxIf(unique_ptr<ParserNode>& node);
  bool ReturnStmt(unique_ptr<ParserNode>& node);
  bool PrintStmt(unique_ptr<ParserNode>& node);
  bool ExprStmt(unique_ptr<ParserNode>& node);
  bool ExprList(unique_ptr<ParserNode>& node);
  bool ExprListPrime(unique_ptr<ParserNode>& node);
  bool Expression(unique_ptr<ParserNode>& node);
  bool AuxExpression(unique_ptr<ParserNode>& node);
  bool OrExpr(unique_ptr<ParserNode>& node);
  bool OrExprPrime(unique_ptr<ParserNode>& node);
  bool AndExpr(unique_ptr<ParserNode>& node);
  bool AndExprPrime(unique_ptr<ParserNode>& node);
  bool EqExpr(unique_ptr<ParserNode>& node);
  bool EqExprprime(unique_ptr<ParserNode>& node);
  bool RelExpr(unique_ptr<ParserNode>& node);
  bool RelExprPrime(unique_ptr<ParserNode>& node);
  bool Expr(unique_ptr<ParserNode>& node);
  bool ExprPrime(unique_ptr<ParserNode>& node);
  bool Term(unique_ptr<ParserNode>& node);
  bool TermPrime(unique_ptr<ParserNode>& node);
  bool Unary(unique_ptr<ParserNode>& node);
  bool Factor(unique_ptr<ParserNode>& node);
  bool FactorPrime(unique_ptr<ParserNode>& node);
  bool Primary(unique_ptr<ParserNode>& node);
  bool AuxPrimary(unique_ptr<ParserNode>& node);
};

#endif // PARSER_HPP