#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
// tokenize.c
//
typedef enum {
  TK_RESERVED, // Keywords or punctuators
  TK_IDENT,    // 識別子identifier
  TK_NUM,      // Integer literals
  TK_EOF,      // End-of-file markers
} TokenKind;
// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token kind
  Token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *str;      // Token string
  int len;        // Token lengthß
};
// Local variable
typedef struct Var Var;
struct Var {
  Var *next;
  char *name; // Variable name
  int offset; // Offset from RBP
};
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
char *strndup(char *p, int len);
bool consume(char *op);
void expect(char *op);
int expect_number();
char *expect_ident();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();
Token *consume_ident();
extern char *user_input;
extern Token *token;
//
// parse.c
//
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_ASSIGN, // =
  ND_ADDR,      // unary &
  ND_DEREF,     // unary *
  ND_NUM, // Integer
  ND_VAR, // variable
  ND_RETURN, // "return"
  ND_IF, // "if"
  ND_WHILE, // "while"
  ND_FOR, // "for"
  ND_BLOCK, // { ... }
  ND_FUNCALL, // Function call
} NodeKind;
// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *next; // Next node
  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side
  // Block
  Node *body;
  // "if, "while" or "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;
  Var *var; // Used if kind == ND_VAR
  int val;       // Used if kind == ND_NUM  
  // Function call
  char *funcname;
  Node *args;
};
typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
};

typedef struct Function Function;
struct Function {
  Function *next;//自分の定義の中で自分を呼び出す場合はあらかじめtypedefで宣言しとく
  char *name;
  VarList *params;
  Node *node;
  VarList *locals;
  int stack_size;
};
Function *program();



//
// codegen.c
//
void codegen(Function *prog);