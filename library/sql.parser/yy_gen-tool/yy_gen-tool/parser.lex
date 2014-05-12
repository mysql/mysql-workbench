%{

#include "yy_common.h"

%}

IDENT [a-zA-Z][a-zA-Z0-9_]*
CHAR_LITERAL "'"."'"
DELIMITER [\:\;\|\.]
PREC (%[P|p][R|r][E|e][C|c])

%%

[ \t\xD]+

[\n] { yy_lineno++; }

{IDENT} { yylval= new Grammar_tree_item(yytext); return IDENT; }

{CHAR_LITERAL} { yylval= new Grammar_tree_item(yytext); return CHAR_LITERAL; }

{DELIMITER} { return (int) yytext[0]; }

{PREC} { yylval= new Grammar_tree_item(yytext); return PREC; }

"'" { printf("%d\n", yy_lineno); }

%%

int yywrap ()
{
  return 1;
}
