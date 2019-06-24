/* Copyright (C) 2000-2015 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


/* A lexical scanner on a temporary buffer with a yacc interface */

#ifndef _MSC_VER
  #pragma GCC diagnostic push
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
#if defined(__APPLE__) || defined (_WIN32)
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

#define MYSQL_LEX 1
#define MYSQL_SERVER
#include "myx_sql_tree_item.h"
//#include "mysql_priv.h"
//#include "item_create.h"
//#include "sp.h"
//#include "sp_head.h"
#include "mysql_version.h"
#include "my_global.h"
#include "my_sys.h"
#include "my_base.h"
#include "m_string.h"
#include "sql_string.h"
#include "unireg.h"
#include "structs.h"
#include "sql_lex.h"
#include <m_ctype.h>
#include "myx_lex_helpers.h"
//#include <hash.h>

namespace mysql_parser
{

/*
  We are using pointer to this variable for distinguishing between assignment
  to NEW row field (when parsing trigger definition) and structured variable.
*/

//sys_var *trg_new_row_fake_var= (sys_var*) 0x01;

/* Macros to look like lex */

//#define yyGet()		*(lex->ptr++)
#define yyGet() (((*lex->ptr == '\n') || ((*lex->ptr == '\r') && (*(lex->ptr+1) != '\n'))) ? (++lex->yylineno, *lex->ptr++) : *lex->ptr++)
#define yyGetLast()	lex->ptr[-1]
#define yyPeek()	lex->ptr[0]
#define yyPeek2()	lex->ptr[1]
//#define yyUnget()	lex->ptr--
#define yyUnget() (((*(--lex->ptr) == '\n') || ((*lex->ptr == '\r') && (*(lex->ptr+1) != '\n'))) ? --lex->yylineno : 0)
//#define yySkip()	lex->ptr++
#define yySkip() (((*lex->ptr == '\n') || ((*lex->ptr == '\r') && (*(lex->ptr+1) != '\n'))) ? (++lex->yylineno, ++lex->ptr) : ++lex->ptr)
#define yyLength()	((uint) (lex->ptr - lex->tok_start)-1)

/* Longest standard keyword name */
#define TOCK_NAME_LENGTH 24

/*
  The following data is based on the latin1 character set, and is only
  used when comparing keywords
*/

static uchar to_upper_lex[]=
{
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
   16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
   32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
   48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
   64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
   80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
   96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
   80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
  208,209,210,211,212,213,214,247,216,217,218,219,220,221,222,255
};


inline int lex_casecmp(const char *s, const char *t, uint len)
{
  while (len-- != 0 &&
	 to_upper_lex[(uchar) *s++] == to_upper_lex[(uchar) *t++]) ;
  return (int) len+1;
}

} // namespace mysql_parser


