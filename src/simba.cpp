#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <iostream>
#include <fstream>

std::ofstream output; //Output file
std::vector<std::string> endInsts;
std::unordered_set<std::string> vecs;
int spaces = 0;

bool insideFunc = false;
std::string Func;
std::vector<std::string> Funcs;
std::unordered_map<std::string,int> ArgNames;

bool errorFlag = false;

//===----------------------------------------------------------------------===//
// Auxiliar
//===----------------------------------------------------------------------===//

void w(const char *Str) {
  if (insideFunc) {
    Func += Str;
  } else {
    output << Str;
  }
}
void wi(const char *Str) {
  int aux = spaces;
  while (aux--)
    w(" ");
  w(Str);
}
void wn(const char *Str) {
  w(Str);
  w("\n");
}
void win(const char *Str) {
  wi(Str);
  w("\n");
}
void error(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  errorFlag = true;
}

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
  tok_eof = -1,

  tok_identifier = -2,
  tok_number = -3,

  tok_var = -4,
  tok_string = -5,
  tok_vector = -6,

  tok_equal = -7,
  tok_eq = -8,
  tok_ne = -9,
  tok_gt = -10,
  tok_ge = -11,
  tok_lt = -12,
  tok_le = -13,
  tok_plus = -14,
  tok_minus = -15,
  tok_end = -16,

  tok_while = -17,
  tok_for = -18,
  tok_swap = -19,
  tok_print = -20,
  tok_printn = -21,
  tok_if = -22,
  tok_func = -23
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

