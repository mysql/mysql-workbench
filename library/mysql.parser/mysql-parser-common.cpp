/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <sstream>
#include <string>
#include <set>
#include <stack>
#include <vector>

#include <antlr3.h>

#include "MySQLLexer.h"  // The generated lexer.
#include "MySQLParser.h"  // The generated lexer.

#include "base/log.h"
#include "base/string_utilities.h"

#include "mysql-parser-common.h"
#include "mysql-parser.h"
#include "mysql-scanner.h"

DEFAULT_LOG_DOMAIN("MySQL parsing")

extern "C" {
  
  /**
   * Checks the given identifier whether it is a defined charset, which directs
   * the lexer to classify it appropriately.
   */
  ANTLR3_UINT32 check_charset(void *payload, pANTLR3_STRING text)
  {
    // Get the actual token text and skip the initial underscore char.
    // There's an additional char at the end of the input for some reason (maybe a lexer bug),
    // so we also ignore the last char.
    MySQLParsingBase *base = (MySQLParsingBase*)payload;

    std::string token_text((const char*)text->chars + 1, text->len - 2);
    if (base->is_charset(base::tolower(token_text)))
      return UNDERSCORE_CHARSET;

    return IDENTIFIER;
  }
  
  /**
   * Checks the given text if it is equal to "\N" (w/o quotes and in uppercase). We need this extra
   * check as our lexer is case insensitive.
   */
  ANTLR3_UINT32 check_null(pANTLR3_STRING text)
  {
    std::string token_text((const char*)text->chars, text->len - 1);
    if (token_text == "\\N")
      return NULL2_SYMBOL;
    return ANTLR3_TOKEN_INVALID;
  }

} // extern "C"

//----------------- MySQLParsingBase ---------------------------------------------------------------

class MySQLParsingBase::Private
{
public:
  std::set<std::string> _charsets; // A list of supported charsets.
  unsigned _sql_mode;
  std::vector<MySQLParserErrorInfo> _error_info;
};

MySQLParsingBase::MySQLParsingBase(const std::set<std::string> &charsets)
{
  d = new Private();
  d->_charsets = charsets;
  d->_sql_mode = 0;
}

//--------------------------------------------------------------------------------------------------

void MySQLParsingBase::add_error(const std::string &text, ANTLR3_UINT32 error, ANTLR3_UINT32 token,
                                 ANTLR3_UINT32 line, ANTLR3_UINT32 offset, ANTLR3_UINT32 length)
{
  MySQLParserErrorInfo info = {text, error, token, length, line, offset};
  d->_error_info.push_back(info);
};

//--------------------------------------------------------------------------------------------------

const std::vector<MySQLParserErrorInfo>& MySQLParsingBase::error_info()
{
  return d->_error_info;
}

//--------------------------------------------------------------------------------------------------

bool MySQLParsingBase::has_errors()
{
  return d->_error_info.size() > 0;
}

//--------------------------------------------------------------------------------------------------