#include "lex.h"
namespace mysql_parser
{

static inline SYMBOL *get_hash_symbol(const char *s, unsigned int len, bool function)
{
#if defined(__APPLE__) || defined(_MSC_VER)
  typedef std::unordered_multimap<size_t, SYMBOL *> Hash_ind;
#else
  typedef std::tr1::unordered_multimap<size_t, SYMBOL *> Hash_ind;
#endif
  typedef std::pair<Hash_ind::const_iterator, Hash_ind::const_iterator> Hash_ind_range;
  static Hash_ind sym_hash_ind;
  static Hash_ind::const_iterator sym_hash_ind_end;
  static Hash_ind func_hash_ind;
  static Hash_ind::const_iterator func_hash_ind_end;

  class Symbols_initializer
  {
  public:
    Symbols_initializer()
    {
      add_from_list(&sym_hash_ind, symbols, sizeof(symbols)/sizeof(SYMBOL));
      sym_hash_ind_end= sym_hash_ind.end();

      add_from_list(&func_hash_ind, sql_functions, sizeof(sql_functions)/sizeof(SYMBOL));
      // things like "varchar(N)" are checked for reserved function names only.
      // adding keywords to the list of functions is a workaround. not sure if this is a proper way.
      add_from_list(&func_hash_ind, symbols, sizeof(symbols)/sizeof(SYMBOL));
      func_hash_ind_end= func_hash_ind.end();
    }
  private:
    void add_from_list(Hash_ind *hash_ind, SYMBOL sym_arr[], size_t sym_arr_sz)
    {
      SYMBOL *sym= sym_arr;
      for (size_t n= 0; sym_arr_sz > n; ++n, ++sym)
      {
        size_t sym_key= sym->length << 16;
        sym_key|= sym->name[0] << 8;
        sym_key|= sym->name[sym->length-1];

        hash_ind->insert(std::make_pair(sym_key, sym));
      }
    }
  };
  static Symbols_initializer symbols_initializer;

  size_t sym_key= len << 16;
  sym_key|= toupper(s[0]) << 8;
  sym_key|= toupper(s[len-1]);

  register Hash_ind *hash_ind;
  register Hash_ind::const_iterator *hash_ind_end;
  if (function)
  {
    hash_ind= &func_hash_ind;
    hash_ind_end= &func_hash_ind_end;
  }
  else
  {
    hash_ind= &sym_hash_ind;
    hash_ind_end= &sym_hash_ind_end;
  }

  Hash_ind_range range= hash_ind->equal_range(sym_key);
  if (range.first != *hash_ind_end)
  {
    if (3 > len)
      return range.first->second;

    for (; range.first != range.second; ++range.first)
      if (0 == strncasecmp(s+1, range.first->second->name+1, len-2))
        return range.first->second;
  }

  return NULL;
}

void lex_init(void)
{
  uint i;
  DBUG_ENTER("lex_init");
  for (i=0 ; i < array_elements(symbols) ; i++)
    symbols[i].length=(uchar) strlen(symbols[i].name);
  for (i=0 ; i < array_elements(sql_functions) ; i++)
    sql_functions[i].length=(uchar) strlen(sql_functions[i].name);

  DBUG_VOID_RETURN;
}


void lex_free(void)
{					// Call this when daemon ends
  DBUG_ENTER("lex_free");
  DBUG_VOID_RETURN;
}


/*
  This is called before every query that is to be parsed.
  Because of this, it's critical to not do too much things here.
  (We already do too much here)
*/

void lex_start(/*THD *thd, */LEX *lex, const uchar *buf, uint length)
{
  //LEX *lex= thd->lex;

  memset(lex, 0, sizeof(*lex));

  DBUG_ENTER("lex_start");

  //lex->thd= lex->unit.thd= thd;
  lex->buf= lex->ptr= buf;
  lex->end_of_query= buf+length;

  //lex->context_stack.empty();
  //lex->unit.init_query();
  //lex->unit.init_select();
  /* 'parent_lex' is used in init_query() so it must be before it. */
  //lex->select_lex.parent_lex= lex;
  //lex->select_lex.init_query();
  //lex->value_list.empty();
  //lex->update_list.empty();
  //lex->param_list.empty();
  //lex->view_list.empty();
  //lex->prepared_stmt_params.empty();
  //lex->auxiliary_table_list.empty();
  //lex->unit.next= lex->unit.master=
  //  lex->unit.link_next= lex->unit.return_to= 0;
  //lex->unit.prev= lex->unit.link_prev= 0;
  //lex->unit.slave= lex->unit.global_parameters= lex->current_select=
  //  lex->all_selects_list= &lex->select_lex;
  //lex->select_lex.master= &lex->unit;
  //lex->select_lex.prev= &lex->unit.slave;
  //lex->select_lex.link_next= lex->select_lex.slave= lex->select_lex.next= 0;
  //lex->select_lex.link_prev= (st_select_lex_node**)&(lex->all_selects_list);
  //lex->select_lex.options= 0;
  //lex->select_lex.sql_cache= SELECT_LEX::SQL_CACHE_UNSPECIFIED;
  //lex->select_lex.init_order();
  //lex->select_lex.group_list.empty();
  lex->describe= 0;
  lex->subqueries= FALSE;
  lex->view_prepare_mode= FALSE;
  lex->stmt_prepare_mode= FALSE;
  lex->derived_tables= 0;
  //lex->lock_option= TL_READ;
  lex->found_semicolon= 0;
  lex->safe_to_cache_query= 1;
  //lex->leaf_tables_insert= 0;
  //lex->parsing_options.reset();
  lex->variables_used= 0;
  lex->empty_field_list_on_rset= 0;
  //lex->select_lex.select_number= 1;
  lex->next_state=MY_LEX_START;
  lex->yylineno = 1;
  lex->in_comment=0;
  lex->length=0;
  lex->part_info= 0;
  //lex->select_lex.in_sum_expr=0;
  //lex->select_lex.expr_list.empty();
  //lex->select_lex.ftfunc_list_alloc.empty();
  //lex->select_lex.ftfunc_list= &lex->select_lex.ftfunc_list_alloc;
  //lex->select_lex.group_list.empty();
  //lex->select_lex.order_list.empty();
  lex->yacc_yyss=lex->yacc_yyvs=0;
  lex->ignore_space= 0;//lex->sql_mode.MODE_IGNORE_SPACE;
  lex->sql_command= lex->orig_sql_command= SQLCOM_END;
  //lex->duplicates= DUP_ERROR;
  lex->ignore= 0;
  lex->sphead= NULL;
  lex->spcont= NULL;
  //lex->proc_list.first= 0;
  lex->escape_used= lex->et_compile_phase= FALSE;
  //lex->query_tables= 0;
  //lex->reset_query_tables_list(FALSE);
  //lex->expr_allows_subselect= TRUE;

  lex->name= 0;
  //lex->name.length= 0;
  //lex->event_parse_data= NULL;
  lex->et= NULL;

  lex->nest_level=0 ;
  //lex->allow_sum_func= 0;
  //lex->in_sum_func= NULL;
  /*
    ok, there must be a better solution for this, long-term
    I tried "bzero" in the sql_yacc.yy code, but that for
    some reason made the values zero, even if they were set
  */
  //lex->server_options.server_name= 0;
  //lex->server_options.server_name_length= 0;
  //lex->server_options.host= 0;
  //lex->server_options.db= 0;
  //lex->server_options.username= 0;
  //lex->server_options.password= 0;
  //lex->server_options.scheme= 0;
  //lex->server_options.socket= 0;
  //lex->server_options.owner= 0;
  //lex->server_options.port= -1;
  lex->binlog_row_based_if_mixed= 0;
  DBUG_VOID_RETURN;
}

void lex_end(LEX *lex)
{
#if 0
  DBUG_ENTER("lex_end");
  DBUG_PRINT("enter", ("lex: 0x%lx", (long) lex));
  if (lex->yacc_yyss)
  {
    my_free(lex->yacc_yyss, MYF(0));
    my_free(lex->yacc_yyvs, MYF(0));
    lex->yacc_yyss= 0;
    lex->yacc_yyvs= 0;
  }

  /* release used plugins */
  plugin_unlock_list(0, (plugin_ref*)lex->plugins.buffer, 
                     lex->plugins.elements);
  reset_dynamic(&lex->plugins);

  DBUG_VOID_RETURN;
#endif
}

int token_start_lineno;
inline SqlAstNode * new_ast_terminal_node(LEX *lex, const char* value, int value_length, char *lex_string_to_free)
{
  if (SqlAstStatics::is_ast_generation_enabled)
  {
    lex->last_item= *lex->yylval= SqlAstStatics::add_ast_node(new SqlAstTerminalNode(
      value,
      value_length,
      token_start_lineno,
      /*stmt_boffset*/(int)(lex->tok_start - lex->buf),
      /*stmt_eoffset*/(int)(lex->ptr - lex->buf)));
    if (!lex->first_item)
      lex->first_item= lex->last_item;
    free(lex_string_to_free);
    return lex->last_item;
  }
  else
  {
    std::shared_ptr<SqlAstTerminalNode> node(new SqlAstTerminalNode(
        value,
        value_length,
        token_start_lineno,
        /*stmt_boffset*/(int)(lex->tok_start - lex->buf),
        /*stmt_eoffset*/(int)(lex->ptr - lex->buf)));
    
    lex->last_item = node.get();
    SqlAstStatics::last_terminal_node(node);
    
    if (!lex->first_item)
    {
      lex->first_item = node.get();
      SqlAstStatics::first_terminal_node(node);
    }
    
    free(lex_string_to_free);
    return NULL;
  }
}

inline SqlAstNode * new_ast_terminal_node(LEX *lex, int value_length, char *lex_string_to_free)
{
  return new_ast_terminal_node(lex, NULL, value_length, lex_string_to_free);
}

static int find_keyword(LEX *lex, uint len, bool function)
{
  const uchar *tok=lex->tok_start;

  SYMBOL *symbol= get_hash_symbol((const char *)tok,len,function);
  if (symbol)
  {
    new_ast_terminal_node(lex, /*(const char*) tok, */len, 0);
#if 0
    lex->yylval->symbol.symbol=symbol;
    lex->yylval->symbol.str= (char*) tok;
    lex->yylval->symbol.length=len;
#endif
    //if ((symbol->tok == NOT_SYM) && (lex->thd->variables.sql_mode & MODE_HIGH_NOT_PRECEDENCE))
    if ((symbol->tok == NOT_SYM) && lex->sql_mode.MODE_HIGH_NOT_PRECEDENCE)
      return NOT2_SYM;
    //if ((symbol->tok == OR_OR_SYM) && !(lex->thd->variables.sql_mode & MODE_PIPES_AS_CONCAT))
    if ((symbol->tok == OR_OR_SYM) && !lex->sql_mode.MODE_PIPES_AS_CONCAT)
      return OR2_SYM;

    return symbol->tok;
  }
  return 0;
}

/*
  Check if name is a keyword

  SYNOPSIS
    is_keyword()
    name      checked name (must not be empty)
    len       length of checked name

  RETURN VALUES
    0         name is a keyword
    1         name isn't a keyword
*/

bool is_keyword(const char *name, uint len)
{
  DBUG_ASSERT(len != 0);
  return get_hash_symbol(name,len,0)!=0;
}

/* make a copy of token before ptr and set yytoklen */

static LEX_STRING get_token(LEX *lex,uint length)
{
  LEX_STRING tmp;
  yyUnget();			// ptr points now after last token char
  tmp.length=lex->yytoklen=length;
  //tmp.str=(char*) lex->thd->strmake((char*) lex->tok_start,tmp.length);
  tmp.str= strmake_root((char*) lex->tok_start,tmp.length);
  return tmp;
}

/* 
 todo: 
   There are no dangerous charsets in mysql for function 
   get_quoted_token yet. But it should be fixed in the 
   future to operate multichar strings (like ucs2)
*/

static LEX_STRING get_quoted_token(LEX *lex,uint length, char quote)
{
  LEX_STRING tmp;
  const uchar *from, *end;
  uchar *to;
  yyUnget();			// ptr points now after last token char
  tmp.length=lex->yytoklen=length;
  //tmp.str=(char*) lex->thd->alloc(tmp.length+1);
  tmp.str=(char*) malloc(tmp.length+1);
  for (from= lex->tok_start, to= (uchar*) tmp.str, end= to+length ;
       to != end ;
       )
  {
    if ((*to++= *from++) == (uchar) quote)
      from++;					// Skip double quotes
  }
  *to= 0;					// End null for safety
  return tmp;
}


/*
  Return an unescaped text literal without quotes
  Fix sometimes to do only one scan of the string
*/

static char *get_text(LEX *lex)
{
  reg1 uchar c,sep;
  uint found_escape=0;
  CHARSET_INFO *cs= lex->charset;//lex->thd->charset();

  sep= yyGetLast();			// String should end with this
  while (lex->ptr != lex->end_of_query)
  {
    c = yyGet();
#ifdef USE_MB
    int l;
    if (use_mb(cs) &&
        (l = my_ismbchar(cs,
                         (const char *)lex->ptr-1,
                         (const char *)lex->end_of_query))) {
	lex->ptr += l-1;
	continue;
    }
#endif
    if (c == '\\' &&
  //!(lex->thd->variables.sql_mode & MODE_NO_BACKSLASH_ESCAPES))
	!lex->sql_mode.MODE_NO_BACKSLASH_ESCAPES)
    {					// Escaped character
      found_escape=1;
      if (lex->ptr == lex->end_of_query)
	return 0;
#ifdef USE_MB
      int l;
      if (use_mb(cs) &&
          (l = my_ismbchar(cs,
                           (const char *)lex->ptr,
                           (const char *)lex->end_of_query))) {
          lex->ptr += l;
          continue;
      }
      else
#endif
        yySkip();
    }
    else if (c == sep)
    {
      if (c == yyGet())			// Check if two separators in a row
      {
	found_escape=1;			// dupplicate. Remember for delete
	continue;
      }
      else
	yyUnget();

      /* Found end. Unescape and return string */
      const uchar *str, *end;
      uchar *start;

      str=lex->tok_start+1;
      end=lex->ptr-1;
      //if (!(start=(uchar*) lex->thd->alloc((uint) (end-str)+1)))
      if (!(start=(uchar*) malloc((uint) (end-str)+1)))
	return 0;//(char*) "";		// Sql_alloc has set error flag
      if (!found_escape)
      {
	lex->yytoklen=(uint) (end-str);
	memcpy(start,str,lex->yytoklen);
	start[lex->yytoklen]=0;
      }
      else
      {
	uchar *to;

        /* Re-use found_escape for tracking state of escapes */
        found_escape= 0;

	for (to=start ; str != end ; str++)
	{
#ifdef USE_MB
	  int l;
	  if (use_mb(cs) &&
              (l = my_ismbchar(cs,
                               (const char *)str, (const char *)end))) {
	      while (l--)
		  *to++ = *str++;
	      str--;
	      continue;
	  }
#endif
	  if (!found_escape &&
      //!(lex->thd->variables.sql_mode & MODE_NO_BACKSLASH_ESCAPES) &&
      !lex->sql_mode.MODE_NO_BACKSLASH_ESCAPES &&
              *str == '\\' && str+1 != end)
	  {
	    switch(*++str) {
	    case 'n':
	      *to++='\n';
	      break;
	    case 't':
	      *to++= '\t';
	      break;
	    case 'r':
	      *to++ = '\r';
	      break;
	    case 'b':
	      *to++ = '\b';
	      break;
	    case '0':
	      *to++= 0;			// Ascii null
	      break;
	    case 'Z':			// ^Z must be escaped on Win32
	      *to++='\032';
	      break;
	    case '_':
	    case '%':
	      *to++= '\\';		// remember prefix for wildcard
	      /* Fall through */
	    default:
              found_escape= 1;
              str--;
	      break;
	    }
	  }
	  else if (!found_escape && *str == sep)
          {
            found_escape= 1;
          }
	  else
          {
	    *to++ = *str;
            found_escape= 0;
          }
	}
	*to=0;
	lex->yytoklen=(uint) (to-start);
      }
      return (char*) start;
    }
  }
  return 0;					// unexpected end of query
}


/*
** Calc type of integer; long integer, longlong integer or real.
** Returns smallest type that match the string.
** When using unsigned long long values the result is converted to a real
** because else they will be unexpected sign changes because all calculation
** is done with longlong or double.
*/

static const char *long_str="2147483647";
static const uint long_len=10;
static const char *signed_long_str="-2147483648";
static const char *longlong_str="9223372036854775807";
static const uint longlong_len=19;
static const char *signed_longlong_str="-9223372036854775808";
static const uint signed_longlong_len=19;
static const char *unsigned_longlong_str="18446744073709551615";
static const uint unsigned_longlong_len=20;

static inline uint int_token(const char *str,uint length)
{
  if (length < long_len)			// quick normal case
    return NUM;
  bool neg=0;

  if (*str == '+')				// Remove sign and pre-zeros
  {
    str++; length--;
  }
  else if (*str == '-')
  {
    str++; length--;
    neg=1;
  }
  while (*str == '0' && length)
  {
    str++; length --;
  }
  if (length < long_len)
    return NUM;

  uint smaller,bigger;
  const char *cmp;
  if (neg)
  {
    if (length == long_len)
    {
      cmp= signed_long_str+1;
      smaller=NUM;				// If <= signed_long_str
      bigger=LONG_NUM;				// If >= signed_long_str
    }
    else if (length < signed_longlong_len)
      return LONG_NUM;
    else if (length > signed_longlong_len)
      return DECIMAL_NUM;
    else
    {
      cmp=signed_longlong_str+1;
      smaller=LONG_NUM;				// If <= signed_longlong_str
      bigger=DECIMAL_NUM;
    }
  }
  else
  {
    if (length == long_len)
    {
      cmp= long_str;
      smaller=NUM;
      bigger=LONG_NUM;
    }
    else if (length < longlong_len)
      return LONG_NUM;
    else if (length > longlong_len)
    {
      if (length > unsigned_longlong_len)
        return DECIMAL_NUM;
      cmp=unsigned_longlong_str;
      smaller=ULONGLONG_NUM;
      bigger=DECIMAL_NUM;
    }
    else
    {
      cmp=longlong_str;
      smaller=LONG_NUM;
      bigger= ULONGLONG_NUM;
    }
  }
  while (*cmp && *cmp++ == *str++) ;
  return ((uchar) str[-1] <= (uchar) cmp[-1]) ? smaller : bigger;
}

bool parser_is_stopped;

/*
  MYSQLlex remember the following states from the following MYSQLlex()

  - MY_LEX_EOQ			Found end of query
  - MY_LEX_OPERATOR_OR_IDENT	Last state was an ident, text or number
				(which can't be followed by a signed number)
*/

//int MYSQLlex(void *arg, void *yythd)
int MYSQLlex(void **arg, void *yyl)
{
  reg1	uchar c = 0;
  int	tokval, result_state;
  uint length;
  enum my_lex_states state;
  LEX_STRING tmp_lex_string;
  
  //LEX	*lex= ((THD *)yythd)->lex;
  LEX *lex= (LEX *)yyl;

  SqlAstNode **yylval= (SqlAstNode **) arg;
  
  //CHARSET_INFO *cs= ((THD *) yythd)->charset();
  CHARSET_INFO *cs= lex->charset;
  
  uchar *state_map= cs->state_map;
  uchar *ident_map= cs->ident_map;

  lex->yylval=yylval;			// The global state

  token_start_lineno= lex->yylineno;

  lex->tok_end_prev= lex->tok_end;
  lex->tok_start_prev= lex->tok_start;

  lex->tok_start=lex->tok_end=lex->ptr;
  state=lex->next_state;
  lex->next_state=MY_LEX_OPERATOR_OR_IDENT;
  for (;;)
  {
    if (parser_is_stopped)
      break;

    switch (state) {
    case MY_LEX_OPERATOR_OR_IDENT:	// Next is operator or keyword
    case MY_LEX_START:			// Start of token
      // Skip startspace
      for (c=yyGet() ; state_map[c] == MY_LEX_SKIP ; c= yyGet()) ;
      lex->tok_start=lex->ptr-1;	// Start of real token
      state= (enum my_lex_states) state_map[c];
      token_start_lineno= lex->yylineno;
      break;
    case MY_LEX_ESCAPE:
      if (yyGet() == 'N')
      {					// Allow \N as shortcut for NULL
        new_ast_terminal_node(lex, /*"\\N", */2, 0);
#if 0
        yylval->lex_str.str=(char*) "\\N";
      	yylval->lex_str.length=2;
#endif
	      return NULL_SYM;
      }
    case MY_LEX_CHAR:			// Unknown or single char token
    case MY_LEX_SKIP:			// This should not happen
      if (c == '-' && yyPeek() == '-' &&
          (my_isspace(cs,yyPeek2()) || 
           my_iscntrl(cs,yyPeek2())))
      {
        state=MY_LEX_COMMENT;
        break;
      }
      lex->ptr= lex->tok_start;//
      new_ast_terminal_node(lex, /*(const char*) (lex->ptr=lex->tok_start), */1, 0);
#if 0
      yylval->lex_str.str=(char*) (lex->ptr=lex->tok_start);// Set to first chr
      yylval->lex_str.length=1;
#endif
      c=yyGet();
      if (c != ')')
	lex->next_state= MY_LEX_START;	// Allow signed numbers
      if (c == ',')
	lex->tok_start=lex->ptr;	// Let tok_start point at next item
      /*
        Check for a placeholder: it should not precede a possible identifier
        because of binlogging: when a placeholder is replaced with
        its value in a query for the binlog, the query must stay
        grammatically correct.
      */
      else if (c == '?' && lex->stmt_prepare_mode && !ident_map[yyPeek()])
        return(PARAM_MARKER);
      return((int) c);

    case MY_LEX_IDENT_OR_NCHAR:
      if (yyPeek() != '\'')
      {					// Found x'hex-number'
	state= MY_LEX_IDENT;
	break;
      }
      (void) yyGet();				// Skip '
      while ((c = yyGet()) && (c !='\'')) ;
      length=(uint)(lex->ptr - lex->tok_start);	// Length of hexnum+3
      if (c != '\'')
      {
	return(ABORT_SYM);		// Illegal hex constant
      }
      (void) yyGet();				// get_token makes an unget
      tmp_lex_string= get_token(lex,length);
      new_ast_terminal_node(lex, /*tmp_lex_string.str+2, */tmp_lex_string.length-3, tmp_lex_string.str);
#if 0
      yylval->lex_str=get_token(lex,length);
      yylval->lex_str.str+=2;		// Skip x'
      yylval->lex_str.length-=3;	// Don't count x' and last '
#endif
      lex->yytoklen-=3;
      return (NCHAR_STRING);

    case MY_LEX_IDENT_OR_HEX:
      if (yyPeek() == '\'')
      {					// Found x'hex-number'
	state= MY_LEX_HEX_NUMBER;
	break;
      }
    case MY_LEX_IDENT_OR_BIN:
      if (yyPeek() == '\'')
      {                                 // Found b'bin-number'
        state= MY_LEX_BIN_NUMBER;
        break;
      }
    case MY_LEX_IDENT:
      const uchar *start;
#if defined(USE_MB) && defined(USE_MB_IDENT)
      if (use_mb(cs))
      {
	result_state= IDENT_QUOTED;
  //result_state= QUOTED;
        if (my_mbcharlen(cs, yyGetLast()) > 1)
        {
          int l = my_ismbchar(cs,
                              (const char *)lex->ptr-1,
                              (const char *)lex->end_of_query);
          if (l == 0) {
            state = MY_LEX_CHAR;
            continue;
          }
          lex->ptr += l - 1;
        }
        while (ident_map[c=yyGet()])
        {
          if (my_mbcharlen(cs, c) > 1)
          {
            int l;
            if ((l = my_ismbchar(cs,
                              (const char *)lex->ptr-1,
                              (const char *)lex->end_of_query)) == 0)
              break;
            lex->ptr += l-1;
          }
        }
      }
      else
#endif
      {
        for (result_state= c; ident_map[c= yyGet()]; result_state|= c) ;
        /* If there were non-ASCII characters, mark that we must convert */
        result_state= result_state & 0x80 ? IDENT_QUOTED : IDENT;
      }
      length= (uint) (lex->ptr - lex->tok_start)-1;
      start= lex->ptr;
      if (lex->ignore_space)
      {
        /*
          If we find a space then this can't be an identifier. We notice this
          below by checking start != lex->ptr.
        */
        for (; state_map[c] == MY_LEX_SKIP ; c= yyGet()) ;
      }
      if ((start == lex->ptr) && (c == '.') && (ident_map[yyPeek()]))
      	lex->next_state=MY_LEX_IDENT_SEP;
      else
      {					// '(' must follow directly if function
	      yyUnget();
	      if ((tokval = find_keyword(lex,length,c == '(')))
	      {
	        lex->next_state= MY_LEX_START;	// Allow signed numbers
	        return(tokval);		// Was keyword
	      }
	      yySkip();			// next state does a unget
      }
      tmp_lex_string= get_token(lex,length);
      new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, 0);
#if 0
      yylval->lex_str=get_token(lex,length);
#endif
      /* 
         Note: "SELECT _bla AS 'alias'"
         _bla should be considered as a IDENT if charset haven't been found.
         So we don't use MYF(MY_WME) with get_charset_by_csname to avoid 
         producing an error.
      */

      //if ((yylval->lex_str.str[0]=='_') && 
      //    (lex->charset=get_charset_by_csname(yylval->lex_str.str+1,
					 //     MY_CS_PRIMARY,MYF(0))))
      //  return(UNDERSCORE_CHARSET);

      if (tmp_lex_string.str[0]=='_')
      {
        CHARSET_INFO *charset= get_charset_by_csname(tmp_lex_string.str+1, MY_CS_PRIMARY, MYF(0));
        if (charset)
        {
          /*
          serg (WB rev-eng context):
          don't change charset, because nobody will set it back. rev-eng staff works only with utf8 charset.
          */

          //lex->charset= charset;
          free(tmp_lex_string.str);
          return (UNDERSCORE_CHARSET);
        }
      }

      free(tmp_lex_string.str);
      return(result_state);			// IDENT or IDENT_QUOTED

    case MY_LEX_IDENT_SEP:		// Found ident and now '.'
      new_ast_terminal_node(lex, /*(char*)lex->ptr, */1, 0);
#if 0
      yylval->lex_str.str=(char*) lex->ptr;
      yylval->lex_str.length=1;
#endif
      c=yyGet();			// should be '.'
      lex->next_state= MY_LEX_IDENT_START;// Next is an ident (not a keyword)
      if (!ident_map[yyPeek()])		// Probably ` or "
	lex->next_state= MY_LEX_START;
      return((int) c);

    case MY_LEX_NUMBER_IDENT:		// number or ident which num-start
      while (my_isdigit(cs,(c = yyGet()))) ;
      if (!ident_map[c])
      {					// Can't be identifier
	state=MY_LEX_INT_OR_REAL;
	break;
      }
      if (c == 'e' || c == 'E')
      {
	// The following test is written this way to allow numbers of type 1e1
	if (my_isdigit(cs,yyPeek()) || 
            (c=(yyGet())) == '+' || c == '-')
	{				// Allow 1E+10
	  if (my_isdigit(cs,yyPeek()))	// Number must have digit after sign
	  {
	    yySkip();
	    while (my_isdigit(cs,yyGet()))
	      ;
      tmp_lex_string=get_token(lex,yyLength());
      new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, 0);
#if 0
      yylval->lex_str=get_token(lex,yyLength());
#endif
	    return(FLOAT_NUM);
	  }
	}
	yyUnget(); /* purecov: inspected */
      }
      else if (c == 'x' && (lex->ptr - lex->tok_start) == 2 &&
	  lex->tok_start[0] == '0' )
      {						// Varbinary
	while (my_isxdigit(cs,(c = yyGet()))) ;
	if ((lex->ptr - lex->tok_start) >= 4 && !ident_map[c])
	{
    tmp_lex_string= get_token(lex,yyLength());
    new_ast_terminal_node(lex, /*tmp_lex_string.str + 2, */tmp_lex_string.length, tmp_lex_string.str);
#if 0
    yylval->lex_str=get_token(lex,yyLength());
	  yylval->lex_str.str+=2;		// Skip 0x
	  yylval->lex_str.length-=2;
#endif
    //lex->yytoklen-=2;
	  return (HEX_NUM);
	}
	yyUnget();
      }
      else if (c == 'b' && (lex->ptr - lex->tok_start) == 2 &&
               lex->tok_start[0] == '0' )
      {						// b'bin-number'
	while (my_isxdigit(cs,(c = yyGet()))) ;
	if ((lex->ptr - lex->tok_start) >= 4 && !ident_map[c])
	{
    tmp_lex_string= get_token(lex,yyLength());
    new_ast_terminal_node(lex, /*tmp_lex_string.str + 2, */tmp_lex_string.length - 2, tmp_lex_string.str);
#if 0
	  yylval->lex_str= get_token(lex, yyLength());
	  yylval->lex_str.str+= 2;		// Skip 0x
	  yylval->lex_str.length-= 2;
#endif
    lex->yytoklen-= 2;
	  return (BIN_NUM);
	}
	yyUnget();
      }
      // fall through
    case MY_LEX_IDENT_START:			// We come here after '.'
      result_state= IDENT;
#if defined(USE_MB) && defined(USE_MB_IDENT)
      if (use_mb(cs))
      {
	result_state= IDENT_QUOTED;
        while (ident_map[c=yyGet()])
        {
          if (my_mbcharlen(cs, c) > 1)
          {
            int l;
            if ((l = my_ismbchar(cs,
                                 (const char *)lex->ptr-1,
                                 (const char *)lex->end_of_query)) == 0)
              break;
            lex->ptr += l-1;
          }
        }
      }
      else
#endif
      {
        for (result_state=0; ident_map[c= yyGet()]; result_state|= c) ;
        /* If there were non-ASCII characters, mark that we must convert */
        result_state= result_state & 0x80 ? IDENT_QUOTED : IDENT;
      }
      if (c == '.' && ident_map[yyPeek()])
	lex->next_state=MY_LEX_IDENT_SEP;// Next is '.'

      tmp_lex_string= get_token(lex,yyLength());
      new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, tmp_lex_string.str);
#if 0
      yylval->lex_str= get_token(lex,yyLength());
#endif
      return(result_state);

