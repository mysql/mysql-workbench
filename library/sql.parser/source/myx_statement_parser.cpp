
#include <iostream>
#include <sstream>
#include <fstream>

#include <string.h>

#include "myx_statement_parser.h"

#include "mysql_version.h"
#include "my_global.h"
#include "my_sys.h"
#include "sql_string.h"
#include "unireg.h"

#include "structs.h"
#include <m_ctype.h>
#include <hash.h>

#include "sql_lex.h"

namespace mysql_parser
{

MyxStatementParser::MyxStatementParser(CHARSET_INFO *charset)
  : cs(charset), eof_hit(false)
{
  delim= ";";
  char_buffer= new char[CHAR_BUFFER_SIZE];
  char_buffer_e= char_buffer_b= char_buffer + CHAR_BUFFER_SIZE;
}

MyxStatementParser::~MyxStatementParser()
{
  delete[] char_buffer;
}

static bool is_empty_statement(const std::string& str)
{
  int i= 0;
  while(str[i] != 0)
    if(str[i++] > ' ')
      return false;

  return true;
}

int MyxStatementParser::fill_buffer(std::istream& is)
{
  char *input_start= std::copy(char_buffer_b, char_buffer_e, char_buffer);
  int len= (int)(input_start - char_buffer);
  is.read(input_start, CHAR_BUFFER_SIZE - len);
  int gc= (int) is.gcount();
  char_buffer_b= char_buffer;
  char_buffer_e= char_buffer + len + gc;
  return gc;
}

int MyxStatementParser::buffer_eof(std::istream& is)
{
  return eof_hit;
}

int MyxStatementParser::get_next_char(std::istream& is, int *len, int count_lines)
{
  if(char_buffer_e - char_buffer_b < 4)
    fill_buffer(is);

  if(char_buffer_e == char_buffer_b)
  {
    eof_hit= true;
    *len= 0;
    return -1;
  }

  *len= 1;
  unsigned int c;

  if (my_mbcharlen(cs, *char_buffer_b) > 1)
  {
    static unsigned int mask[3]= { 0x0000FFFF, 0x00FFFFFF, 0xFFFFFFFF };
    
    *len= my_ismbchar(cs, char_buffer_b, char_buffer_e);
    c= *(unsigned int *)char_buffer_b;
    char_buffer_b += *len;
    c&= mask[*len - 2];
  }
  else
  {
    c= *char_buffer_b++;
  }

  if (count_lines)
  {
    switch (c)
    {
    case '\r':
      {
        int len;
        if ('\n' == peek_next_char(is, &len))
          break;
      }
    case '\n':
      ++_total_lc;
      _symbols_since_newline= 0;
      break;
    default:
      _symbols_since_newline += *len;
      break;
    }
  }

  return c;
}

int MyxStatementParser::peek_next_char(std::istream& is, int *len)
{
  int c= get_next_char(is, len, 0);
  char_buffer_b -= *len;
  return c;
}

void MyxStatementParser::add_char_to_buffer(std::string& buffer, int c, int len) const
{
  unsigned uc= (unsigned)c;

  switch(len) // all cases fall through
  {
  case 4:
    buffer += (char)((uc & 0xFF000000) >> 24);
  case 3:
    buffer += (char)((uc & 0x00FF0000) >> 16);
  case 2:
    buffer += (char)((uc & 0x0000FF00) >> 8);
  case 1:
    buffer += (char)((uc & 0x000000FF) >> 0);
  }
}

void MyxStatementParser::process(std::istream& is, process_sql_statement_callback cb, void *arg, int mode)
{
  static const char *kwd= "DELIMITER";
  
  int c;
  ParserState state = start, prevState = start;
  std::string stmt_buffer;
  std::string delim_buffer;
  char strchar = 0;
  _stmt_boffset= 0;
  _stmt_first_line_first_symbol_pos= 0;
  _symbols_since_newline= 0;
  _total_lc= 0;

  int len;
  bool m, can_be_kwd;
  int p;

  while(!buffer_eof(is) && !parser_is_stopped) {
    switch(state) {
    case eos:
      break;
    
    case start:
      stmt_buffer.clear();
      c= get_next_char(is, &len);
      while(my_isspace(cs, c) || (c == '\n') || (c == '\r')) {
        add_char_to_buffer(stmt_buffer, c, len);
        c= get_next_char(is, &len);
      }
      add_char_to_buffer(stmt_buffer, c, len);
      if(kwd[0] == my_toupper(cs, c)) {
        state= delimkwd;
      } else if(c == '`') {
        strchar= '`';
        state= str;
      } else if(c == '\'') {
        strchar= '\'';
        state= str;
      } else if(c == '"') {
        strchar= '"';
        state= str;
      } else if((c == '/') && (peek_next_char(is, &len) == '*')) {
        prevState= start;
        state= mlcomment;
      } else if((c == '-') && (peek_next_char(is, &len) == '-')) {
        prevState= start;
        state= comment1;
      } else if(c == '#') {
        prevState= start;
        state= comment2;
      } else if(c == delim[0]) {
        state= delimtok;
      } else {
        state= stmt;
      }
      continue;
    
    case delimkwd:
      m= true;
      for (int i= 1; kwd[i] != '\0'; i++) {
        c= peek_next_char(is, &len);
        if(my_toupper(cs, c) != kwd[i]) {
          m= false;
          break;
        }
        else
        {
          c= get_next_char(is, &len);
          add_char_to_buffer(stmt_buffer, c, len);        
        }
      }
      if(!m) {
        //state= stmt;
        //continue;
        goto stmtlabel;
      }
      c= get_next_char(is, &len);
      add_char_to_buffer(stmt_buffer, c, len);
      if(!my_isspace(cs, c)) {
        state= stmt;
        continue;
      }
      c= peek_next_char(is, &len);
      while(my_isspace(cs, c)) {
        c= get_next_char(is, &len);
        add_char_to_buffer(stmt_buffer, c, len);
        c= peek_next_char(is, &len);
      }
      if((c == '\r') || (c == '\n')) {
        add_char_to_buffer(stmt_buffer, c, len);
        state= stmt;
        continue;
      }
      delim_buffer.clear();
      _stmt_boffset+= (int)stmt_buffer.size();
      while((c != '\r') && (c != '\n') /*&& !my_isspace(cs, c)*/ && !buffer_eof(is)) 
      {
        c= get_next_char(is, &len);
        _stmt_boffset+= len;
        delim_buffer+= (char)c;
        c= peek_next_char(is, &len);
      }
      //if(delim_buffer.length() > delim.length()) 
      //{
      //  if(delim_buffer.compare(delim_buffer.length() - delim.length(), delim.length(), delim) == 0)
      //  {
      //    delim_buffer.erase(delim_buffer.length() - delim.length());
      //  }
      //}

      // new delimiter
      if(!delim_buffer.empty())
      {
        stmt_buffer.clear();
        for (size_t n= 0, count= delim_buffer.size(); n < count; ++n)
        {
          if (my_isspace(cs, delim_buffer[n]))
          {
            delim_buffer.resize(n);
            break;
          }
        }
        delim= delim_buffer;
        for(;;)
        {
          c= peek_next_char(is, &len);
          if((c != '\r') && (c != '\n'))
            break;
          c= get_next_char(is, &len);
          _stmt_boffset+= len;
        }
        _stmt_first_line_first_symbol_pos= _symbols_since_newline;
      }

      state= start;
      continue;

    case str:
      c= get_next_char(is, &len);
      while((c != strchar) && !buffer_eof(is)) 
      {
        add_char_to_buffer(stmt_buffer, c, len);
        if(c == '\\') 
        {
          c= get_next_char(is, &len);
          add_char_to_buffer(stmt_buffer, c, len);
        }
        c= get_next_char(is, &len);
      }
      add_char_to_buffer(stmt_buffer, c, len);
      if(!buffer_eof(is)) 
      {
        state= stmt;
      }
      continue;

    case mlcomment:
      c= get_next_char(is, &len);
      add_char_to_buffer(stmt_buffer, c, len);
      if(c != '*') {
        state= stmt;
        continue;
      }

      p= ' ';
      while(!buffer_eof(is)) {
        c= get_next_char(is, &len);
        add_char_to_buffer(stmt_buffer, c, len);
        if((c == '/') && (p == '*')) {
          state= stmt;
          break;
        }
        if(buffer_eof(is))
        {
          break;
        }
        p= c;
      }
      continue;

    case comment2:
      c= get_next_char(is, &len);
      while((c != '\r') && (c != '\n') && !buffer_eof(is)) {
        add_char_to_buffer(stmt_buffer, c, len);
        c= get_next_char(is, &len);
      }
      add_char_to_buffer(stmt_buffer, c, len);
      state= stmt;
      
      c= peek_next_char(is, &len);
      while((c == '\r') || (c == '\n'))
      {
        c= get_next_char(is, &len);
        add_char_to_buffer(stmt_buffer, c, len);
        c= peek_next_char(is, &len);
      }

      continue;

    case comment1:
      c= get_next_char(is, &len);
      if(c != '-') {
        add_char_to_buffer(stmt_buffer, c, len);
        state= prevState;
        continue;
      }
      while((c != '\r') && (c != '\n') && !buffer_eof(is)) {
        add_char_to_buffer(stmt_buffer, c, len);
        c= get_next_char(is, &len);
      }
      add_char_to_buffer(stmt_buffer, c, len);
      state= stmt;

      c= peek_next_char(is, &len);
      while((c == '\r') || (c == '\n'))
      {
        c= get_next_char(is, &len);
        add_char_to_buffer(stmt_buffer, c, len);
        c= peek_next_char(is, &len);
      }
      continue;

    case delimtok:
      m= true;
      for(size_t i= 1; i < delim.size(); i++) {
        c= get_next_char(is, &len);
        add_char_to_buffer(stmt_buffer, c, len);
        if(my_toupper(cs, c) != delim[i]) {
          m= false;
          break;
        }
      }
      if(!m) {
        state= stmt;
        continue;
      }
      // new statement is read
      stmt_buffer.erase(stmt_buffer.length() - delim.length());
      {
        std::string::size_type stmt_boffset_inc = stmt_buffer.size() + delim.size();
        if(!is_empty_statement(stmt_buffer))
          cb(this, stmt_buffer.c_str(), arg);
        _stmt_boffset+= (int)stmt_boffset_inc;
      }
      stmt_buffer.clear();
      _stmt_first_line_first_symbol_pos= _symbols_since_newline;

      state= start;
      continue;

    case stmt:
stmtlabel:
      can_be_kwd= true;

      while(!buffer_eof(is)) {
        c= get_next_char(is, &len);
        add_char_to_buffer(stmt_buffer, c, len);
        if(can_be_kwd && (kwd[0] == my_toupper(cs, c))) {
          prevState= stmt;
          state= delimkwd;
          break;
        } else if(c == '`') {
          prevState= stmt;
          strchar= '`';
          state= str;
          break;
        } else if(c == '\'') {
          prevState= stmt;
          strchar= '\'';
          state= str;
          break;
        } else if(c == '"') {
          prevState= stmt;
          strchar= '"';
          state= str;
          break;
        } else if((c == '/') && (peek_next_char(is, &len) == '*')) {
          prevState= stmt;
          state= mlcomment;
          break;
        } else if((c == '-') && (peek_next_char(is, &len) == '-')) {
          prevState= stmt;
          state= comment1;
          break;
        } else if(c == '#') {
          prevState= stmt;
          state= comment2;
          break;
        } else if(c == delim[0]) {
          prevState= stmt;
          state= delimtok;
          break;
        }
        if(c > ' ')
          can_be_kwd= false;
      }
      continue;
    } // switch
  }

  if (parser_is_stopped)
    return;
  else if(!(mode & MYX_SPM_DELIMS_REQUIRED)
    && (stmt_buffer.length() > 0)
    && !is_empty_statement(stmt_buffer))
  {
    int stmt_boffset_inc= (int)stmt_buffer.size();
    cb(this, stmt_buffer.c_str(), arg);
    _stmt_boffset+= stmt_boffset_inc;
  }
}

MYX_PUBLIC_FUNC 
int myx_process_sql_statements(const char *sql, CHARSET_INFO *cs, process_sql_statement_callback cb, void *user_data, int mode)
{
  MyxStatementParser p(cs);
  std::istringstream tmp(sql, std::ios::in | std::ios::binary);
  p.process(tmp, cb, user_data, mode);
  return 0;
}

MYX_PUBLIC_FUNC 
int myx_process_sql_statements_from_file(const char *filename, CHARSET_INFO *cs, process_sql_statement_callback cb, void *user_data, int mode)
{
  std::ifstream is;

#if defined(_MSC_VER)
  wchar_t filename_[MAX_PATH+1]= {0};
  MultiByteToWideChar(CP_UTF8, 0, filename, -1, filename_, MAX_PATH);
  is.open(filename_, std::ios::in | std::ios::binary);
#else
  is.open(filename, std::ios::in | std::ios::binary);
#endif

  // Find out size of the file and return if it doesn't contain anything.
  is.seekg (0, std::ios::end);
  std::streamoff length = is.tellg();
  is.seekg (0, std::ios::beg);
  if (length < 3) // 3 bytes to allow checking for a UTF-8 BOM.
    return 0;

  // Check if the file contains a BOM (byte-order-mark). 3 characters for the UTF-8 encoded BOM
  // +1 for the terminating 0 added by get().
  char buffer[4];
  is.read(buffer, 4);
  if (((unsigned char)buffer[0] != 0xEF) || ((unsigned char)buffer[1] != 0xBB) || ((unsigned char)buffer[2] != 0xBF))
  {
    // Not a BOM, so reset the input pointer.
    is.seekg (0, std::ios::beg);
  }

  MyxStatementParser p(cs);
  p.process(is, cb, user_data, mode);
  return 0;
}

} // namespace mysql_parser
