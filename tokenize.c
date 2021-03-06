#include "takuya.h"

char *user_input;
Token *token;

// Reports an error and exit.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

char *strndup(char *p, int len) {
  char *buf = malloc(len + 1);
  strncpy(buf, p, len);
  buf[len] = '\0';
  return buf;
}

// Returns true if the current token matches a given string.//定義された変数かチェック
Token *peek(char *s) {
  if (token->kind != TK_RESERVED || strlen(s) != token->len ||
      memcmp(token->str, s, token->len))
    return NULL;
  return token;
}


// Consumes the current token if it matches a given string.
bool consume(char *s) {
  if (!peek(s))
    return false;
  token = token->next;
  return true;
}
Token *consume_ident() {
   if (token->kind != TK_IDENT)
     return NULL;
   Token *t = token;//Token t = *tokenとかくと、local変数に新しいToken型をコピーしたことになってしまうから駄目
   token = token->next;
   return t;
}

// Ensure that the current token is a given string
void expect(char *s) {
  if (!peek(s))
    error_at(token->str, "expected \"%s\"");
  token = token->next;
}

// Ensure that the current token is TK_NUM.
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "expected a number");
  int val = token->val;
  token = token->next;
  return val;
}

// Ensure that the current token is TK_IDENT.
char *expect_ident() {
  if (token->kind != TK_IDENT)
    error_at(token->str, "expected an identifier");
  char *s = strndup(token->str, token->len);
  token = token->next;
  return s;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// Create a new token and add it as the next token of `cur`.
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

bool is_alpha(char *c){
  return ('a'<=c[0] && c[0] <= 'z') || ('A' <= c[0] && c[0] <= 'Z') || c[0] == '_';
}

bool is_alnum(char *c) {
   return is_alpha(c) || ('0' <= c[0] && c[0] <= '9');
 }
char *starts_with_reserved(char *p) {
  // Keyword
  static char *kw[] = {"return", "if", "else", "while", "for", "int" , "sizeof"};
  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {//https://teratail.com/questions/350067
    int len = strlen(kw[i]);
    if (startswith(p, kw[i]) && !is_alnum(p+len))
      return kw[i];
  }
  // Multi-letter punctuator
  static char *ops[] = {"==", "!=", "<=", ">="};
  for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++)
    if (startswith(p, ops[i]))
      return ops[i];
  return NULL;
}
// Tokenize `user_input` and returns new tokens.
Token *tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    
    // Skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }
    // Keyword or multi-letter punctuator
    char *kw = starts_with_reserved(p);
    if (kw){
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p+=len;
      continue;
    }

    // Single-letter punctuator
    if (strchr("+-*/()<>;={},&[]", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }
    if (is_alpha(p)){
      cur = new_token(TK_IDENT, cur, p++, 1);
      while (is_alnum(p)){
        cur->len++;
        p++;
      }
      continue;
    }
    // Integer literal
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    error_at(p, "invalid token");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