    case MY_LEX_USER_VARIABLE_DELIMITER:	// Found quote char
    {
      uint double_quotes= 0;
      char quote_char= c;                       // Used char
      lex->tok_start=lex->ptr;			// Skip first `
      while ((c=yyGet()))
      {
	int length;
	if ((length= my_mbcharlen(cs, c)) == 1)
	{
	  if (c == (uchar) NAMES_SEP_CHAR)
	    break; /* Old .frm format can't handle this char */
	  if (c == quote_char)
	  {
	    if (yyPeek() != quote_char)
	      break;
	    c=yyGet();
	    double_quotes++;
	    continue;
	  }
	}
#ifdef USE_MB
	else if (length < 1)
	  break;				// Error
	lex->ptr+= length-1;
#endif
      }
      if (double_quotes)
      {
        tmp_lex_string= get_quoted_token(lex,yyLength() - double_quotes, quote_char);
        new_ast_terminal_node(lex, tmp_lex_string.str, lex->yytoklen, tmp_lex_string.str);
#if 0
	      yylval->lex_str=get_quoted_token(lex,yyLength() - double_quotes,
					 quote_char);
#endif
      }
      else
      {
        tmp_lex_string= get_token(lex,yyLength());
        new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, tmp_lex_string.str);
#if 0
        yylval->lex_str=get_token(lex,yyLength());
#endif
      }
      if (c == quote_char)
	yySkip();			// Skip end `
      lex->next_state= MY_LEX_START;
      return(IDENT_QUOTED);
    }
    case MY_LEX_INT_OR_REAL:		// Compleat int or incompleat real
      if (c != '.')
      {					// Found complete integer number.
        tmp_lex_string= get_token(lex,yyLength());
        new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, 0);
        uint tok= int_token(tmp_lex_string.str, tmp_lex_string.length);
        free(tmp_lex_string.str);
        return tok;
