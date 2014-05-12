#ifndef statement_parser_h
#define statement_parser_h

#include "mysql_sql_parser_public_interface.h"
#include "myx_sql_parser_public_interface.h"
#include <string>

namespace mysql_parser
{

typedef struct charset_info_st CHARSET_INFO;

class MYSQL_SQL_PARSER_PUBLIC_FUNC MyxStatementParser
{
  static const int CHAR_BUFFER_SIZE= 32768; // required to be >= 4

  enum ParserState { start, stmt, str, comment1, comment2, mlcomment, delimtok, delimkwd, eos };

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

  std::string delim;
  CHARSET_INFO *cs;
  
  char *char_buffer;
  char *char_buffer_b;
  char *char_buffer_e;
  bool eof_hit;
  int _stmt_boffset;
  int _stmt_first_line_first_symbol_pos;
  int _symbols_since_newline;
  int _total_lc;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

  int fill_buffer(std::istream& is);
  int buffer_eof(std::istream& is);

  int get_next_char(std::istream& is, int *len, int count_lines= 1);
  int peek_next_char(std::istream& is, int *len);
  void add_char_to_buffer(std::string& buffer, int c, int len) const;

public:
  MyxStatementParser(CHARSET_INFO *charset);
  virtual ~MyxStatementParser();

  void process(std::istream& is, process_sql_statement_callback, void *arg, int mode);
  const std::string & delimiter() const { return delim; }
  int statement_boffset() const { return _stmt_boffset; }
  int statement_first_line_first_symbol_pos() const { return _stmt_first_line_first_symbol_pos; }
  int total_line_count() const { return _total_lc; }
};

} // namespace mysql_parser

#endif // _STATEMENT_PARSER_H_
