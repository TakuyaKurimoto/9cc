#include "takuya.h"

VarList *locals;

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}
Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

Node *new_var(Var *var) {
  Node *node = new_node(ND_VAR);
  node->var = var;
  return node;
}

// Find a local variable by name.
Var *find_var(Token *tok) {
  for (VarList *vl = locals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  }
  return NULL;
}

Var *push_var(char *name, Type *ty) {
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->ty = ty;
  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  vl->next = locals;
  locals = vl;
  return var;
}



Function *function();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *postfix();
Node *primary();

// program = function*
Function *program() {
   Function head;
   head.next = NULL;
   Function *cur=&head;
    while (! at_eof()){
    cur->next = function();
    cur = cur->next;
  }
  return head.next;
} 

Type *basetype() {
  expect("int");
  Type *ty = int_type();
  while (consume("*"))
    ty = pointer_to(ty);
  return ty;
}

Type *read_type_suffix(Type *base) {
  if (!consume("["))
    return base;
  int sz = expect_number();
  expect("]");
  base = read_type_suffix(base);//二次元配列はarray of arrayと表せる。
  return array_of(base, sz);//char * argv[] はchar へのポインタの配列という事になる
}

VarList *read_func_param() {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = push_var(name, ty);
  return vl;
}

VarList *read_func_params() {
  if (consume(")"))
    return NULL;
  VarList *head = read_func_param();
  VarList *cur = head;
  while (!consume(")")) {
    expect(",");
    cur->next = read_func_param();
    cur = cur->next;
  }
  return head;
}

// function = basetype ident "(" params? ")" "{" stmt* "}"
// params   = param ("," param)*
// param    = basetype ident
Function *function() {
  locals = NULL;//ローカルズを毎回リセットする。
  Function *fn = calloc(1, sizeof(Function));
  basetype();//関数にはまだ型名を定義しない
  fn->name = expect_ident();
  expect("(");
  fn->params = read_func_params();//引数をローカル変数と見なす。
  expect("{");

  Node head;//https://teratail.com/questions/349077
  Node *node = stmt();
  head.next = node;
  while (!consume("}")){
    node->next = stmt();
    node=node->next;
  }
  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

// declaration = basetype ident ("[" num "]")* ("=" expr) ";"
Node *declaration() {
  Token *tok = token;
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  Var *var = push_var(name, ty);
  if (consume(";"))
    return new_node(ND_NULL);
  expect("=");
  Node *lhs = new_var(var);
  Node *rhs = expr();
  expect(";");
  return new_binary(ND_ASSIGN, lhs, rhs);
  
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
// 　　　| "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
// 　　　| declaration
//      | expr ";"
Node *stmt() {
  if (consume("return")) {
    Node *node = new_unary(ND_RETURN, expr());
    expect(";");
    return node;
  }
  if (consume("if")) {
    Node *node = new_node(ND_IF);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else")) {
      node->els = stmt();
    }
    return node;
  }
  if (consume("while")) {
    Node *node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }
  if (consume("for")) {
    Node *node = new_node(ND_FOR);
    expect("(");
    if (!consume(";")) {
      node->init = expr();
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->inc = expr();
      expect(")");
    }
    node->then = stmt();
    return node;
  }
  if (consume("{")) {
    Node head;
    head.next = NULL;
    Node *cur = &head; 
    while (!consume("}")){
      Node *node = stmt();
      cur->next = node;
      cur = cur->next;
    }
    Node *node = new_node(ND_BLOCK);
    node->body = head.next;
    return node;
  }
  if (peek("int"))
    return declaration();
  Node *node = expr();
  expect(";");
  return node;
}
// expr = equality
Node *expr() {
  return assign();
}
// assign = equality ("=" assign)?
Node *assign() {
  Node *node = equality();
  if (consume("=")){
    node = new_binary(ND_ASSIGN, node, assign());
  }
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_binary(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_binary(ND_NE, node, relational());
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_binary(ND_LT, node, add());
    else if (consume("<="))
      node = new_binary(ND_LE, node, add());
    else if (consume(">"))
      node = new_binary(ND_LT, add(), node);
    else if (consume(">="))
      node = new_binary(ND_LE, add(), node);
    else
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_binary(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_binary(ND_SUB, node, mul());
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_binary(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-" | "*" | "&")? unary
//        | postfix
Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_binary(ND_SUB, new_num(0), unary());
  if (consume("&"))
    return new_unary(ND_ADDR, unary());
  if (consume("*"))
    return new_unary(ND_DEREF, unary());
  return postfix();
}

// postfix = primary ("[" expr "]")*
Node *postfix() {
  Node *node = primary();
  Token *tok;
  while (consume("[")) {
    // x[y] is short for *(x+y)
    Node *exp = new_binary(ND_ADD, node, expr());
    expect("]");
    node = new_unary(ND_DEREF, exp);
  }
  return node;
}

// func-args = "(" (assign ("," assign)*)? ")"
Node *func_args() {
  if (consume(")"))
    return NULL;
  Node *head = assign();
  Node *cur = head;
  while (consume(",")) {
    cur->next = assign();
    cur = cur->next;
  }
  expect(")");
  return head;
}
// primary = "(" expr ")" | ident func-args? | num
// args = "(" ")"
Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }
  Token *t = consume_ident();
    if (t){//構造体はnullと比較できないからif (*t)はエラー　https://base64.work/so/c/1158512
      if (consume("(")) {
        
        Node *node = new_node(ND_FUNCALL);
        node->funcname = strndup(t->str, t->len);
        node->args = func_args();
        return node;
      }
      Var *var = find_var(t);
      if (!var)
        error_at(t->str, "undefined variable");
      return new_var(var);
    }
  return new_num(expect_number());
}