#if 0
        yylval->lex_str=get_token(lex,yyLength());
      	return int_token(yylval->lex_str.str,yylval->lex_str.length);
#endif
      }
      // fall through
    case MY_LEX_REAL:			// Incomplete real number
      while (my_isdigit(cs,c = yyGet())) ;

      if (c == 'e' || c == 'E')
      {
	c = yyGet();
	if (c == '-' || c == '+')
	  c = yyGet();			// Skip sign
	if (!my_isdigit(cs,c))
	{				// No digit after sign
	  state= MY_LEX_CHAR;
	  break;
	}
	while (my_isdigit(cs,yyGet()))
	  ;
  tmp_lex_string= get_token(lex,yyLength());
  new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, tmp_lex_string.str);
#if 0
  yylval->lex_str=get_token(lex,yyLength());
#endif
  return(FLOAT_NUM);
      }
      tmp_lex_string= get_token(lex,yyLength());
      new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, tmp_lex_string.str);
#if 0
      yylval->lex_str=get_token(lex,yyLength());
#endif
      return(DECIMAL_NUM);

    case MY_LEX_HEX_NUMBER:		// Found x'hexstring'
      (void) yyGet();				// Skip '
      while (my_isxdigit(cs,(c = yyGet()))) ;
      length=(uint)(lex->ptr - lex->tok_start);	// Length of hexnum+3
      if (!(length & 1) || c != '\'')
      {
	return(ABORT_SYM);		// Illegal hex constant
      }
      (void) yyGet();				// get_token makes an unget
      tmp_lex_string= get_token(lex,yyLength());
      new_ast_terminal_node(lex, /*tmp_lex_string.str + 2, */tmp_lex_string.length - 3, tmp_lex_string.str);
