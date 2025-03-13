#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <iostream>
#include <fstream>

static char *outputFile;
std::ofstream output; //Output file

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

enum Goto {
  plain = -1,
  equal = -2,
  not_equal = -3,
  greater_than = -4,
  greater_or_equal = -5,
  less_than = -6,
  less_or_equal = -7
};

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
  tok_eof = -1,

  tok_identifier = -2,
  tok_number = -3,
  tok_str = -4,
  tok_string = -5,
  tok_string_n = -6,
  tok_var = -7,
  tok_vector = -8,

  tok_reserve = -9,

  tok_section = -10,
  tok_label = -11,
  tok_sublabel = -12,
  tok_import = -13,

  tok_prints = -14,
  tok_printi = -15,
  tok_printv = -16,
  tok_assign = -17,
  tok_sum = -18,
  tok_sub = -19,
  tok_goto = -20,
  tok_e_goto = -21,
  tok_ne_goto = -22,
  tok_g_goto = -23,
  tok_ge_goto = -24,
  tok_l_goto = -25,
  tok_le_goto = -26,
  tok_element = -27,
  tok_set_element = -28,
  tok_load = -29,
  tok_call = -30,
  tok_return = -31,
  tok_push = -32,
  tok_pushv = -33,

  tok_error = -34
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
    return tok_str;
  }

  if (LastChar == '.') { // section: '.' [a-zA-Z]*
    IdentifierStr = "";
    while (isalpha((LastChar = getchar()))) {
      IdentifierStr += LastChar;
    }
    return tok_section;
  }

  if (isalpha(LastChar) || LastChar == '_') { // identifier: [a-zA-Z_][a-zA-Z0-9_]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())) || LastChar == '_')
      IdentifierStr += LastChar;

    if (IdentifierStr == "prints")
      return tok_prints;
    if (IdentifierStr == "printi")
      return tok_printi;
    if (IdentifierStr == "printv")
      return tok_printv;
    if (IdentifierStr == "assign")
      return tok_assign;
    if (IdentifierStr == "sum")
      return tok_sum;
    if (IdentifierStr == "sub")
      return tok_sub;
    if (IdentifierStr == "goto")
      return tok_goto;
    if (IdentifierStr == "e_goto")
      return tok_e_goto;
    if (IdentifierStr == "ne_goto")
      return tok_ne_goto;
    if (IdentifierStr == "g_goto")
      return tok_g_goto;
    if (IdentifierStr == "ge_goto")
      return tok_ge_goto;
    if (IdentifierStr == "l_goto")
      return tok_l_goto;
    if (IdentifierStr == "le_goto")
      return tok_le_goto;
    if (IdentifierStr == "element")
      return tok_element;
    if (IdentifierStr == "set_element")
      return tok_set_element;
    if (IdentifierStr == "load")
      return tok_load;
    if (IdentifierStr == "call")
      return tok_call;
    if (IdentifierStr == "return")
      return tok_return;
    if (IdentifierStr == "push")
      return tok_push;
    if (IdentifierStr == "pushv")
      return tok_pushv;

    if (IdentifierStr == "label")
      return tok_label;
    if (IdentifierStr == "sublabel")
      return tok_sublabel;
    if (IdentifierStr == "import")
      return tok_import;
    if (IdentifierStr == "string")
      return tok_string;
    if (IdentifierStr == "string_n")
      return tok_string_n;
    if (IdentifierStr == "var")
      return tok_var;
    if (IdentifierStr == "vector")
      return tok_vector;
    
    if (IdentifierStr == "reserve")
      return tok_reserve;

    if (IdentifierStr == "Error")
      return tok_error;

    if (IdentifierStr == "rsp") {
      if(LastChar == '+'){
        IdentifierStr += LastChar;
        while (isdigit((LastChar = getchar())))
          IdentifierStr += LastChar;
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

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace {

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() = default;

  virtual int codegen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  int Val;

public:
  NumberExprAST(int Val) : Val(Val) {}

  int codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}

  int codegen() override;
};

class PrintsExprAST : public ExprAST {
  std::string Name;
  std::unique_ptr<ExprAST> Size;

public:
  PrintsExprAST(const std::string &Name, std::unique_ptr<ExprAST> Size) : 
  Name(Name) , Size(std::move(Size)) {}

  int codegen() override;
};

class PrintiExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Arg;

public:
  PrintiExprAST(std::unique_ptr<ExprAST> Arg) : Arg(std::move(Arg)) {}

  int codegen() override;
};

class PrintvExprAST : public ExprAST {
  std::string Var;

public:
  PrintvExprAST(const std::string &Var) : Var(std::move(Var)) {}

  int codegen() override;
};

}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}

