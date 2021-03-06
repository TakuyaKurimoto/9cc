#include "takuya.h"

int labelseq = 0;
char *funcname;
char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
void gen(Node *node);

// Pushes the given node's address to the stack.
void gen_addr(Node *node) {
  switch (node->kind) {
    case ND_VAR: {
      Var *var = node->var;
      if (var->is_local) {
        printf("  lea rax, [rbp-%d]\n", var->offset);
        printf("  push rax\n");
      } else {
        printf("  push offset %s\n", var->name);
      }
      return;
    }
    case ND_DEREF:
      gen(node->lhs);
      return;
  }

  error("not an lvalue");
}
void gen_lval(Node *node) {
  if (node->ty->kind == TY_ARRAY)
    error_at("", "not an lvalue");
  gen_addr(node);
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
    case ND_NULL:
     return;
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_VAR:
     gen_addr(node);
     if (node->ty->kind != TY_ARRAY)
      load();
     return;
    case ND_ASSIGN:
     gen_lval(node->lhs);
     gen(node->rhs);
     store();
     return;
    case ND_ADDR:
      gen_addr(node->lhs);
      return;
    case ND_DEREF:
      gen(node->lhs);
      if (node->ty->kind != TY_ARRAY)
        load();
      return;
    case ND_RETURN:
      gen(node->lhs);
      printf("  pop rax\n");
      printf(" jmp .Lreturn.%s\n", funcname);
     return;
    case ND_IF: {
     int seq = labelseq++;
     if (node->els) {
       gen(node->cond);
       printf("  pop rax\n");
       printf("  cmp rax, 0\n");//0の時はelse節に飛ぶ
       printf("  je  .Lelse%d\n", seq);
       gen(node->then);
       printf("  jmp .Lend%d\n", seq);
       printf(".Lelse%d:\n", seq);
       gen(node->els);
       printf(".Lend%d:\n", seq);
     } else {
       gen(node->cond);
       printf("  pop rax\n");
       printf("  cmp rax, 0\n");
       printf("  je  .Lend%d\n", seq);
       gen(node->then);
       printf(".Lend%d:\n", seq);//Lから始まるラベルはアセンブラによって特別に認識される名前で、自動的にファイルスコープになります。.Lから始めるようにしておくと、他のファイルに含まれているラベルと衝突する心配がいらなくなります。
     }
     return;
   }
   case ND_WHILE: {
      int seq = labelseq++;  
      printf(".Lbegin%d:\n", seq);   
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", seq);
      gen(node->then);
      printf("  jmp .Lbegin%d\n", seq);
      printf(".Lend%d:\n", seq);
      return;
     }
   case ND_FOR: {
      int seq = labelseq++;  
      if (node->init){
        gen(node->init);
      }
      printf(".Lbegin%d:\n", seq);    
      if (node->cond) {    
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je  .Lend%d\n", seq);
      }
      gen(node->then);
      if (node->inc){
        gen(node->inc);
      }
      printf("  jmp .Lbegin%d\n", seq);
      printf(".Lend%d:\n", seq);
      return;
     }
    case ND_BLOCK: {
      for (Node *n = node->body; n; n = n->next){
       gen(n);
      }
      return;
    }
    case ND_FUNCALL:{
     
     int nargs = 0;
     for (Node *arg = node->args; arg; arg = arg->next) {
       gen(arg);
       nargs++;
     }
     
     for (nargs; nargs > 0; nargs--){
       printf("  pop %s\n", argreg[nargs-1]);
      }
    // We need to align RSP to a 16 byte boundary before
    // calling a function because it is an ABI requirement.
    // RAX is set to 0 for variadic function.
    //関数呼び出しをする前にRSPが16の倍数になっていなければいけない。
    int seq = labelseq++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .Lcall%d\n", seq);//演算の結果が0であればzfフラグがセットされる。jnzはzfフラクがセットされていない時にジャンプする。
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcname);
    printf("  jmp .Lend%d\n", seq);
    printf(".Lcall%d:\n", seq);
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcname);
    printf("  add rsp, 8\n");
    printf(".Lend%d:\n", seq);
    printf("  push rax\n");//関数の戻り値はraxに渡される。この後returnでpop raxするからここでpushしておく
    return;
    }
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
   if (node->ty->base)
     printf("  imul rdi, %d\n", size_of(node->ty->base));
   printf("  add rax, rdi\n");
   break;
 case ND_SUB:
   if (node->ty->base)
     printf("  imul rdi, %d\n", size_of(node->ty->base));
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

void emit_data(Program *prog) {
  printf(".data\n");
  for (VarList *vl = prog->globals; vl; vl = vl->next) {
    Var *var = vl->var;
    printf("%s:\n", var->name);
    printf("  .zero %d\n", size_of(var->ty));
  }
}
void emit_text(Program *prog) {
  printf(".text\n");
  for (Function *fn = prog->fns; fn; fn = fn->next) {  
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);
    funcname = fn->name;

    // プロローグ
    // ローカル変数分の領域を確保する
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf(" sub rsp, %d\n", fn->stack_size);
    // レジスタに入っている引数の値を、ローカル変数の値とみなして、値を保存する
    int i = 0;
    for (VarList *vl = fn->params; vl; vl = vl->next) {
      Var *var = vl->var;

      printf("  mov [rbp-%d], %s\n", var->offset, argreg[i++]);
    }
    for (Node *n=fn->node;n;n=n->next){
        gen(n);

    }
    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値になる
    printf(".Lreturn.%s:\n", funcname);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");

    // A result must be at the top of the stack, so pop it
    // to RAX to make it a program exit code.

    printf("  ret\n");
  }
  
} 
void codegen(Program *prog) {
  printf(".intel_syntax noprefix\n");
  emit_data(prog);
  emit_text(prog);
}
