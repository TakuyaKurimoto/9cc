#include "takuya.h"

// Pushes the given node's address to the stack.
void gen_addr(Node *node) {
  if (node->kind == ND_LVAR) {
    int offset = (node->name - 'a' + 1) * 8;
    printf("  lea rax, [rbp-%d]\n", offset);//raxにrbp-offsetが指すアドレスそのものを転送　https://ja.wikibooks.org/wiki/X86アセンブラ/データ転送命令
    printf("  push rax\n");
    return;
  }

  error("not an lvalue");
}

void load() {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

void store() {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

void gen(Node *node) {
  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_LVAR:
     gen_addr(node);
     load();
     return;
    case ND_ASSIGN:
     gen_addr(node->lhs);
     gen(node->rhs);
     store();
     return;
    case ND_RETURN:
      gen(node->lhs);
      printf("  pop rax\n");
      printf(" jmp .Lreturn\n");
     return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}

void codegen(Node *node) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // プロローグ
  // 変数26個分の領域を確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  for (Node *n=node;n;n=n->next){
      gen(n);
      
  }
  // エピローグ
  // 最後の式の結果がRAXに残っているのでそれが返り値になる
  printf(".Lreturn:\n");
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");

  // A result must be at the top of the stack, so pop it
  // to RAX to make it a program exit code.
  
  printf("  ret\n");
}