#if 0
      yylval->lex_str=get_token(lex,length);
      yylval->lex_str.str+=2;		// Skip x'
      yylval->lex_str.length-=3;	// Don't count x' and last '
#endif
      lex->yytoklen-=3;
      return (HEX_NUM);

    case MY_LEX_BIN_NUMBER:           // Found b'bin-string'
      (void) yyGet();                                // Skip '
      while ((c= yyGet()) == '0' || c == '1') ;
      length= (uint)(lex->ptr - lex->tok_start);    // Length of bin-num + 3
      if (c != '\'')
      return(ABORT_SYM);              // Illegal hex constant
      (void) yyGet();                        // get_token makes an unget
      tmp_lex_string= get_token(lex,yyLength());
      new_ast_terminal_node(lex, /*tmp_lex_string.str + 2, */tmp_lex_string.length - 3, tmp_lex_string.str);
#if 0
      yylval->lex_str= get_token(lex, length);
      yylval->lex_str.str+= 2;        // Skip b'
      yylval->lex_str.length-= 3;     // Don't count b' and last '
#endif
      lex->yytoklen-= 3;
      return (BIN_NUM); 

    case MY_LEX_CMP_OP:			// Incomplete comparison operator
      if (state_map[yyPeek()] == MY_LEX_CMP_OP || state_map[yyPeek()] == MY_LEX_LONG_CMP_OP)
	      yySkip();
      if ((tokval = find_keyword(lex,(uint) (lex->ptr - lex->tok_start),0)))
      {
	      lex->next_state= MY_LEX_START;	// Allow signed numbers
	      return(tokval);
      }
      state = MY_LEX_CHAR;		// Something fishy found
      break;

    case MY_LEX_LONG_CMP_OP:		// Incomplete comparison operator
      if (state_map[yyPeek()] == MY_LEX_CMP_OP || state_map[yyPeek()] == MY_LEX_LONG_CMP_OP)
      {
	      yySkip();
	      if (state_map[yyPeek()] == MY_LEX_CMP_OP)
	        yySkip();
      }
      if ((tokval = find_keyword(lex,(uint) (lex->ptr - lex->tok_start),0)))
      {
	      lex->next_state= MY_LEX_START;	// Found long op
	      return(tokval);
      }
      state = MY_LEX_CHAR;		// Something fishy found
      break;

    case MY_LEX_BOOL:
      if (c != yyPeek())
      {
      	state=MY_LEX_CHAR;
      	break;
      }
      yySkip();
      tokval = find_keyword(lex,2,0);	// Is a bool operator
      lex->next_state= MY_LEX_START;	// Allow signed numbers
      return(tokval);

    case MY_LEX_STRING_OR_DELIMITER:
      //if (((THD *) yythd)->variables.sql_mode & MODE_ANSI_QUOTES)
      if (lex->sql_mode.MODE_ANSI_QUOTES)
      {
	      state= MY_LEX_USER_VARIABLE_DELIMITER;
	      break;
      }
      /* " used for strings */
    case MY_LEX_STRING:			// Incomplete text string
      tmp_lex_string.str= get_text(lex);
      if(!tmp_lex_string.str)
      {
	      new_ast_terminal_node(lex, /*0, */0, tmp_lex_string.str);
        state= MY_LEX_CHAR;		// Read char by char
	      break;
      }
      new_ast_terminal_node(lex, tmp_lex_string.str, lex->yytoklen, tmp_lex_string.str);
