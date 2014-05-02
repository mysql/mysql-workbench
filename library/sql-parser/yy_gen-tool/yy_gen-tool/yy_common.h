#pragma once

#include "grammar_tree_item.h"

typedef Grammar_tree_item * YYSTYPE;
#define YYSTYPE_IS_DECLARED

#define YYERROR_VERBOSE

#include "parser.tab.hh"

extern FILE *yyin;
extern YYSTYPE yy_result;
extern int yy_lineno;

int yyinit(FILE *input_file);
int yyfreeres();
int yylex();
int yyparse();

void yyerror(const char *msg);
int error(const char *msg);
int warning(const char *msg);

int generate_grammar_file(const Grammar_tree_item* tree, const char *filename);
