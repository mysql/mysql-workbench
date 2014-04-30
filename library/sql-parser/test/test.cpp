#include "LexerTest.h"

//int main(int argc, char* argv[])
//{
//  LexerTest test;
//  test.testLexer();
//
//  return 0;
//}

#define MYSQL_LEX 1
#define MYSQL_SERVER


//extern "C" 
//{
extern int pthread_dummy(int) { return 0; }
//}

typedef void* YYSTYPE;
#define YYSTYPE_IS_DECLARED

#include <sstream>
#include "myx_lex_helpers.h"

#include "mysql_version.h"
#include "my_global.h"
#include "my_sys.h"
#include "sql_string.h"
#include "unireg.h"

#include "structs.h"
#include <m_ctype.h>
#include <hash.h>

#include "sql_lex.h"


int MYSQLlex(void **arg, void *yylex);
void lex_init(void);
void lex_start(LEX *lex, const uchar *buf, uint length);

int main(int argc, char* argv[])
{
  YYSTYPE yystype;

  lex_init();
  const char *query= "CREATE TABLE test.t1 (id INT NOT NULL PRIMARY KEY, t TEXT)";

  LEX lex;
  lex_start(&lex, reinterpret_cast<const unsigned char *>(query), (unsigned int)strlen(query));
  lex.charset= get_charset_by_name("utf8_bin", MYF(0));

  lex_args.arg1= &yystype;
  lex_args.arg2= &lex;

  yytokentype retval;

  while(1)
  {
    retval= (yytokentype)yylex(&yystype);
    retval= retval;
  }

  //LexerTest test;
  //test.stringParse(query);
  
  return 0;
}