unsigned MySQLParsingBase::sql_mode()
{
  return d->_sql_mode;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if the given string is one of the supported charsets.
 */
bool MySQLParsingBase::is_charset(const std::string &s)
{
  return d->_charsets.find(s) != d->_charsets.end();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is an identifier.
 */
bool MySQLParsingBase::is_identifier(ANTLR3_UINT32 type)
{
  bool result = (type == IDENTIFIER) || (type == BACK_TICK_QUOTED_ID);
  if (!result)
  {
    // Symbols are sorted so that keywords allowed as identifiers are in a continuous range
    // making this check easy (and reduce the parser size by 300% compared to generated token ids).
    result = (type >= ACTION_SYMBOL && type <= PARTITION_SYMBOL);

    if (!result)
    {
      // Double quoted text represents identifiers only if the ANSI QUOTES sql mode is active.
      result = ((d->_sql_mode & SQL_MODE_ANSI_QUOTES) != 0) && (type == DOUBLE_QUOTED_TEXT);
    }
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is a keyword.
 */
bool MySQLParsingBase::is_keyword(ANTLR3_UINT32 type)
{
  if (type >= ACTION_SYMBOL && type <= PARTITION_SYMBOL)
    return true;

  switch (type)
  {
  case AT_SIGN_SYMBOL:
  case AT_AT_SIGN_SYMBOL:
  case BACK_TICK:
  case BACK_TICK_QUOTED_ID:
  case BITNUMBER:
  case BITSTRING:
  case BITWISE_AND_OPERATOR:
  case BITWISE_NOT_OPERATOR:
  case BITWISE_OR_OPERATOR:
  case BITWISE_XOR_OPERATOR:
  case CLOSE_PAR_SYMBOL:
  case COLON_SYMBOL:
  case COMMA_SYMBOL:
  case DASHDASH_COMMENT:
  case DIGIT:
  case DIGITS:
  case DIV_OPERATOR:
  case DOT_SYMBOL:
  case DOUBLE_QUOTE:
  case DOUBLE_QUOTED_TEXT:
  case EQUAL_OPERATOR:
  case ESCAPE_OPERATOR:
  case EXPRESSION_TOKEN:
  case FIELD_REF_ID_TOKEN:
  case FLOAT:
  case FUNCTION_CALL_TOKEN:
  case GREATER_OR_EQUAL_OPERATOR:
  case GREATER_THAN_OPERATOR:
  case HEXDIGIT:
  case HEXNUMBER:
  case HEXSTRING:
  case IDENTIFIER:
  case INDEX_HINT_LIST_TOKEN:
  case INTEGER:
  case JOIN_EXPR_TOKEN:
  case LESS_OR_EQUAL_OPERATOR:
  case LESS_THAN_OPERATOR:
  case LETTER_WHEN_UNQUOTED:
  case LOGICAL_AND_OPERATOR:
  case LOGICAL_NOT_OPERATOR:
  case LOGICAL_OR_OPERATOR:
  case MINUS_OPERATOR:
  case ML_COMMENT_END:
  case ML_COMMENT_HEAD:
  case MOD_OPERATOR:
  case MULT_OPERATOR:
  case NCHAR_TEXT:
  case NOT_EQUAL2_OPERATOR:
  case NOT_EQUAL_OPERATOR:
  case NULL2_SYMBOL:
  case NULL_SAFE_EQUAL_OPERATOR:
  case OPEN_PAR_SYMBOL:
  case PARAM_MARKER:
  case PAR_EXPRESSION_TOKEN:
  case PLUS_OPERATOR:
  case POUND_COMMENT:
  case SEMICOLON_SYMBOL:
  case SHIFT_LEFT_OPERATOR:
  case SHIFT_RIGHT_OPERATOR:
  case SINGLE_QUOTE:
  case SINGLE_QUOTED_TEXT:
  case STRING_TOKEN:
  case SUBQUERY_TOKEN:
  case TABLE_REF_ID_TOKEN:
  case UNDERLINE_SYMBOL:
  case UNDERSCORE_CHARSET:
  case VERSION_COMMENT:
  case VERSION_COMMENT_END:
  case VERSION_COMMENT_INTRODUCER:
  case VERSION_COMMENT_START_TOKEN:
  case VERSION_COMMENT_TAIL:
  case WS:
  case XA_ID_TOKEN:
    return false;

  default:
    return true;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is a relation token.
 */
bool MySQLParsingBase::is_relation(ANTLR3_UINT32 type)
{
  switch (type)
  {
    case EQUAL_OPERATOR:
    case ASSIGN_OPERATOR:
    case NULL_SAFE_EQUAL_OPERATOR:
    case GREATER_OR_EQUAL_OPERATOR:
    case GREATER_THAN_OPERATOR:
    case LESS_OR_EQUAL_OPERATOR:
    case LESS_THAN_OPERATOR:
    case NOT_EQUAL_OPERATOR:
    case NOT_EQUAL2_OPERATOR:
    case PLUS_OPERATOR:
    case MINUS_OPERATOR:
    case MULT_OPERATOR:
    case DIV_OPERATOR:
    case MOD_OPERATOR:
    case LOGICAL_NOT_OPERATOR:
    case BITWISE_NOT_OPERATOR:
    case SHIFT_LEFT_OPERATOR:
    case SHIFT_RIGHT_OPERATOR:
    case LOGICAL_AND_OPERATOR:
    case BITWISE_AND_OPERATOR:
    case BITWISE_XOR_OPERATOR:
    case LOGICAL_OR_OPERATOR:
    case BITWISE_OR_OPERATOR:

    case OR_SYMBOL:
    case XOR_SYMBOL:
    case AND_SYMBOL:
    case IS_SYMBOL:
    case BETWEEN_SYMBOL:
    case LIKE_SYMBOL:
    case REGEXP_SYMBOL:
    case IN_SYMBOL:
    case SOUNDS_SYMBOL:
    case NOT_SYMBOL:
      return true;

  default:
    return false;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is a number type.
 */
bool MySQLParsingBase::is_number(ANTLR3_UINT32 type)
{
  switch (type)
  {
    case INTEGER:
    case FLOAT:
    case HEXNUMBER:
    case HEXSTRING:
    case BITNUMBER:
    case BITSTRING:
      return true;

  default:
    return false;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is an operator or punctuation character.
 */
bool MySQLParsingBase::is_operator(ANTLR3_UINT32 type)
{
  switch (type)
  {
    case EQUAL_OPERATOR:
    case ASSIGN_OPERATOR:
    case NULL_SAFE_EQUAL_OPERATOR:
    case GREATER_OR_EQUAL_OPERATOR:
    case GREATER_THAN_OPERATOR:
    case LESS_OR_EQUAL_OPERATOR:
    case LESS_THAN_OPERATOR:
    case NOT_EQUAL_OPERATOR:
    case NOT_EQUAL2_OPERATOR:
    case PLUS_OPERATOR:
    case MINUS_OPERATOR:
    case MULT_OPERATOR:
    case DIV_OPERATOR:
    case MOD_OPERATOR:
    case LOGICAL_NOT_OPERATOR:
    case BITWISE_NOT_OPERATOR:
    case SHIFT_LEFT_OPERATOR:
    case SHIFT_RIGHT_OPERATOR:
    case LOGICAL_AND_OPERATOR:
    case BITWISE_AND_OPERATOR:
    case BITWISE_XOR_OPERATOR:
    case LOGICAL_OR_OPERATOR:
    case BITWISE_OR_OPERATOR:

    case DOT_SYMBOL:
    case COMMA_SYMBOL:
    case SEMICOLON_SYMBOL:
    case COLON_SYMBOL:
    case OPEN_PAR_SYMBOL:
    case CLOSE_PAR_SYMBOL:
    case AT_SIGN_SYMBOL:
    case AT_AT_SIGN_SYMBOL:
    case PARAM_MARKER:
      return true;

  default:
    return false;
  }
}

//--------------------------------------------------------------------------------------------------

bool MySQLParsingBase::is_subtree(struct ANTLR3_BASE_TREE_struct *tree)
{
  return tree->getChildCount(tree) > 0;
}

//--------------------------------------------------------------------------------------------------

unsigned MySQLParsingBase::parse_sql_mode(const std::string &sql_mode)
{
  unsigned result = 0;

  std::string sql_mode_string = base::toupper(sql_mode);
  std::istringstream iss(sql_mode_string);
  std::string mode;
  while (std::getline(iss, mode, ','))
  {
    mode = base::trim(mode);
    if (mode == "ANSI" || mode == "DB2" || mode == "MAXDB" || mode == "MSSQL" || mode == "ORACLE" ||
      mode == "POSTGRESQL")
      result |= SQL_MODE_ANSI_QUOTES | SQL_MODE_PIPES_AS_CONCAT | SQL_MODE_IGNORE_SPACE;
    else if (mode == "ANSI_QUOTES")
      result |= SQL_MODE_ANSI_QUOTES;
    else if (mode == "PIPES_AS_CONCAT")
      result |= SQL_MODE_PIPES_AS_CONCAT;
    else if (mode == "NO_BACKSLASH_ESCAPES")
      result |= SQL_MODE_NO_BACKSLASH_ESCAPES;
    else if (mode == "IGNORE_SPACE")
      result |= SQL_MODE_IGNORE_SPACE;
    else if (mode == "HIGH_NOT_PRECEDENCE" || mode == "MYSQL323" || mode == "MYSQL40")
      result |= SQL_MODE_HIGH_NOT_PRECEDENCE;
  }

  d->_sql_mode = result;

  return result;
}

//--------------------------------------------------------------------------------------------------