#if 0
      if (!(yylval->lex_str.str = get_text(lex)))
      {
	      state= MY_LEX_CHAR;		// Read char by char
	      break;
      }
      yylval->lex_str.length=lex->yytoklen;
#endif
      return(TEXT_STRING);

    case MY_LEX_COMMENT:			//  Comment
      //lex->select_lex.options|= OPTION_FOUND_COMMENT;
      while ((c = yyGet()) && (c != '\n') && !((c == '\r') && (yyPeek() != '\n'))) ;
      yyUnget();			// Safety against eof
      state = MY_LEX_START;		// Try again
      break;
    case MY_LEX_LONG_COMMENT:		/* Long C comment? */
      if (yyPeek() != '*')
      {
	state=MY_LEX_CHAR;		// Probable division
	break;
      }
      yySkip();				// Skip '*'
      //lex->select_lex.options|= OPTION_FOUND_COMMENT;
      if (yyPeek() == '!')		// MySQL command in comment
      {
	ulong version=MYSQL_VERSION_ID;
	yySkip();
	state=MY_LEX_START;
	if (my_isdigit(cs,yyPeek()))
	{				// Version number
	  version=strtol((char*) lex->ptr,(char**) &lex->ptr,10);
	}
	if (version <= MYSQL_VERSION_ID)
	{
	  lex->in_comment=1;
	  break;
	}
      }
      while (lex->ptr != lex->end_of_query &&
	     ((c=yyGet()) != '*' || yyPeek() != '/')) ;
      if (lex->ptr != lex->end_of_query)
	yySkip();			// remove last '/'
      state = MY_LEX_START;		// Try again
      break;
    case MY_LEX_END_LONG_COMMENT:
      if (lex->in_comment && yyPeek() == '/')
      {
	yySkip();
	lex->in_comment=0;
	state=MY_LEX_START;
      }
      else
	state=MY_LEX_CHAR;		// Return '*'
      break;
    case MY_LEX_SET_VAR:		// Check if ':='
      if (yyPeek() != '=')
      {
	state=MY_LEX_CHAR;		// Return ':'
	break;
      }
      yySkip();
      new_ast_terminal_node(lex, /*":=", */2, 0);
      return (SET_VAR);
    case MY_LEX_SEMICOLON:			// optional line terminator
      if (yyPeek())
      {
        //if ((thd->client_capabilities & CLIENT_MULTI_STATEMENTS) && 
        //    !lex->stmt_prepare_mode)
        if(0) //serg: disabled so procedure body (with ';') could be parsed successfully
        {
	        lex->safe_to_cache_query= 0;
          lex->found_semicolon=(char*) lex->ptr;
          //thd->server_status|= SERVER_MORE_RESULTS_EXISTS;
          lex->next_state=     MY_LEX_END;
          return (END_OF_INPUT);
        }
        state= MY_LEX_CHAR;		// Return ';'
	break;
      }
      /* fall true */
    case MY_LEX_EOL:
      if (lex->ptr >= lex->end_of_query)
      {
	lex->next_state=MY_LEX_END;	// Mark for next loop
	return(END_OF_INPUT);
      }
      state=MY_LEX_CHAR;
      break;
    case MY_LEX_END:
      lex->next_state=MY_LEX_END;
      return(0);			// We found end of input last time
      
      /* Actually real shouldn't start with . but allow them anyhow */
    case MY_LEX_REAL_OR_POINT:
      if (my_isdigit(cs,yyPeek()))
	state = MY_LEX_REAL;		// Real
      else
      {
	state= MY_LEX_IDENT_SEP;	// return '.'
	yyUnget();			// Put back '.'
      }
      break;
    case MY_LEX_USER_END:		// end '@' of user@hostname
      switch (state_map[yyPeek()]) {
      case MY_LEX_STRING:
      case MY_LEX_USER_VARIABLE_DELIMITER:
      case MY_LEX_STRING_OR_DELIMITER:
	break;
      case MY_LEX_USER_END:
	lex->next_state=MY_LEX_SYSTEM_VAR;
	break;
      default:
	lex->next_state=MY_LEX_HOSTNAME;
	break;
      }
      new_ast_terminal_node(lex, /*"@", */1, 0);
