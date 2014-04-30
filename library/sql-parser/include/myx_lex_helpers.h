#ifndef myx_lex_helpers_h
#define myx_lex_helpers_h

#include <iostream>
#include <fstream>

namespace mysql_parser
{

extern const void *tree;

struct Lex_args
{
  void *arg1;
  void *arg2;
};
extern struct Lex_args lex_args;

extern std::istream* lex_input_stream;

extern int yylex(void **yylval);
extern void yyerror(const char *msg);
//extern int yywrap();
//extern void yy_custom_input(char *buf, int* result, int max_size);
//extern int yy_token_match(int token, const char *value);
//int yy_unknown_token(const char *value);
//extern FILE *yyin;
//extern HANDLE h_file;

} // namespace mysql_parser

extern int yyparse();

#endif // myx_lex_helpers_h
