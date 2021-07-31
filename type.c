#include "takuya.h"

Type *int_type() {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_INT;
  return ty;
}

Type *pointer_to(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->base = base;
  return ty;
}

void visit(Node *node) {
  if (!node)
    return;

  visit(node->lhs);
  visit(node->rhs);
  visit(node->cond);
  visit(node->then);
  visit(node->els);
  visit(node->init);
  visit(node->inc);

  for (Node *n = node->body; n; n = n->next)
    visit(n);
  for (Node *n = node->args; n; n = n->next)
    visit(n);

  switch (node->kind) {
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_VAR:
  case ND_FUNCALL:
  case ND_NUM:
    node->ty = int_type();
    return;
  case ND_ADD:
     if (node->rhs->ty->kind == TY_PTR) {//左にポインタ型が来るように調整
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    }
    if (node->rhs->ty->kind == TY_PTR)
      error_at("", "invalid pointer arithmetic operands");
    node->ty = node->lhs->ty;
    return;
  case ND_SUB:
    if (node->rhs->ty->kind == TY_PTR)
      error_at("", "invalid pointer arithmetic operands");
    node->ty = node->lhs->ty;
    return;
  case ND_ASSIGN:
    node->ty = node->lhs->ty;
    return;
  case ND_ADDR:
    node->ty = pointer_to(node->lhs->ty);
    return;
  case ND_DEREF:
    if (node->lhs->ty->kind == TY_PTR)//x=3; y=&x;*yとかだと*yという値は3を表すからint型。＊＊yみたいなやつは＊yというポインタ型を表すからポインタ型
      node->ty = node->lhs->ty->base;
    else
      node->ty = int_type();
    return;
  }
}

void add_type(Function *prog) {
  for (Function *fn = prog; fn; fn = fn->next)
    for (Node *node = fn->node; node; node = node->next)
      visit(node);
}