/// printexpr ::= 'prints' '(' id ',' numberexpr ')'
static std::unique_ptr<ExprAST> ParsePrintsExpr() {
  getNextToken(); // consume 'prints'

  if (CurTok != '(') {
    return LogError("Wrong 'prints' syntax");
  }
  getNextToken(); // consume '('

  std::string Arg = IdentifierStr;
  getNextToken();

  if (CurTok != ',') {
    return LogError("Wrong 'prints' syntax");
  }
  getNextToken(); // consume ')'

  auto Size = ParseNumberExpr();
  if (!Size) {
    return nullptr;
  }

  if (CurTok != ')') {
    return LogError("Wrong 'prints' syntax");
  }
  getNextToken(); // consume ')'

  if (CurTok != ';') {
    return LogError("No ';' after 'print'");
  }
  getNextToken(); // consume ';'

  return std::make_unique<PrintsExprAST>(Arg, std::move(Size));
}

/// printiexpr ::= 'printi' '(' numberexpr ')'
static std::unique_ptr<ExprAST> ParsePrintiExpr() {
  getNextToken(); // consume 'printi'

  if (CurTok != '(') {
    return LogError("Wrong 'printi' syntax");
  }
  getNextToken(); // consume '('

  auto Arg = ParseNumberExpr();
  if (!Arg) {
    return nullptr;
  }

  if (CurTok != ')') {
    return LogError("Wrong 'printi' syntax");
  }
  getNextToken(); // consume ')'

  if (CurTok != ';') {
    return LogError("No ';' after 'printi'");
  }
  getNextToken(); // consume ';'

  return std::make_unique<PrintiExprAST>(std::move(Arg));
}

