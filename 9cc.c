#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  char *p = argv[1];//argvにはアドレスが格納されている https://dev.grapecity.co.jp/support/powernews/column/clang/028/page02.htm

  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");
  printf("  mov rax, %ld\n", strtol(p, &p, 10));//pというポインタの値が格納されている住所を&pは表している。そしてstrtolはその住所に新たな値を代入している

  while (*p) {//*pはpというポインタが表す住所に入っている値を表す。https://www.sgnet.co.jp/technology/c/6-3/
    if (*p == '+') {
      p++;//これはポインタが表す番地に1を足している
      printf("  add rax, %ld\n", strtol(p, &p, 10));
      continue;
    }

    if (*p == '-') {
      p++;
      printf("  sub rax, %ld\n", strtol(p, &p, 10));
      continue;
    }

    fprintf(stderr, "予期しない文字です: '%c'\n", *p);
    return 1;
  }

  printf("  ret\n");
  return 0;
}