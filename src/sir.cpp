#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

#include <iostream>
#include <fstream>

static char *outputFile;
std::ofstream output; //Output file
std::unordered_map<std::string, std::vector<std::string>> dataSection;
std::vector<std::string> closing;
int ifCounter = 0;
int whileCounter = 0;
int strCounter = 0;
bool usePrinti = false;
bool usePrints = true;
bool errorFlag = false;

//===----------------------------------------------------------------------===//
// Auxiliar
//===----------------------------------------------------------------------===//

void w(const char *Str) {
    output << Str;
}
void wn(const char *Str) {
    output << Str;
    output << "\n";
}
void error(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
}
void addToDataSection (std::string Inst, std::string Name, std::string Value) {
  std::string NameValue = Name + " (" + Value + ")";
  dataSection[Inst].push_back(NameValue);
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

  tok_if = -7,
  tok_eq = -8,
  tok_neq = -9,
  tok_gt = -10,
  tok_ge = -11,
  tok_lt = -12,
  tok_le = -13,
  tok_print = -14,
  tok_printn = -15,
  tok_equal = -16,
  tok_plus = -17,
  tok_minus = -18,
  tok_while = -19,
  tok_func = -20,

  tok_end = -21,
  tok_exit = -22,
  tok_error = -23
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
    while (isalpha((LastChar = getchar())) || LastChar == '_')
      IdentifierStr += LastChar;

    if (IdentifierStr == "Error")
      return tok_error;

    if (IdentifierStr == "var")
      return tok_var;
    if (IdentifierStr == "vector")
      return tok_vector;
    
    if (IdentifierStr == "if")
      return tok_if;
    if (IdentifierStr == "eq")
      return tok_eq;
    if (IdentifierStr == "neq")
      return tok_neq;
    if (IdentifierStr == "gt")
      return tok_gt;
    if (IdentifierStr == "ge")
      return tok_ge;
    if (IdentifierStr == "lt")
      return tok_lt;
    if (IdentifierStr == "le")
      return tok_le;
    if (IdentifierStr == "print")
      return tok_print;
    if (IdentifierStr == "printn")
      return tok_printn;
    if (IdentifierStr == "while")
      return tok_while;
    if (IdentifierStr == "func")
      return tok_func;
    
    if (IdentifierStr == "_____")
      return tok_exit;

    if (IdentifierStr == "arg") {
      int numIter;
      IdentifierStr = LastChar;
      while (isdigit((LastChar=getchar()))) {
        IdentifierStr += LastChar;
      }
      numIter = std::stoi(IdentifierStr);
      IdentifierStr = "rsp+" + std::to_string((int)numIter*8);
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

  if (LastChar == '}') {
    LastChar = getchar();
    return tok_end;
  }

  if (LastChar == '=') {
    LastChar = getchar();
    return tok_equal;
  }
  if (LastChar == '+') {
    LastChar = getchar();
    return tok_plus;
  }
  if (LastChar == '-') {
    LastChar = getchar();
    return tok_minus;
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
// SASM Code Generation
//===----------------------------------------------------------------------===//

void SASMTextInitial() {
  wn(".bss (0,0)");
  wn("reserve el (1)");
  wn("reserve aux (1)");
  wn("reserve _iter (1)");
  wn("reserve _end (1)");
  wn("");
  wn(".text (1,1)");
  wn("");
  wn("label _start");
  wn("");
}

void SASMTextFinal() {
  if (errorFlag)
    return;

  if (usePrinti || usePrints) {
    w("import (");
    if (usePrinti) {
      w("printi");
      if (usePrints)
        w(" ");
    }
    if (usePrints)
      w("prints");
    wn(")");
  }
  wn("");
}

void SASMDataSection() {
  if (errorFlag)
    return;

  wn(".data (2,0)");
  addToDataSection("string_n", "nl", "\"\"");
  for (auto inst: dataSection) {
    for (auto NameValue: inst.second) {
      w(inst.first.c_str());
      w(" ");
      wn(NameValue.c_str());
    } 
  }
}

void HandleVar() {
  getNextToken(); // consume 'var'

  std::string Var = IdentifierStr;
  getNextToken();

  if (CurTok != tok_equal ) {
    error("Wrong 'var' syntax");
    return;
  }
  getNextToken(); //consume '='

  int Value = NumVal;
  getNextToken();

  if (CurTok != ';') {
    error("No ';' after 'var'");
    return;
  }
  getNextToken(); // consume ';'


  //-------------Writing the var in SASM

  addToDataSection("var", Var, std::to_string(Value));
}
void HandleVector() {
  getNextToken(); // consume 'vector'

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

  int Element = -1;
  int n_Elements = 0;
  std::string Elements = "";
  while (CurTok != ']') {
    if (CurTok  == ',')
      Elements += ",";
    else {
      Element = NumVal;
      Elements += std::to_string(Element);
      n_Elements++;
    }
    getNextToken();
  }
  Elements = std::to_string(n_Elements)+ "," + Elements;
  getNextToken(); //consume ']'

  if (CurTok != ';') {
    error("No ';' after 'vector'");
    return;
  }
  getNextToken(); // consume ';'


  //-------------Writing the vector in SASM
  addToDataSection("vector", Var, Elements);
}

void HandleIf() {
  getNextToken(); // consume 'if'

  if (CurTok != '(') {
    error("Wrong 'if' syntax");
    return;
  }
  getNextToken(); //consume '('

  std::string Var = IdentifierStr;
  getNextToken();

  if (CurTok != tok_eq && CurTok != tok_neq &&
      CurTok != tok_gt && CurTok != tok_ge &&
      CurTok != tok_lt && CurTok != tok_le ) {
    error("Wrong 'if' syntax");
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
    error("Wrong 'if' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != '{') {
    error("Wrong 'if' syntax");
    return;
  }
  getNextToken(); //consume '{'

  //-------------Writing the if in SASM
  wn("");
  w("sublabel if");
  wn(std::to_string(++ifCounter).c_str());

  switch (op) {
    case tok_eq:
      w("ne");
      break;
    case tok_neq:
      w("e");
      break;
    case tok_gt:
      w("le");
      break;
    case tok_ge:
      w("l");
      break;
    case tok_lt:
      w("ge");
      break;
    case tok_le:
      w("g");
      break;
    
    default:
      error("Wrong 'if' syntax");
      break;
  }

  w("_goto (");
  w(Var.c_str());
  w(", ");
  if (cmpVar != "")
    w(cmpVar.c_str());
  else
    w(std::to_string(cmpValue).c_str());
  w(", else");
  w(std::to_string(ifCounter).c_str());
  wn(");");

  const std::string closeTag = "else" + std::to_string(ifCounter);
  closing.push_back(closeTag);
}
void HandlePrint(bool newline) {
  getNextToken(); // consume 'print'

  if (CurTok != '(') {
    error("Wrong 'print' syntax");
    return;
  }
  getNextToken(); //consume '('

  int mode = 0;
  std::string Var = "";
  std::string Str = "";
  int Value = -1;
  enum Mode {
    VarMode = -1,
    StrMode = -2,
    UintMode = -3
  };
  if (CurTok == tok_number) {
    mode = UintMode;
    Value = NumVal;
  } else if (CurTok == tok_identifier) {
    mode = VarMode;
    Var = IdentifierStr;
  } else {
    mode = StrMode;
    Str = IdentifierStr;
  }
  getNextToken();


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

  //-------------Writing the print in SASM

  if (mode == UintMode) {
    w("printi (");
    w(std::to_string(Value).c_str());
    wn(");");

    if (newline)
      wn("prints (nl, 1);");

    usePrinti = true;
  } else if (mode == VarMode) {
    w("printv (");
    w(Var.c_str());
    wn(");");

    if (newline)
      wn("prints (nl, 1);");

    usePrinti = true;
  } else {
    w("prints (msg");
    w(std::to_string(++strCounter).c_str());
    w(", ");
    w(std::to_string(Str.size() + 1).c_str());
    wn(");");

    usePrints = true;
    std::string NameValue = "msg" + std::to_string(strCounter) + " (\"" +
                            Str + "\")";
    
    if (newline)
      dataSection["string_n"].push_back(NameValue);
    else
      dataSection["string"].push_back(NameValue);
  }
}
void HandleEqual() {
  std::string Var = IdentifierStr;

  getNextToken(); // consume '='

  std::string sourceVar = "";
  int Value = -1;
  if (CurTok == tok_identifier)
    sourceVar = IdentifierStr;
  else
    Value = NumVal;
  getNextToken();

  if (CurTok != ';') {
    error("No ';' after 'assign'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the assign in SASM

  w("assign (");
  w(Var.c_str());
  w(", ");
  if (sourceVar != "") {
    w(sourceVar.c_str());
  } else
    w(std::to_string(Value).c_str());
  wn(");");
}
void HandlePlus() {
  std::string Var = IdentifierStr;

  getNextToken(); // consume '+'

  std::string sourceVar = "";
  int Value = -1;
  if (CurTok == tok_identifier)
    sourceVar = IdentifierStr;
  else
    Value = NumVal;
  getNextToken();

  if (CurTok != ';') {
    error("No ';' after 'sum'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the sum in SASM

  w("sum (");
  w(Var.c_str());
  w(", ");
  if (sourceVar != "") {
    w(sourceVar.c_str());
  } else
    w(std::to_string(Value).c_str());
  wn(");");
}
void HandleMinus() {
  std::string Var = IdentifierStr;

  getNextToken(); // consume '-'

  std::string sourceVar = "";
  int Value = -1;
  if (CurTok == tok_identifier)
    sourceVar = IdentifierStr;
  else
    Value = NumVal;
  getNextToken();

  if (CurTok != ';') {
    error("No ';' after 'sub'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the sub in SASM

  w("sub (");
  w(Var.c_str());
  w(", ");
  if (sourceVar != "") {
    w(sourceVar.c_str());
  } else
    w(std::to_string(Value).c_str());
  wn(");");
}
void HandleVecElement() {
  std::string Vec = IdentifierStr;

  getNextToken(); // consume '['

  std::string iterVar = "";
  int iterValue = -1;
  if (CurTok == tok_identifier)
    iterVar = IdentifierStr;
    if (iterVar == "_size") {
      iterVar = "";
      iterValue = 0;
    }
  else
    iterValue = NumVal + 1;
  getNextToken();

  getNextToken(); // consume ']'

  bool set = false;
  std::string Var = "";
  int Value = -1;
  if (CurTok == tok_equal) {
    set = true;
    getNextToken(); // consume '='
    if (CurTok == tok_identifier) {
      Var = IdentifierStr;
    } else {
      Value = NumVal;
    }
    getNextToken();

    if (CurTok != ';') {
      error("No ';' after 'set_vecElement'");
      return;
    }
    getNextToken(); // consume ';'
  } else {
    getNextToken(); // consume '-'
    getNextToken(); // consume '>'
  }

  //-------------Writing the vecElement in SASM

  if (set)
    w("set_element (");
  else
    w("element (");
  w(Vec.c_str());
  w(", ");
  if (iterVar != "") {
    w(iterVar.c_str());
  } else
    w(std::to_string(iterValue).c_str());

  if (set) {
    w(", ");
    if (Var != "") {
      w(Var.c_str());
    } else
      w(std::to_string(Value).c_str());
    }
  wn(");");
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

  if (CurTok != tok_eq && CurTok != tok_neq &&
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
  getNextToken(); // consume comparator

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

  //-------------Writing the while in SASM
  wn("");
  w("sublabel while");
  wn(std::to_string(++whileCounter).c_str());

  switch (op) {
    case tok_eq:
      w("ne");
      break;
    case tok_neq:
      w("e");
      break;
    case tok_gt:
      w("le");
      break;
    case tok_ge:
      w("l");
      break;
    case tok_lt:
      w("ge");
      break;
    case tok_le:
      w("g");
      break;
    
    default:
      error("Wrong 'while' syntax");
      break;
  }

  w("_goto (");
  w(Var.c_str());
  w(", ");
  if (cmpVar != "")
    w(cmpVar.c_str());
  else
    w(std::to_string(cmpValue).c_str());
  w(", endwhile");
  w(std::to_string(whileCounter).c_str());
  wn(");");

  const std::string closeTag = "endwhile" + std::to_string(whileCounter);
  closing.push_back(closeTag);
}
void HandleFunc() {
  getNextToken(); // consume 'func'

  std::string Name = IdentifierStr;
  getNextToken();

  if (CurTok != '(') {
    error("Wrong 'func' syntax");
    return;
  }
  getNextToken(); // consume '('

  int n_args = NumVal;
  getNextToken(); // consume args nmbr

  if (CurTok != ')') {
    error("Wrong 'func' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != '{') {
    error("Wrong 'func' syntax");
    return;
  }
  getNextToken(); //consume '{'

  //-------------Writing the func in SASM
  wn("");

  w("label ");
  wn (Name.c_str());
  std::string closeTag;
  if (n_args == 0)
    closeTag = "return;";
  else
    closeTag = "return " + std::to_string(n_args) + ';';

  closing.push_back(closeTag);
}
void HandleCall() {
  std::string funcName = IdentifierStr;

  getNextToken(); // consume '('

  std::vector<std::string> args;
  std::string arg;
  while (CurTok != ')') {
    if (CurTok == ',') {
      getNextToken(); // consume ','
    } else if (CurTok == '[') {
      getNextToken(); // consume '['
      arg = IdentifierStr;
      args.push_back(arg);
      getNextToken(); // consume arg
      getNextToken(); // consume ']' 
    } else {
      arg = IdentifierStr;
      arg += ".";
      args.push_back(arg);
      getNextToken(); // consume arg
    }
  }

  if (CurTok != ')') {
    error("Wrong 'call' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != ';') {
    error("No ';' after 'call'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the while in SASM
  for (auto arg: args) {
    arg = args.back();
    args.pop_back();
    if (arg.back() == '.') {
      arg.pop_back();
      w("pushv (");
      w(arg.c_str());
      wn(");");
    } else {
      w("push (");
      w(arg.c_str());
      wn(");");
    }
  }

  w("call (");
  w(funcName.c_str());
  wn(");");
}

void HandleEnd() {
  getNextToken(); // consume '}'

  //-------------Writing the closeTag in SASM

  std::string closeTag = closing.back();

  closing.pop_back();

  if (closeTag[1] == 'n') {
    w("goto (while");
    w(&closeTag.back());
    wn(");");
    w("sublabel ");
  } else if ( closeTag[0] == 'e') {
    w("sublabel ");
  }
  wn(closeTag.c_str());
  wn("");
}
void HandleExit() {
  getNextToken(); // consume ______

  //-------------Writing the exit in SASM

  wn("");
  wn("prints (nl, 1);");
  wn("label _exit");
}
void HandleError() {
  errorFlag = true;

  while (CurTok != tok_eof)
    getNextToken();

  output.close();
  output.open(outputFile);

  w("Error: There was an error in the Simba code");
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void MainLoop() {
  while (true) {
    switch (CurTok) {
    case tok_eof:
      return;

    case tok_var:
      HandleVar();
      break;
    case tok_vector:
      HandleVector();
      break;
    
    case tok_if:
      HandleIf();
      break;
    case tok_print:
      HandlePrint(false);
      break;
    case tok_printn:
      HandlePrint(true);
      break;
    case tok_equal:
      HandleEqual();
      break;
    case tok_plus:
      HandlePlus();
      break;
    case tok_minus:
      HandleMinus();
      break;
    case '[':
      HandleVecElement();
      break;
    case tok_while:
      HandleWhile();
      break;
    case tok_func:
      HandleFunc();
      break;
    case '(':
      HandleCall();
      break;

    case tok_end:
      HandleEnd();
      break;
    case tok_exit:
      HandleExit();
      break;
    case tok_error:
      HandleError();
      break;
    
    default:
      getNextToken();
      break;
    }
  }
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char *argv[]) {
  outputFile = argv[1];

  output.open(outputFile);

  SASMTextInitial();

  getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  SASMTextFinal();
  SASMDataSection();

  output.close();

  return 0;
}