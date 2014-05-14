#include <stdio.h>
#include <sstream>
#ifdef _WIN32
#include "mysql_sql_parser_public_interface.h"
#endif
#include "myx_sql_parser_public_interface.h"
#include "myx_lex_helpers.h"
#include "myx_sql_parser.tab.hh"
#include "myx_sql_tree_item.h"

//#include "sql_lex.h"

namespace mysql_parser
{

std::istream* lex_input_stream= 0;
static std::string err_msg;
const void* tree= 0;
struct Lex_args lex_args;
extern int MYSQLlex(void **arg, void *yyl);

int yylex(void **yylval) 
{
  //struct Lex_args *p= (struct Lex_args *)ptr_to_arg_pair;
  //return MYSQLlex(lex_args.arg1, lex_args.arg2); 
  
  int state= mysql_parser::MYSQLlex(yylval, mysql_parser::lex_args.arg2);
  //int state= myx_map_lexer_value(MYSQLlex(yylval, lex_args.arg2)); 
  //return state == END_OF_INPUT ? 0 : state;
  return state;
}

void yyerror(const char *msg) { mysql_parser::err_msg= msg; }
/*
int yywrap() { return 1; }  // stop after EOF

void yy_custom_input(char *buf, int* result, int max_size) 
{
  mysql_parser::lex_input_stream->read(buf, max_size);
  *result= mysql_parser::lex_input_stream->gcount();
}

int yy_token_match(int token, const char *value)
{
  return token;
}

int yy_unknown_token(const char *value)
{
  //printf("error %s", value);
  return 0;
}
*/

MYX_PUBLIC_FUNC const std::string & myx_get_err_msg()
{
  return err_msg;
}

MYX_PUBLIC_FUNC const void *myx_get_parser_tree()
{
  return tree;
}

MYX_PUBLIC_FUNC void myx_set_parser_input(std::istream *sqlstream)
{
  lex_input_stream= sqlstream;
}

MYX_PUBLIC_FUNC void myx_set_parser_source(const char *sql)
{
  lex_input_stream= new std::istringstream(sql);
}

MYX_PUBLIC_FUNC void myx_set_parser_source(std::istream *sqlstream)
{
  lex_input_stream= sqlstream;
}

MYX_PUBLIC_FUNC void myx_free_parser_source(void)
{
  delete lex_input_stream;
  SqlAstStatics::cleanup_ast_nodes();
}

MYX_PUBLIC_FUNC void myx_parse(void)
{
  err_msg.clear();
  yyparse();
}


// server replacement routines
//
//extern "C" {

extern char *strmake_root(const char *str, unsigned int len)
{
  char *pos;
  if ((pos=(char *)malloc(len+1)))
  {
    memcpy(pos,str,len);
    pos[len]=0;
  }
  return pos;
}

extern char *strdup_root(const char *str)
{
  return strmake_root(str, (unsigned int) strlen(str));
} 

extern char *alloc_root(unsigned int size)
{
  return (char *)malloc(size);
}

extern char *memdup_root(const char *str, unsigned int len)
{
  char *pos;
  if ((pos=alloc_root(len)))
    memcpy(pos,str,len);
  return pos;
}

//} // extern C

} // namespace mysql_parser
