#include <iostream>
#include <fstream>
#include "yy_common.h"

using namespace std;
extern char *yytext;
extern bool html_gen_mode;

int main(int argc, const char *argv[])
{
  (void) argc;
  (void) argv;

  // process flags
  for (size_t n= 1; n < argc; ++n)
  {
    const char *param= argv[n];
    if (0 == strcmp(param, "-html"))
      ::html_gen_mode= 1;
  }

  yyinit(fopen(argv[argc-2], "r"));

/*
  yytokentype res= (yytokentype) 1;
  while (res != 0)
  {
    res= (yytokentype) yylex();
    cout << yytext << endl;
  }
*/

  (void) yyparse();

  if (NULL != yy_result)
    generate_grammar_file(yy_result, argv[argc-1]);

  yyfreeres();

  return 0;
}