/// gettok - Return the next token from standard input.
static int gettok() {
  static int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = getchar();

  if (LastChar == '"' || LastChar == '\'') { // string: '"' [a-zA-Z0-9!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~ ]
    IdentifierStr = "";
    int Final = LastChar;
    while(isprint((LastChar = getchar())) && LastChar != Final) {
      IdentifierStr += LastChar;
    }
    LastChar = getchar();
    return tok_string;
  }

  if (isalpha(LastChar) || LastChar == '_') { // identifier: [a-zA-Z_][a-zA-Z0-9_]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())) || LastChar == '_')
      IdentifierStr += LastChar;

    if (IdentifierStr == "var")
      return tok_var;
    if (IdentifierStr == "vector")
      return tok_vector;
    
    if (IdentifierStr == "while")
      return tok_while;
    if (IdentifierStr == "for")
      return tok_for;
    if (IdentifierStr == "swap")
      return tok_swap;
    if (IdentifierStr == "print")
      return tok_print;
    if (IdentifierStr == "printn")
      return tok_printn;
    if (IdentifierStr == "if")
      return tok_if;
    if (IdentifierStr == "func")
      return tok_func;

    if (insideFunc) {
      if ((ArgNames.find(IdentifierStr) != ArgNames.end())) {
        IdentifierStr = "arg" + std::to_string(ArgNames[IdentifierStr]);
      } else {
        error("Using varible in function not present in func arguments\n");
      }
    }
    
    return tok_identifier;
  }

  if (isdigit(LastChar)) { // Number: [0-9]+
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar));

    NumVal = strtod(NumStr.c_str(), nullptr);
    return tok_number;
  }

  if (LastChar == '#') {
    // Comment until end of line.
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  if (LastChar == '=') {
    LastChar = getchar();
    if (LastChar == '=') {
      LastChar = getchar();
      return tok_eq;
    }
    return tok_equal;
  }
  if (LastChar == '!') {
    LastChar = getchar();
    if (LastChar == '=') {
      LastChar = getchar();
      return tok_ne;
    }
  }
  if (LastChar == '>') {
    LastChar = getchar();
    if (LastChar == '=') {
      LastChar = getchar();
      return tok_ge;
    }
    return tok_gt;
  }
  if (LastChar == '<') {
    LastChar = getchar();
    if (LastChar == '=') {
      LastChar = getchar();
      return tok_le;
    }
    return tok_lt;
  }
  if (LastChar == '+') {
    LastChar = getchar();
    return tok_plus;
  }
  if (LastChar == '-') {
    LastChar = getchar();
    if (isdigit(LastChar)) {
      while (isdigit(LastChar))
        LastChar = getchar();
    }

    return tok_minus;
  }

  if (LastChar == '}') {
    LastChar = getchar();
    return tok_end;
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;  

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

//===----------------------------------------------------------------------===//
// SimbaIR Code Generation
//===----------------------------------------------------------------------===//


void HandleVar() {
  getNextToken(); // consume 'var'

  std::vector<std::string> Names;
  while (CurTok != tok_equal) {
    if (CurTok == ',')
      getNextToken();
    else if (CurTok != tok_identifier) {
      error("Unsuported 'var' name");
      return;
    }

    std::string Var = IdentifierStr;
    Names.push_back(Var);
    getNextToken();
  }

  if (CurTok != tok_equal ) {
    error("Wrong 'var' syntax");
    return;
  }
  getNextToken(); //consume '='

  std::vector<int> Values;
  while (CurTok != ';') {
    if (CurTok == ',')
      getNextToken();
    else if (CurTok == tok_minus) {
      error("Negative values not supported in 'var'");
      return;
    } else if (CurTok != tok_number) {
      error("Variables can only be assigned to integers in 'var'");
      return;
    }

    int Value = NumVal;
    Values.push_back(Value);
    getNextToken();
  }

  if (Names.size() != Values.size()) {
    error("Number of variables different from the number of values in 'var'");
    return;
  }
  
  if (CurTok != ';') {
    error("No ';' after 'var'");
    return;
  }
  getNextToken(); // consume ';'


  //-------------Writing the var in SimbaIR

  for (int i=0; i < Names.size(); i++) {
    wi("var ");
    w(Names[i].c_str());
    w(" = ");
    w(std::to_string(Values[i]).c_str());
    wn(";");
  }
}
void HandleVector() {
  getNextToken(); // consume 'vector'

  if (CurTok != tok_identifier) {
    error("No valid 'vector' name");
    return;
  }
  std::string Var = IdentifierStr;
  getNextToken();

  if (CurTok != tok_equal ) {
    error("Wrong 'vector' syntax");
    return;
  }
  getNextToken(); //consume '='
  
  if (CurTok != '[' ) {
    error("Wrong 'vector' syntax");
    return;
  }
  getNextToken(); //consume '['

  std::string Elements = "";
  int Element = -1;
  bool first = true;
  while (CurTok != ']') {
    if (!first && CurTok  != ',') {
      error("Wrong 'vector' syntax");
      return;
    }
    if (!first) {
      Elements += ",";
      getNextToken();
    }

    if (CurTok != tok_number ) {
      error("Wrong 'vector' syntax");
      return;
    }
    Element = NumVal;
    Elements += std::to_string(Element);
    getNextToken();
    first = false;
  }
  getNextToken(); //consume ']'

  if (CurTok != ';') {
    error("No ';' after 'vector'");
    return;
  }
  getNextToken(); // consume ';'


  //-------------Writing the vector in SimbaIR
  wi("vector ");
  w(Var.c_str());
  w(" = [");
  w(Elements.c_str());
  wn("];");

  vecs.insert(Var);
}

void HandleAssign() {
  //if (CurTok != tok_identifier) {
  //  error("Must assign to a variable in 'assign'");
  //  return;
  //}
  std::string Var = IdentifierStr;

  getNextToken(); // consume '='

  std::string SourceVar = "";
  int SourceValue = -1;
  if (CurTok == tok_identifier)
    SourceVar = IdentifierStr;
  else
    SourceValue = NumVal;
  getNextToken(); // consume source

  std::vector<std::string> apendices;
  std::string apendix = "";
  while (CurTok != ';') {
    apendix = Var + " ";
    if (CurTok == tok_plus) {
      apendix += "+ ";
    } else if (CurTok == tok_minus){
      apendix += "- ";
    } else {
      error("Wrong 'assign' syntax");
      return;
    }
    getNextToken(); // consume operaion

    if (CurTok == tok_identifier) {
      apendix += IdentifierStr + ";";
    } else if (CurTok == tok_number) {
      apendix += std::to_string((int)NumVal) + ";";
    } else {
      error("Wrong 'assign' syntax");
      return;
    }
    getNextToken(); // consume operand

    apendices.push_back(apendix);
  }

  if (CurTok != ';') {
    error("No ';' after 'assign'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the assign in SimbaIR
  
  wi(Var.c_str());
  w(" = ");
  if (SourceVar != "")
    w(SourceVar.c_str());
  else
    w(std::to_string(SourceValue).c_str());
  wn(";");

  for (auto apendix: apendices) {
    win(apendix.c_str());
  }
}
void HandleSum() {
  std::string Var = IdentifierStr;

  getNextToken(); // consume '+'

  std::string SourceVar = "";
  int SourceValue = -1;
  if (CurTok == tok_identifier)
    SourceVar = IdentifierStr;
  else
    SourceValue = NumVal;
  getNextToken(); // consume source

  if (CurTok != ';') {
    error("No ';' after 'sum'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the sum in SimbaIR
  
  wi(Var.c_str());
  w(" + ");
  if (SourceVar != "")
    w(SourceVar.c_str());
  else
    w(std::to_string(SourceValue).c_str());
  wn(";");

}
void HandleSub() {
  std::string Var = IdentifierStr;

  getNextToken(); // consume '-'

  std::string SourceVar = "";
  int SourceValue = -1;
  if (CurTok == tok_identifier)
    SourceVar = IdentifierStr;
  else
    SourceValue = NumVal;
  getNextToken(); // consume source

  if (CurTok != ';') {
    error("No ';' after 'sub'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the sub in SimbaIR
  
  wi(Var.c_str());
  w(" - ");
  if (SourceVar != "")
    w(SourceVar.c_str());
  else
    w(std::to_string(SourceValue).c_str());
  wn(";");

}
void HandleWhile() {
  getNextToken(); // consume 'while'

  if (CurTok != '(') {
    error("Wrong 'while' syntax");
    return;
  }
  getNextToken(); //consume '('

  std::string Var = IdentifierStr;
  getNextToken();

  if (CurTok != tok_eq && CurTok != tok_ne &&
      CurTok != tok_gt && CurTok != tok_ge &&
      CurTok != tok_lt && CurTok != tok_le ) {
    error("Wrong 'while' syntax");
    return;
  }
  int op = CurTok;
  getNextToken(); //consume operator

  std::string cmpVar = "";
  int cmpValue = -1;
  if (CurTok == tok_identifier) {
    cmpVar = IdentifierStr;
  } else {
    cmpValue = NumVal;
  }
  getNextToken();

  if (CurTok != ')') {
    error("Wrong 'while' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != '{') {
    error("Wrong 'while' syntax");
    return;
  }
  getNextToken(); //consume '{'

  //-------------Writing the while in SimbaIR
  wn("");
  wi("while (");
  w(Var.c_str());
  w(" ");

  switch (op) {
    case tok_eq:
      w("eq");
      break;
    case tok_ne:
      w("neq");
      break;
    case tok_gt:
      w("gt");
      break;
    case tok_ge:
      w("ge");
      break;
    case tok_lt:
      w("lt");
      break;
    case tok_le:
      w("le");
      break;
    
    default:
      error("Wrong 'while' syntax");
      break;
  }

  w(" ");
  if (cmpVar != "")
    w(cmpVar.c_str());
  else
    w(std::to_string(cmpValue).c_str());
  wn(") {");

  endInsts.push_back("");
  spaces += 4;
}
void HandleFor() {
  getNextToken(); // consume 'for'

  if (CurTok != '(') {
    error("Wrong 'for' syntax");
    return;
  }
  getNextToken(); //consume '('

  std::string AssignVar = "";
  int AssignValue = -1;
  std::string AssignSourceVar = "";
  if (CurTok != ';') {
    AssignVar = IdentifierStr;
    getNextToken(); // consume var

    if (CurTok != tok_equal) {
      error("Wrong 'for' syntax");
      return;
    }
    getNextToken(); //consume '='

    if (CurTok == tok_identifier)
      AssignSourceVar = IdentifierStr;
    else
      AssignValue = NumVal;
    getNextToken(); // consume source
  }
  getNextToken(); // consume ';'

  std::string CondVar = IdentifierStr;
  getNextToken(); // consume var

  if (CurTok != tok_eq && CurTok != tok_ne &&
      CurTok != tok_gt && CurTok != tok_ge &&
      CurTok != tok_lt && CurTok != tok_le ) {
    error("Wrong 'for' syntax");
    return;
  }
  int op = CurTok;
  getNextToken(); //consume operator

  std::string CondSourceVar = "";
  int CondValue = -1;
  if (CurTok == tok_identifier) {
    CondSourceVar = IdentifierStr;
  } else {
    CondValue = NumVal;
  }
  getNextToken(); // consume source

  if (CurTok != ';') {
    error("Wrong 'for' syntax");
    return;
  }
  getNextToken(); //consume ';'

  std::string IncrVar = "";
  int IncrValue = -1;
  std::string IncrSourceVar = "";
  int incrOp = tok_plus;
  if (CurTok != ')') {
    IncrVar = IdentifierStr;
    getNextToken(); // consume var

    if (CurTok == tok_minus)
      incrOp = tok_minus;
     else if (CurTok != tok_plus) {
      error("Wrong 'for' syntax");
      return;
    }
    getNextToken(); // consume '+'/'-'

    if (CurTok == incrOp)
      IncrValue = 1;
    else if (CurTok == tok_identifier)
      IncrSourceVar = IdentifierStr;
    else
      IncrValue = NumVal;
    getNextToken(); // consume source
  }
  getNextToken(); //consume ')'

  if (CurTok != '{') {
    error("Wrong 'for' syntax");
    return;
  }
  getNextToken(); //consume '{'

  //-------------Writing the for in SimbaIR

  if (AssignVar != "") {
    wi(AssignVar.c_str());
    w(" = ");
    if (AssignSourceVar != "")
      w(AssignSourceVar.c_str());
    else
      w(std::to_string(AssignValue).c_str());
    wn(";");
  }

  wn("");
  wi("while (");
  w(CondVar.c_str());
  w(" ");

  switch (op) {
    case tok_eq:
      w("eq");
      break;
    case tok_ne:
      w("neq");
      break;
    case tok_gt:
      w("gt");
      break;
    case tok_ge:
      w("ge");
      break;
    case tok_lt:
      w("lt");
      break;
    case tok_le:
      w("le");
      break;
    
    default:
      error("Wrong 'while' syntax");
      break;
  }

  w(" ");
  if (CondSourceVar != "")
    w(CondSourceVar.c_str());
  else
    w(std::to_string(CondValue).c_str());
  wn(") {");
  
  spaces += 4;

  std::string endInst;
  if (IncrVar != "") {
    endInst = IncrVar + " ";
    if (incrOp == tok_plus)
      endInst += "+ ";
    else
      endInst += "- ";

    if (IncrSourceVar != "")
      endInst += IncrSourceVar;
    else
      endInst += std::to_string(IncrValue);
    
    endInst += ";";
    endInsts.push_back(endInst);
  }
}
void HandleSwap() {
  getNextToken(); // consume swap

  if (CurTok != '(') {
    error("Wrong 'swap' syntax");
    return;
  }
  getNextToken(); //consume '('

  std::string Vector = IdentifierStr;
  getNextToken();

  if (CurTok != ',') {
    error("Wrong 'swap' syntax");
    return;
  }
  getNextToken(); //consume ','

  int firstValue = -1;
  std::string firstVar = "";
  if (CurTok == tok_identifier)
    firstVar = IdentifierStr;
  else 
    firstValue = NumVal;
  getNextToken(); // consume first

  if (CurTok != ',') {
    error("Wrong 'swap' syntax");
    return;
  }
  getNextToken(); //consume ','

  int secondValue = -1;
  std::string secondVar = "";
  if (CurTok == tok_identifier)
    secondVar = IdentifierStr;
  else 
    secondValue = NumVal;
  getNextToken(); // consume second

  if (CurTok != ')') {
    error("Wrong 'swap' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != ';') {
    error("No ';' after 'swap'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the swap in SimbaIR

  wi(Vector.c_str());
  w("[");
  if (firstVar != "")
    w(firstVar.c_str());
  else
    w(std::to_string(firstValue).c_str());
  w("] -> ");
  wn("aux = el;");

  wi(Vector.c_str());
  w("[");
  if (secondVar != "")
    w(secondVar.c_str());
  else
    w(std::to_string(secondValue).c_str());
  wn("] ->");

  wi(Vector.c_str());
  w("[");
  if (firstVar != "")
    w(firstVar.c_str());
  else
    w(std::to_string(firstValue).c_str());
  wn("] = el;");
  
  wi(Vector.c_str());
  w("[");
  if (secondVar != "")
    w(secondVar.c_str());
  else
    w(std::to_string(secondValue).c_str());
  wn("] = aux;");

}
void HandlePrint(bool nl) {
  getNextToken(); // consume print

  if (CurTok != '(') {
    error("Wrong 'print' syntax");
    return;
  }
  getNextToken(); //consume '('

  std::string Var = "";
  std::string String = "";
  int Value = -1;
  if (CurTok == tok_string)
    String = IdentifierStr;
  else if (CurTok == tok_identifier)
    Var = IdentifierStr;
  else
    Value = NumVal;
  getNextToken(); // consume printable

  if (CurTok != ')') {
    error("Wrong 'print' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != ';') {
    error("No ';' after 'print'");
    return;
  }
  getNextToken(); // consume ';'
    

  //-------------Writing the assign in SimbaIR
  
  bool vec = false;
  if (Var != "" && (vecs.find(Var) != vecs.end())) {
    vec = true;
  }

  if (vec || insideFunc) {
    wn("");
    win("_iter = 0;");
    wi(Var.c_str());
    wn("[_size] -> _end = el;");
    win("_end - 1;");

    win("while (_iter le _end) {");

    wi("    print(\"");
    w(Var.c_str());
    wn("[\");");
    win("    print(_iter);");
    win("    print(\"]: \");");

    wi("    ");
    w(Var.c_str());
    wn("[_iter] -> printn(el);");
    win("    _iter + 1;");
    win("}");
    win("printn(\"\");");
    wn("");
  } else if (Value != -1) {
    if (nl)
      wi("printn(");
    else
      wi("print(");
    w(std::to_string(Value).c_str());
    wn(");");
  } else if (Var != "") {
    if (nl)
      wi("printn(");
    else
      wi("print(");
    w(Var.c_str());
    wn(");");
  } else {
    if (nl)
      wi("printn(\"");
    else
      wi("print(\"");
    w(String.c_str());
    wn("\");");
  }

}
void HandleIf() {
  getNextToken(); // consume if

  if (CurTok != '(') {
    error("Wrong 'if' syntax");
    return;
  }
  getNextToken(); //consume '('

  std::string Var = IdentifierStr;
  getNextToken(); // consume var

  bool VecElement1 = false;
  std::string IterVar = "";
  int IterValue = -1;
  if (CurTok == '[') {
    getNextToken(); // consume '['

    if (CurTok == tok_identifier)
      IterVar = IdentifierStr;
    else
      IterValue = NumVal;
    getNextToken(); // consume iterator

    if (CurTok != ']') {
      error("Wrong 'if' syntax");
      return;
    }
    getNextToken(); //consume ']'
  
    VecElement1 = true;
  }

  if (CurTok != tok_eq && CurTok != tok_ne &&
      CurTok != tok_gt && CurTok != tok_ge &&
      CurTok != tok_lt && CurTok != tok_le ) {
    error("Wrong 'if' syntax");
    return;
  }
  int op = CurTok;
  getNextToken(); //consume operator

  std::string Var2 = "";
  std::string IterVar2 = "";
  int IterValue2 = -1;
  bool VecElement2 = false;
  int CmpValue = -1;
  if (CurTok == tok_identifier) {
    Var2 = IdentifierStr;
    getNextToken(); // consume var

    if (CurTok == '[') {
      getNextToken(); // consume '['

      if (CurTok == tok_identifier)
        IterVar2 = IdentifierStr;
      else
        IterValue2 = NumVal;
      getNextToken(); // consume iterator

      if (CurTok != ']') {
        error("Wrong 'if' syntax");
        return;
      }
      getNextToken(); //consume ']'
    
      VecElement2 = true;
    }
  } else {
    CmpValue = NumVal;
    getNextToken(); // consume cmpvalue
  }

  if (CurTok != ')') {
    error("Wrong 'print' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != '{') {
    error("Wrong 'print' syntax");
    return;
  }
  getNextToken(); //consume '{'

  //-------------Writing the assign in SimbaIR
  
  std::string Operand1 = "";
  std::string Operand2 = "";
  if (VecElement1 && VecElement2) {
    wi(Var.c_str());
    w("[");
    if (IterVar != "")
      w(IterVar.c_str());
    else
      w(std::to_string(IterValue).c_str());
    wn("] -> aux = el;");

    wi(Var2.c_str());
    w("[");
    if (IterVar2 != "")
      w(IterVar2.c_str());
    else
      w(std::to_string(IterValue2).c_str());
    wn("] ->");

    Operand1 = "aux";
    Operand2 = "el";
  } else if (VecElement1) {
    wi(Var.c_str());
    w("[");
    if (IterVar != "")
      w(IterVar.c_str());
    else
      w(std::to_string(IterValue).c_str());
    wn("] ->");

    Operand1 = "el";
    if (Var2 != "")
      Operand2 = Var2;
    else
      Operand2 = std::to_string(CmpValue);
  } else {
    Operand1 = Var;
    if (Var2 != "")
      Operand2 = Var2;
    else
      Operand2 = std::to_string(CmpValue);
  }

  wi("if (");
  w(Operand1.c_str());
  w(" ");
  switch (op) {
    case tok_eq:
      w("eq");
      break;
    case tok_ne:
      w("neq");
      break;
    case tok_gt:
      w("gt");
      break;
    case tok_ge:
      w("ge");
      break;
    case tok_lt:
      w("lt");
      break;
    case tok_le:
      w("le");
      break;
    
    default:
      error("Wrong 'while' syntax");
      break;
  }
  w(" ");
  w(Operand2.c_str());
  wn(") {");

  spaces += 4;
  endInsts.push_back("");
}
void HandleFunc() {
  getNextToken(); // consume func

  std::string FuncName = IdentifierStr;
  getNextToken();

  if (CurTok != '(') {
    error("Wrong 'func' syntax");
    return;
  }
  getNextToken(); //consume '('

  int argCount = 0;
  std::string argName;
  std::vector<std::string> argNames;
  while (CurTok != ')') {
    if (CurTok == ',') {
      getNextToken(); // consume ','
    } else {
      argName = IdentifierStr;
      ArgNames[argName] = ++argCount;
      getNextToken(); // consume object
    }
  }

  if (CurTok != ')') {
    error("Wrong 'func' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != '{') {
    error("Wrong 'func' syntax");
    return;
  }
  insideFunc = true;
  getNextToken(); //consume '{'

  //-------------Writing the func in SimbaIR

  wi("func ");
  w(FuncName.c_str());
  w("(");
  w(std::to_string(argCount).c_str());
  wn(") {");

  endInsts.push_back("leaveFunc");
  spaces += 4;
}
void HandleCall() {
  std::string FuncName = IdentifierStr;

  getNextToken(); // consume '('

  std::string arg;
  std::vector<std::string> args;
  while (CurTok != ')') {
    if (CurTok == ',') {
      getNextToken(); // consume ','
    } else {
      arg = IdentifierStr;
      if (vecs.find(arg) != vecs.end()) {
        arg = "[" + arg + "]";
      }
      args.push_back(arg);
      getNextToken(); // consume object
    }
  }

  if (CurTok != ')') {
    error("Wrong 'call' syntax");
    return;
  }
  getNextToken(); //consume ')'

  //-------------Writing the call in SimbaIR

  wi(FuncName.c_str());
  w("(");
  for(int i=0; i<args.size(); i++) {
    if (i)
      w(",");
    w(args[i].c_str());
  }
  wn(");");
}
void HandleVecElement() {
  std::string Vec = IdentifierStr;
  getNextToken(); // consume '['

  std::string iterVar = "";
  int iterValue = -1;
  if (CurTok == tok_identifier)
    iterVar = IdentifierStr;
  else
    iterValue = NumVal;
  getNextToken(); // consume iterator

  if (CurTok != ']') {
    error("Wrong 'vecElement' syntax");
    return;
  }
  getNextToken(); // consume ']'

  if (CurTok != tok_equal) {
    error("Wrong 'vecElement' syntax");
    return;
  }
  getNextToken(); // consume '='
  
  std::string Var = "";
  int Value = -1;
  if (CurTok == tok_identifier) {
    Var = IdentifierStr;
  } else {
    Value = NumVal;
  }
  getNextToken(); // consume source

  if (CurTok != ';') {
    error("No ';' after 'set_vecElement'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the vecElement in SimbaIR

  wi(Vec.c_str());
  w("[");
  if (iterVar != "")
    w(iterVar.c_str());
  else
    w(std::to_string(iterValue).c_str());
  w("] = ");
  if (Var != "")
    w(Var.c_str());
  else
    w(std::to_string(Value).c_str());
  wn(";");
}

void HandleEnd() {
  getNextToken(); // consume '}'

  //-------------Writing the end in SimbaIR

  std::string endInst = endInsts.back();

  endInsts.pop_back();

  if (endInst != "" && endInst != "leaveFunc")
    win(endInst.c_str());
  spaces -= 4;
  win("}");
  wn("");

  if (endInst == "leaveFunc") {
    std::string func = Func;
    Func = "";
    Funcs.push_back(func);
    insideFunc = false;
    ArgNames.clear();
  }
}
void HandleExit() {
  if (errorFlag)
    return;
  
  wn("");
  wn("_____");
  wn("");

  for (auto func: Funcs) {
    wn(func.c_str());
  }
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void MainLoop() {
  while (!errorFlag) {
    switch (CurTok) {
    case tok_eof:
      return;

    case tok_var:
      HandleVar();
      break;
    case tok_vector:
      HandleVector();
      break;
    
    case tok_equal:
      HandleAssign();
      break;
    case tok_plus:
      HandleSum();
      break;
    case tok_minus:
      HandleSub();
      break;
    case tok_while:
      HandleWhile();
      break;
    case tok_for:
      HandleFor();
      break;
    case tok_swap:
      HandleSwap();
      break;
    case tok_print:
      HandlePrint(false);
      break;
    case tok_printn:
      HandlePrint(true);
      break;
    case tok_if:
      HandleIf();
      break;
    case tok_func:
      HandleFunc();
      break;
    case '(':
      HandleCall();
      break;
    case '[':
      HandleVecElement();
      break;
    
    case tok_end:
      HandleEnd();
      break;
    
    default:
      getNextToken();
      break;
    }
  }
}

static void CheckErrors(char* outputFile) {
  if (!errorFlag)
    return;

  w("Error: There was an error in the Simba code");
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char *argv[]) {
  static char *outputFile = argv[1];

  output.open(outputFile);
  getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  HandleExit();

  CheckErrors(outputFile);

  output.close();

  return 0;
}