#if 0
      yylval->lex_str.str=(char*) lex->ptr;
      yylval->lex_str.length=1;
#endif
      return((int) '@');
    case MY_LEX_HOSTNAME:		// end '@' of user@hostname
      for (c=yyGet() ; 
	   my_isalnum(cs,c) || c == '.' || c == '_' ||  c == '$';
	   c= yyGet()) ;

      tmp_lex_string= get_token(lex,yyLength());
      new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, tmp_lex_string.str);
#if 0
      yylval->lex_str=get_token(lex,yyLength());
#endif
      return(LEX_HOSTNAME);
    case MY_LEX_SYSTEM_VAR:
      new_ast_terminal_node(lex, /*(char*) lex->ptr, */1, 0);
#if 0
      yylval->lex_str.str=(char*) lex->ptr;
      yylval->lex_str.length=1;
#endif
      yySkip();					// Skip '@'
      lex->next_state= (state_map[yyPeek()] ==
			MY_LEX_USER_VARIABLE_DELIMITER ?
			MY_LEX_OPERATOR_OR_IDENT :
			MY_LEX_IDENT_OR_KEYWORD);
      return((int) '@');
    case MY_LEX_IDENT_OR_KEYWORD:
      /*
	We come here when we have found two '@' in a row.
	We should now be able to handle:
	[(global | local | session) .]variable_name
      */
      
      for (result_state= 0; ident_map[c= yyGet()]; result_state|= c) ;
      /* If there were non-ASCII characters, mark that we must convert */
      result_state= result_state & 0x80 ? IDENT_QUOTED : IDENT;
      
      if (c == '.')
	lex->next_state=MY_LEX_IDENT_SEP;
      length= (uint) (lex->ptr - lex->tok_start)-1;
      if (length == 0) 
        return(ABORT_SYM);              // Names must be nonempty.
      if ((tokval= find_keyword(lex,length,0)))
      {
	yyUnget();				// Put back 'c'
	return(tokval);				// Was keyword
      }
      tmp_lex_string= get_token(lex, length);
      new_ast_terminal_node(lex, /*tmp_lex_string.str, */tmp_lex_string.length, tmp_lex_string.str);