/// printvexpr ::= 'printv' '(' id ')'
static std::unique_ptr<ExprAST> ParsePrintvExpr() {
  getNextToken(); // consume 'printv'

  if (CurTok != '(') {
    return LogError("Wrong 'printv' syntax");
  }
  getNextToken(); // consume '('

  std::string Var = IdentifierStr;
  getNextToken(); // consume var

  if (CurTok != ')') {
    return LogError("Wrong 'printv' syntax");
  }
  getNextToken(); // consume ')'

  if (CurTok != ';') {
    return LogError("No ';' after 'printv'");
  }
  getNextToken(); // consume ';'

  return std::make_unique<PrintvExprAST>(Var);
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

int LogErrorI(const char *Str) {
    LogError(Str);
    return 0;
}

int NumberExprAST::codegen() {
    w(std::to_string(Val).c_str());
    return 0;
}

int VariableExprAST::codegen() {
    return 0;
}

int PrintsExprAST::codegen() {
  w("  ; prints (");
  w(Name.c_str());
  w(", ");
  Size->codegen();
  wn(")");

  w("  mov  edi, ");
  wn(Name.c_str());
  w("  mov  edx, ");
  Size->codegen();
  wn("");

  wn("  call  print_string");
  wn("");

  return 0;
}

int PrintiExprAST::codegen() {
  w("  ; printi (");
  Arg->codegen();
  wn(")");

  w("  mov  ebx, ");
  Arg->codegen();
  wn("");

  wn("  lea  edi, [ebx]");
  wn("  call  print_uint32");
  wn("");
  return 0;
}

int PrintvExprAST::codegen() {
  w("  ; printv (");
  w(Var.c_str());
  wn(")");

  w("  mov  ebx, [");
  w(Var.c_str());
  wn("]");

  wn("  lea  edi, [ebx + 0]");
  wn("  call  print_uint32");
  wn("");
  return 0;
}

//===----------------------------------------------------------------------===//
// ASM Code Generation
//===----------------------------------------------------------------------===//

void CreateASMLabel(const char* name, bool sub = false, int afterSpaces = 0) {
  if (!sub) {
    wn("ALIGN 16");

    w("global ");
    wn(name);
  }

  w(name);
  wn(":");
  while(afterSpaces--)
    wn("");
}

void CreateASMSection() {
  std::string Name = IdentifierStr;
  getNextToken(); // consume section name

  if (CurTok != '(' ) {
    LogError("Wrong 'section' syntax");
    return;
  }
  getNextToken(); // consume '('

  int beforeSpaces = NumVal;
  getNextToken(); // consume beforeSpaces number

  if (CurTok != ',' ) {
    LogError("Wrong 'section' syntax");
    return;
  }
  getNextToken(); // consume ','

  int afterSpaces = NumVal;
  getNextToken(); // consume afterSpaces number

  if (CurTok != ')' ) {
    LogError("Wrong 'section' syntax");
    return;
  }
  getNextToken(); // consume ')'

  //-------------Writing the section in ASM

  while(beforeSpaces--)
    wn("");

  w("section .");
  wn(Name.c_str());

  while(afterSpaces--) 
    wn("");
}

//===----------------------------------------------------------------------===//

void Mov(const char* dest, const char* source) {
  w("  mov  ");
  w(dest);
  w(", ");
  wn(source);
}

void Push(const char* target) {
  w("  push   ");
  wn(target);
}

void Add(const char* dest, const char* source) {
  w("  add  ");
  w(dest);
  w(", ");
  wn(source);
}

void Test(const char* dest, const char* source) {
  w("  test  ");
  w(dest);
  w(", ");
  wn(source);
}

void Lea(const char* dest, const char* source) {
  w("  lea  ");
  w(dest);
  w(", ");
  wn(source);
}

void Sub(const char* dest, const char* source) {
  w("  sub  ");
  w(dest);
  w(", ");
  wn(source);
}

void Div(const char* target) {
  w("  div ");
  wn(target); 
}

void Dec(const char* target) {
  w("  dec  ");
  wn(target); 
}

void Jnz(const char* label) {
  w("  jnz  ");
  wn(label);
}

void Ret() {
  wn("  ret");
}
void Syscall() {
  wn("  syscall");
}
void CleanReg(const char* reg) {
  w("  xor    ");
  w(reg);
  w(", ");
  wn(reg);
}

//===----------------------------------------------------------------------===//

void ASMPrintsHandler() {
  CreateASMLabel("print_string");
  Lea("rsi", "[edi]");
  Mov("rax","1");
  Mov("rdi","1");
  Syscall();
  Ret();
  wn("");
}

void ASMPrintiHandler() {
  CreateASMLabel("print_uint32");
  Mov("eax","edi");
  Mov("ecx","10");
  Push("rcx");
  Mov("rsi","rsp");
  CreateASMLabel(".toascii_digit", true);
  CleanReg("edx");
  Div("ecx");
  Add("edx","'0'");
  Dec("rsi");
  Mov("[rsi]", "dl");
  Test("eax","eax");
  Jnz(".toascii_digit");
  Mov("eax", "1");
  Mov("edi","1");
  Lea("edx", "[rsp]");
  Sub("edx","esi");
  Syscall();
  Add("rsp","8");
  Ret();
  wn("");
}

void ASMHandlers () {
  getNextToken(); // consume 'import'

  if (CurTok != '(') {
    LogError("Wrong 'import' syntax");
    return;
  }

  bool reading = true;
  while (reading) {
    getNextToken();
    switch (CurTok) {
      case tok_prints:
        ASMPrintsHandler();
        break;
      case tok_printi:
        ASMPrintiHandler();
        break;
      default:
        reading = false;
        break;
    }
  }

  if (CurTok != ')') {
    LogError("Wrong 'import' syntax");
    return;
  }
  getNextToken(); // consume ')'
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void HandlePrints() {
  if (auto Prints = ParsePrintsExpr())
    Prints->codegen();
  else
    getNextToken();
}

static void HandlePrinti() {
  if (auto Printi = ParsePrintiExpr())
    Printi->codegen();
  else
    getNextToken();
}

static void HandlePrintv() {
  if (auto Printv = ParsePrintvExpr())
    Printv->codegen();
  else
    getNextToken();
}

static void HandleAssign() {
  getNextToken(); // consume 'assign'

  if (CurTok != '(' ) {
    LogError("Wrong 'assign' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string Var = IdentifierStr;
  getNextToken(); // consume varName

  if (CurTok != ',' ) {
    LogError("Wrong 'assign' syntax");
    return;
  }
  getNextToken(); //consume ','

  std::string SourceVar = "";
  int SourceValue = -1;
  if (CurTok == tok_identifier) {
    SourceVar = IdentifierStr;
  } else {
    SourceValue = NumVal;
  }
  getNextToken();

  if (CurTok != ')' ) {
    LogError("Wrong 'assign' syntax");
    return;
  }
  getNextToken(); //consume ')'
  
  if (CurTok != ';') {
    LogError("No ';' after 'assign'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the assign in ASM
  w("  ; assign (");
  w(Var.c_str());
  w(", ");
  if (SourceVar != "") {
    w(SourceVar.c_str());
  } else {
    w(std::to_string(SourceValue).c_str());
  }
  wn(")");

  if (SourceVar != "") {
    w("  mov  ebx, ");
    w("[");w(SourceVar.c_str());wn("]");
    w("  mov dword [");
    w(Var.c_str());
    wn("], ebx");
  } else {
    w("  mov dword [");
    w(Var.c_str());
    w("], ");
    wn(std::to_string(SourceValue).c_str());
  }
  wn("");
}

static void HandleSum() {
  getNextToken(); // consume 'sum'

  if (CurTok != '(' ) {
    LogError("Wrong 'sum' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string DestVar = IdentifierStr;
  getNextToken(); // consume destVarName

  if (CurTok != ',' ) {
    LogError("Wrong 'sum' syntax");
    return;
  }
  getNextToken(); //consume ','

  std::string SourceVar = "";
  int Value = -1;
  if (CurTok == tok_identifier) {
    SourceVar = IdentifierStr;
  } else {
    Value = NumVal;
  }
  getNextToken(); // consume value

  if (CurTok != ')' ) {
    LogError("Wrong 'sum' syntax");
    return;
  }
  getNextToken(); //consume ')'
  
  if (CurTok != ';') {
    LogError("No ';' after 'sum'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the sum in ASM
  w("  ; sum (");
  w(DestVar.c_str());
  w(", ");
  if (SourceVar != "") {
    w(SourceVar.c_str());
  } else {
    w(std::to_string(Value).c_str());
  }
  wn(")");

  w("  mov  ebx, [");
  w(DestVar.c_str());
  wn("]");

  w("  add  ebx, ");
  if (SourceVar != "") {
    w("[");
    w(SourceVar.c_str());
    wn("]");
  } else {
    wn(std::to_string(Value).c_str());
  }

  w("  mov  [");
  w(DestVar.c_str());
  wn("], ebx");

  wn("");
}

static void HandleSub() {
  getNextToken(); // consume 'sub'

  if (CurTok != '(' ) {
    LogError("Wrong 'sum' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string DestVar = IdentifierStr;
  getNextToken(); // consume destVarName

  if (CurTok != ',' ) {
    LogError("Wrong 'sub' syntax");
    return;
  }
  getNextToken(); //consume ','

  std::string SourceVar = "";
  int Value = -1;
  if (CurTok == tok_identifier) {
    SourceVar = IdentifierStr;
  } else {
    Value = NumVal;
  }
  getNextToken(); // consume value

  if (CurTok != ')' ) {
    LogError("Wrong 'sub' syntax");
    return;
  }
  getNextToken(); //consume ')'
  
  if (CurTok != ';') {
    LogError("No ';' after 'sub'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the sub in ASM
  w("  ; sub (");
  w(DestVar.c_str());
  w(", ");
  if (SourceVar != "") {
    w(SourceVar.c_str());
  } else {
    w(std::to_string(Value).c_str());
  }
  wn(")");

  w("  mov  ebx, [");
  w(DestVar.c_str());
  wn("]");

  w("  sub  ebx, ");
  if (SourceVar != "") {
    w("[");
    w(SourceVar.c_str());
    wn("]");
  } else {
    wn(std::to_string(Value).c_str());
  }

  w("  mov  [");
  w(DestVar.c_str());
  wn("], ebx");

  wn("");
}

static void HandleGoto(Goto mode) {
  std::string Var;
  std::string LimitVar = "";
  int LimitValue = -1;

  getNextToken(); // consume 'goto'

  if (CurTok != '(' ) {
    LogError("Wrong 'goto' syntax");
    return;
  }
  getNextToken(); // consume '('

  if (mode != plain) {
    Var = IdentifierStr;
    getNextToken(); // consume varName

    if (CurTok != ',' ) {
      LogError("Wrong 'goto' syntax");
      return;
    }
    getNextToken(); // consume ','

    if (CurTok == tok_identifier) {
      LimitVar = IdentifierStr;
    } else {
      LimitValue = NumVal;
    }
    getNextToken(); // consume limit

    if (CurTok != ',' ) {
      LogError("Wrong 'goto' syntax");
      return;
    }
    getNextToken(); // consume ','
  }

  std::string Label = IdentifierStr;
  getNextToken(); //consume label


  if (CurTok != ')' ) {
    LogError("Wrong 'goto' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != ';') {
    LogError("No ';' after 'goto'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the goto in ASM
  switch (mode) {
    case plain:
      w("  ; goto (");
      w(Label.c_str());
      wn(")");
      break;
    case equal:
      w("  ; e_goto (");
      break;
    case not_equal:
      w("  ; ne_goto (");
      break;
    case greater_than:
      w("  ; g_goto (");
      break;
    case greater_or_equal:
      w("  ; ge_goto (");
      break;
    case less_than:
      w("  ; l_goto (");
      break;
    case less_or_equal:
      w("  ; le_goto (");
      break;
  }

  if (mode == plain) {
    w("  jmp  ");
    wn(Label.c_str());
  } else {
    w(Var.c_str());
    w(", ");
    if (LimitVar != "") w(LimitVar.c_str());
    else w(std::to_string(LimitValue).c_str());
    w(", ");
    w(Label.c_str());
    wn(")");

    w("  mov  ebx, [");
    w(Var.c_str());
    wn("]");

    w("  cmp  ebx, ");
    if (LimitVar != "") {
      w("[");w(LimitVar.c_str());wn("]");
    } else {
      wn(std::to_string(LimitValue).c_str());
    }

    switch (mode) {
      case equal:
        w("  je  ");wn(Label.c_str());
        break;
      case not_equal:
        w("  jne  ");wn(Label.c_str());
        break;
      case greater_than:
        w("  jg  ");wn(Label.c_str());
        break;
      case greater_or_equal:
        w("  jge  ");wn(Label.c_str());
        break;
      case less_than:
        w("  jl  ");wn(Label.c_str());
        break;
      case less_or_equal:
        w("  jle  ");wn(Label.c_str());
        break;
      default:
        break;
    }
  }
  wn("");
}

static void HandleElement() {
  getNextToken(); // consume 'element'

  if (CurTok != '(' ) {
    LogError("Wrong 'element' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string Vector = IdentifierStr;
  if (isdigit(Vector.back())) {
    std::string aux = "[" + Vector + "]";
    Vector = aux;
  }
  getNextToken(); //consume vectorName

  if (CurTok != ',' ) {
    LogError("Wrong 'element' syntax");
    return;
  }
  getNextToken(); //consume ','

  std::string Var = "";
  int Value = -1;
  if (CurTok == tok_identifier) {
    Var = IdentifierStr;
  } else {
    Value = NumVal;
  }
  getNextToken();

  if (CurTok != ')' ) {
    LogError("Wrong 'element' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != ';') {
    LogError("No ';' after 'element'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the element in ASM
  w("  ; element (");
  w(Vector.c_str());
  w(", ");
  if (Var != "") {
    w(Var.c_str());
  } else {
    w(std::to_string(Value).c_str());
  }
  wn(")");

  Mov("esi",Vector.c_str());

  w("  mov  eax, ");
  if (Var != "") {
    w("[");
    w(Var.c_str());
    wn("]");
    wn("  add  eax, 1");
  } else {
    wn(std::to_string(Value).c_str());
  }

  wn("  imul  eax, eax, 4");
  Add("esi","eax");
  Mov("eax","[esi]");
  Mov("dword [el]","eax");
  wn("");
}

static void HandleSetElement() {
  getNextToken(); // consume 'set_element'

  if (CurTok != '(' ) {
    LogError("Wrong 'set_element' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string Vector = IdentifierStr;
  if (isdigit(Vector.back())) {
    std::string aux = "[" + Vector + "]";
    Vector = aux;
  }
  getNextToken(); //consume vectorName

  if (CurTok != ',' ) {
    LogError("Wrong 'set_element' syntax");
    return;
  }
  getNextToken(); //consume ','

  std::string IndexVar = "";
  int IndexValue = -1;
  if (CurTok == tok_identifier) {
    IndexVar = IdentifierStr;
  } else {
    IndexValue = NumVal;
  }
  getNextToken();

  if (CurTok != ',' ) {
    LogError("Wrong 'set_element' syntax");
    return;
  }
  getNextToken(); //consume ','

  std::string Var = "";
  int Value = -1;
  if (CurTok == tok_identifier) {
    Var = IdentifierStr;
  } else {
    Value = NumVal;
  }
  getNextToken();

  if (CurTok != ')' ) {
    LogError("Wrong 'set_element' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != ';') {
    LogError("No ';' after 'set_element'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the setelement in ASM
  w("  ; set_element (");
  w(Vector.c_str());
  w(", ");
  if (IndexVar != "") {
    w(IndexVar.c_str());
  } else {
    w(std::to_string(IndexValue).c_str());
  }
  w(", ");
  if (Var != "") {
    w(Var.c_str());
  } else {
    w(std::to_string(Value).c_str());
  }
  wn(")");

  Mov("esi",Vector.c_str());

  w("  mov  eax, ");
  if (IndexVar != "") {
    w("[");
    w(IndexVar.c_str());
    wn("]");
    wn("  add  eax, 1");
  } else {
    wn(std::to_string(IndexValue).c_str());
  }

  wn("  imul  eax, eax, 4");
  Add("esi","eax");
  w("  mov  eax, ");
  if (Var != "") {
    w("[");w(Var.c_str());wn("]");
  } else {
    wn(std::to_string(Value).c_str());
  }
  Mov("dword [esi]","eax");
  wn("");
}

static void HandleLoad() {
  getNextToken(); // consume 'load'

  if (CurTok != '(' ) {
    LogError("Wrong 'load' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string Register = IdentifierStr;
  getNextToken(); //consume register

  if (CurTok != ',' ) {
    LogError("Wrong 'load' syntax");
    return;
  }
  getNextToken(); //consume ','

  std::string Var = "";
  int Value = -1;
  if (CurTok == tok_identifier) {
    Var = IdentifierStr;
  } else {
    Value = NumVal;
  }
  getNextToken();

  if (CurTok != ')' ) {
    LogError("Wrong 'load' syntax");
    return;
  }
  getNextToken(); //consume ')'

  if (CurTok != ';') {
    LogError("No ';' after 'load'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the setelement in ASM
  w("  ; load (");
  w(Register.c_str());
  w(", ");
  if (Var != "") {
    w(Var.c_str());
  } else {
    w(std::to_string(Value).c_str());
  }
  wn(")");

  w("  mov  ");
  w(Register.c_str());
  w(", ");
  if (Var != "") {
    wn(Var.c_str());
  } else {
    wn(std::to_string(Value).c_str());
  }
  wn("");
}

static void HandleCall() {
  getNextToken(); // consume 'call'
  
  if (CurTok != '(' ) {
    LogError("Wrong 'call' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string Label = IdentifierStr;
  getNextToken(); // consume label

  if (CurTok != ')' ) {
    LogError("Wrong 'call' syntax");
    return;
  }
  getNextToken(); // consume ')'

  if (CurTok != ';') {
    LogError("No ';' after 'call'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the call in ASM
  w("  call  ");
  wn(Label.c_str());
  wn("");
}

static void HandleReturn() {
  getNextToken(); // consume 'return'

  int CleanBytes = -1;
  if (CurTok == tok_number) {
    CleanBytes = NumVal;
    getNextToken(); // consume number
  }
  
  if (CurTok != ';') {
    LogError("No ';' after 'return'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the return in ASM
  w("  ret");
  if (CleanBytes != -1) {
    w("  ");wn(std::to_string(CleanBytes).c_str());
  } else {
    wn("");
  }
  wn("");
}

static void HandlePush(bool var) {
  getNextToken(); // consume 'push'
  
  if (CurTok != '(' ) {
    LogError("Wrong 'push' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string Var = "";
  int Value = -1;
  if (CurTok == tok_identifier)
    Var = IdentifierStr;
  else
    Value = NumVal;
  getNextToken(); // consume object

  if (CurTok != ')' ) {
    LogError("Wrong 'push' syntax");
    return;
  }
  getNextToken(); // consume ')'

  if (CurTok != ';') {
    LogError("No ';' after 'push'");
    return;
  }
  getNextToken(); // consume ';'

  //-------------Writing the push in ASM
  w("  push  qword  ");
  if (Var != "")
    if (var) {
      w("[");w(Var.c_str());wn("]");
    } else {
      wn(Var.c_str());
    }
  else
    wn(std::to_string(Value).c_str());
}


static void HandleLabel(bool sub = false, int afterSpaces = 0) {
  getNextToken(); // consume 'label'
  
  CreateASMLabel(IdentifierStr.c_str(), sub, afterSpaces);

  if (IdentifierStr == "_exit") {
    CleanReg("edi");
    Mov("eax","231");
    Syscall();
    wn("");
    wn("");
  }

  getNextToken(); // consume label name
}

static void HandleString(bool newline) {
  getNextToken(); // consume 'string'
  
  std::string Name = IdentifierStr;
  getNextToken(); //consume strName

  if (CurTok != '(' ) {
    LogError("Wrong 'string' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::string String = "'" + IdentifierStr +  "'";
  getNextToken(); //consume string

  if (CurTok != ')' ) {
    LogError("Wrong 'string' syntax");
    return;
  }
  getNextToken(); //consume ')'

  //-------------Writing the string in ASM
  w("  ");
  w(Name.c_str());
  w("  db  ");
  w(String.c_str());
  if (newline)
    wn(", 10");
  else
    wn(", 0");
}

static void HandleVar() {
  getNextToken(); // consume 'var'
  
  std::string Name = IdentifierStr;
  getNextToken(); //consume varName

  if (CurTok != '(' ) {
    LogError("Wrong 'var' syntax");
    return;
  }
  getNextToken(); // consume '('

  int Value = NumVal;
  getNextToken(); // consume value

  if (CurTok != ')' ) {
    LogError("Wrong 'var' syntax");
    return;
  }
  getNextToken(); //consume ')'

  //-------------Writing the var in ASM
  w("  ");
  w(Name.c_str());
  w("  dd  ");
  wn(std::to_string(Value).c_str());
}

static void HandleVector() {
  getNextToken(); // consume 'vector'
  
  std::string Name = IdentifierStr;
  getNextToken(); //consume vectorName

  if (CurTok != '(' ) {
    LogError("Wrong 'vector' syntax");
    return;
  }
  getNextToken(); // consume '('

  std::vector<std::string> elements;
  int Element;
  while (CurTok != ')') {
    switch (CurTok) {
      case ',':
        elements.push_back(", ");
        break;

      case tok_number:
        Element = NumVal;
        elements.push_back(std::to_string(Element));
        break;
      
      default:
        LogError("Wrong 'vector' syntax");
        return;
    }
    getNextToken();
  }

  if (CurTok != ')' ) {
    LogError("Wrong 'vector' syntax");
    return;
  }
  getNextToken(); //consume ')'

  //-------------Writing the vector in ASM
  w("  ");
  w(Name.c_str());
  w("  dd  ");
  for (auto el : elements) {
    w(el.c_str());
  }
  wn("");
}

static void HandleReserve() {
  getNextToken(); // consume 'reserve'
  
  std::string Name = IdentifierStr;
  getNextToken(); //consume varName

  if (CurTok != '(' ) {
    LogError("Wrong 'reserve' syntax");
    return;
  }
  getNextToken(); // consume '('

  int blocksToReserve = NumVal;
  getNextToken();

  if (CurTok != ')' ) {
    LogError("Wrong 'reserve' syntax");
    return;
  }
  getNextToken(); //consume ')'

  //-------------Writing the reserve in ASM
  w("  ");
  w(Name.c_str());
  w("  resd  ");
  w(std::to_string(blocksToReserve).c_str());
  wn("");
}

static void HandleError() {
  while(CurTok != tok_eof)
    getNextToken();

  std::remove(outputFile);
}

/// top ::= printiexpr | printiexpr
static void MainLoop() {
  while (true) {
    switch (CurTok) {
    case tok_eof:
      return;
    case tok_section:
      CreateASMSection();
      break;
    case tok_label:
      HandleLabel();
      break;
    case tok_sublabel:
      HandleLabel(true, 1);
      break;
    case tok_import:
      ASMHandlers();
      break;
    case tok_string:
      HandleString(false);
      break;
    case tok_string_n:
      HandleString(true);
      break;
    case tok_var:
      HandleVar();
      break;
    case tok_vector:
      HandleVector();
      break;

    case tok_reserve:
      HandleReserve();
      break;    

    case tok_prints:
      HandlePrints();
      break;
    case tok_printi:
      HandlePrinti();
      break;
    case tok_printv:
      HandlePrintv();
      break;
    case tok_assign:
      HandleAssign();
      break;
    case tok_sum:
      HandleSum();
      break;
    case tok_sub:
      HandleSub();
      break;
    case tok_goto:
      HandleGoto(plain);
      break;
    case tok_e_goto:
      HandleGoto(equal);
      break;
    case tok_ne_goto:
      HandleGoto(not_equal);
      break;
    case tok_g_goto:
      HandleGoto(greater_than);
      break;
    case tok_ge_goto:
      HandleGoto(greater_or_equal);
      break;
    case tok_l_goto:
      HandleGoto(less_than);
      break;
    case tok_le_goto:
      HandleGoto(less_or_equal);
      break;
    case tok_element:
      HandleElement();
      break;
    case tok_set_element:
      HandleSetElement();
      break;
    case tok_load:
      HandleLoad();
      break;
    case tok_call:
      HandleCall();
      break;
    case tok_return:
      HandleReturn();
      break;
    case tok_push:
      HandlePush(false);
      break;
    case tok_pushv:
      HandlePush(true);
      break;
    
    case tok_error:
      HandleError();
      break;

    default:
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

  getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  output.close();

  return 0;
}