#if 0
      yylval->lex_str=get_token(lex,length);
#endif
      return(result_state);
    }
  }
  return 0;
}

/*
  Initialize LEX object.

  SYNOPSIS
    st_lex::st_lex()

  NOTE
    LEX object initialized with this constructor can be used as part of
    THD object for which one can safely call open_tables(), lock_tables()
    and close_thread_tables() functions. But it is not yet ready for
    statement parsing. On should use lex_start() function to prepare LEX
    for this.
*/

st_lex::st_lex()
  : yylval(0), result(0), sql_command(SQLCOM_END)//, query_tables_own_last(0)
{
}

/*
  check if command can use VIEW with MERGE algorithm (for top VIEWs)

  SYNOPSIS
    st_lex::can_use_merged()

  DESCRIPTION
    Only listed here commands can use merge algorithm in top level
    SELECT_LEX (for subqueries will be used merge algorithm if
    st_lex::can_not_use_merged() is not TRUE).

  RETURN
    FALSE - command can't use merged VIEWs
    TRUE  - VIEWs with MERGE algorithms can be used
*/

bool st_lex::can_use_merged()
{
  switch (sql_command)
  {
  case SQLCOM_SELECT:
  case SQLCOM_CREATE_TABLE:
  case SQLCOM_UPDATE:
  case SQLCOM_UPDATE_MULTI:
  case SQLCOM_DELETE:
  case SQLCOM_DELETE_MULTI:
  case SQLCOM_INSERT:
  case SQLCOM_INSERT_SELECT:
  case SQLCOM_REPLACE:
  case SQLCOM_REPLACE_SELECT:
  case SQLCOM_LOAD:
    return TRUE;
  default:
    return FALSE;
  }
}

/*
  Check if command can't use merged views in any part of command

  SYNOPSIS
    st_lex::can_not_use_merged()

  DESCRIPTION
    Temporary table algorithm will be used on all SELECT levels for queries
    listed here (see also st_lex::can_use_merged()).

  RETURN
    FALSE - command can't use merged VIEWs
    TRUE  - VIEWs with MERGE algorithms can be used
*/

bool st_lex::can_not_use_merged()
{
  switch (sql_command)
  {
  case SQLCOM_CREATE_VIEW:
  case SQLCOM_SHOW_CREATE:
  /*
    SQLCOM_SHOW_FIELDS is necessary to make 
    information schema tables working correctly with views.
    see get_schema_tables_result function
  */
  case SQLCOM_SHOW_FIELDS:
    return TRUE;
  default:
    return FALSE;
  }
}

/*
  Detect that we need only table structure of derived table/view

  SYNOPSIS
    only_view_structure()

  RETURN
    TRUE yes, we need only structure
    FALSE no, we need data
*/

bool st_lex::only_view_structure()
{
  switch (sql_command) {
  case SQLCOM_SHOW_CREATE:
  case SQLCOM_SHOW_TABLES:
  case SQLCOM_SHOW_FIELDS:
  case SQLCOM_REVOKE_ALL:
  case SQLCOM_REVOKE:
  case SQLCOM_GRANT:
  case SQLCOM_CREATE_VIEW:
    return TRUE;
  default:
    return FALSE;
  }
}


/*
  Should Items_ident be printed correctly

  SYNOPSIS
    need_correct_ident()

  RETURN
    TRUE yes, we need only structure
    FALSE no, we need data
*/


bool st_lex::need_correct_ident()
{
  switch(sql_command)
  {
  case SQLCOM_SHOW_CREATE:
  case SQLCOM_SHOW_TABLES:
  case SQLCOM_CREATE_VIEW:
    return TRUE;
  default:
    return FALSE;
  }
}

} // namespace mysql_parser

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif
