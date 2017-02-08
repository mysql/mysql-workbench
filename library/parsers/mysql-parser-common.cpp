/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include <map>

#include <antlr3.h>
#include <glib.h>

#include "MySQLLexer.h"  // The generated lexer.
#include "MySQLParser.h" // The generated lexer.

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
ANTLR3_UINT32 check_charset(void *payload, pANTLR3_STRING text) {
  // Get the actual token text and skip the initial underscore char.
  // There's an additional char at the end of the input for some reason (maybe a lexer bug),
  // so we also ignore the last char.
  MySQLRecognitionBase *base = (MySQLRecognitionBase *)payload;

  std::string token_text((const char *)text->chars + 1, text->len - 2);
  if (base->is_charset(base::tolower(token_text)))
    return UNDERSCORE_CHARSET;

  return IDENTIFIER;
}

} // extern "C"

//--------------------------------------------------------------------------------------------------

static const unsigned char *skipLeadingWhitespace(const unsigned char *head, const unsigned char *tail) {
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//--------------------------------------------------------------------------------------------------

static bool isLineBreak(const unsigned char *head, const unsigned char *lineLreak) {
  if (*lineLreak == '\0')
    return false;

  while (*head != '\0' && *lineLreak != '\0' && *head == *lineLreak) {
    head++;
    lineLreak++;
  }
  return *lineLreak == '\0';
}

//--------------------------------------------------------------------------------------------------

/**
 * A statement splitter to take a list of sql statements and split them into individual statements,
 * return their position and length in the original string (instead the copied strings).
 */
void determineStatementRanges(const char *sql, size_t length, const std::string &initialDelimiter,
                              std::vector<std::pair<size_t, size_t>> &ranges, const std::string &lineBreak) {
  std::string delimiter = initialDelimiter.empty() ? ";" : initialDelimiter;
  const unsigned char *delimiterHead = (unsigned char *)delimiter.c_str();

  const unsigned char keyword[] = "delimiter";

  const unsigned char *head = (unsigned char *)sql;
  const unsigned char *tail = head;
  const unsigned char *end = head + length;
  const unsigned char *newLine = (unsigned char *)lineBreak.c_str();
  bool haveContent = false; // Set when anything else but comments were found for the current statement.

  while (tail < end) {
    switch (*tail) {
      case '/': // Possible multi line comment or hidden (conditional) command.
        if (*(tail + 1) == '*') {
          tail += 2;
          bool isHiddenCommand = (*tail == '!');
          while (true) {
            while (tail < end && *tail != '*')
              tail++;
            if (tail == end) // Unfinished comment.
              break;
            else {
              if (*++tail == '/') {
                tail++; // Skip the slash too.
                break;
              }
            }
          }

          if (!isHiddenCommand && !haveContent)
            head = tail; // Skip over the comment.
        } else
          tail++;

        break;

      case '-': // Possible single line comment.
      {
        const unsigned char *endChar = tail + 2;
        if (*(tail + 1) == '-' && (*endChar == ' ' || *endChar == '\t' || isLineBreak(endChar, newLine))) {
          // Skip everything until the end of the line.
          tail += 2;
          while (tail < end && !isLineBreak(tail, newLine))
            tail++;
          if (!haveContent)
            head = tail;
        } else
          tail++;

        break;
      }

      case '#': // MySQL single line comment.
        while (tail < end && !isLineBreak(tail, newLine))
          tail++;
        if (!haveContent)
          head = tail;
        break;

      case '"':
      case '\'':
      case '`': // Quoted string/id. Skip this in a local loop.
      {
        haveContent = true;
        char quote = *tail++;
        while (tail < end && *tail != quote) {
          // Skip any escaped character too.
          if (*tail == '\\')
            tail++;
          tail++;
        }
        if (*tail == quote)
          tail++; // Skip trailing quote char to if one was there.

        break;
      }

      case 'd':
      case 'D': {
        haveContent = true;

        // Possible start of the keyword DELIMITER. Must be at the start of the text or a character,
        // which is not part of a regular MySQL identifier (0-9, A-Z, a-z, _, $, \u0080-\uffff).
        unsigned char previous = tail > (unsigned char *)sql ? *(tail - 1) : 0;
        bool isIdentifierChar = previous >= 0x80 || (previous >= '0' && previous <= '9') ||
                                ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z') || previous == '$' ||
                                previous == '_';
        if (tail == (unsigned char *)sql || !isIdentifierChar) {
          const unsigned char *run = tail + 1;
          const unsigned char *kw = keyword + 1;
          int count = 9;
          while (count-- > 1 && (*run++ | 0x20) == *kw++)
            ;
          if (count == 0 && *run == ' ') {
            // Delimiter keyword found. Get the new delimiter (everything until the end of the line).
            tail = run++;
            while (run < end && !isLineBreak(run, newLine))
              run++;
            delimiter = base::trim(std::string((char *)tail, run - tail));
            delimiterHead = (unsigned char *)delimiter.c_str();

            // Skip over the delimiter statement and any following line breaks.
            while (isLineBreak(run, newLine))
              run++;
            tail = run;
            head = tail;
          } else
            tail++;
        } else
          tail++;

        break;
      }

      default:
        if (*tail > ' ')
          haveContent = true;
        tail++;
        break;
    }

    if (*tail == *delimiterHead) {
      // Found possible start of the delimiter. Check if it really is.
      size_t count = delimiter.size();
      if (count == 1) {
        // Most common case. Trim the statement and check if it is not empty before adding the range.
        head = skipLeadingWhitespace(head, tail);
        if (head < tail)
          ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
        head = ++tail;
        haveContent = false;
      } else {
        const unsigned char *run = tail + 1;
        const unsigned char *del = delimiterHead + 1;
        while (count-- > 1 && (*run++ == *del++))
          ;

        if (count == 0) {
          // Multi char delimiter is complete. Tail still points to the start of the delimiter.
          // Run points to the first character after the delimiter.
          head = skipLeadingWhitespace(head, tail);
          if (head < tail)
            ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
          tail = run;
          head = run;
          haveContent = false;
        }
      }
    }
  }

  // Add remaining text to the range list.
  head = skipLeadingWhitespace(head, tail);
  if (head < tail)
    ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
}

//----------------- MySQLRecognitionBase ---------------------------------------------------------------

class MySQLRecognitionBase::Private {
public:
  const std::set<std::string> _charsets; // A list of supported charsets.
  unsigned _sql_mode;
  std::vector<ParserErrorInfo> _error_info;

  Private(const std::set<std::string> &charsets) : _charsets(charsets) {
    _sql_mode = 0;
  }
};

MySQLRecognitionBase::MySQLRecognitionBase(const std::set<std::string> &charsets) {
  d = new Private(charsets);
}

//--------------------------------------------------------------------------------------------------

MySQLRecognitionBase::~MySQLRecognitionBase() {
  delete d;
}

//--------------------------------------------------------------------------------------------------

void MySQLRecognitionBase::add_error(const std::string &message, ANTLR3_UINT32 token, ANTLR3_MARKER token_start,
                                     ANTLR3_UINT32 line, ANTLR3_UINT32 offset_in_line, ANTLR3_MARKER length) {
  ParserErrorInfo info = {message, token,          (size_t)(token_start - (ANTLR3_MARKER)lineStart()),
                          line,    offset_in_line, (size_t)length};
  d->_error_info.push_back(info);
};

//--------------------------------------------------------------------------------------------------

const std::vector<ParserErrorInfo> &MySQLRecognitionBase::error_info() const {
  return d->_error_info;
}

//--------------------------------------------------------------------------------------------------

bool MySQLRecognitionBase::has_errors() const {
  return d->_error_info.size() > 0;
}

//--------------------------------------------------------------------------------------------------

unsigned MySQLRecognitionBase::sql_mode() const {
  return d->_sql_mode;
}

//--------------------------------------------------------------------------------------------------

void MySQLRecognitionBase::set_sql_mode(const std::string &sql_mode) {
  unsigned result = 0;

  std::string sql_mode_string = base::toupper(sql_mode);
  std::istringstream iss(sql_mode_string);
  std::string mode;
  while (std::getline(iss, mode, ',')) {
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
}

//--------------------------------------------------------------------------------------------------

void MySQLRecognitionBase::set_sql_mode(unsigned sql_mode) {
  d->_sql_mode = sql_mode;
}

//--------------------------------------------------------------------------------------------------

const std::set<std::string> MySQLRecognitionBase::charsets() const {
  return d->_charsets;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if the given string is one of the supported charsets.
 */
bool MySQLRecognitionBase::is_charset(const std::string &s) const {
  return d->_charsets.find(s) != d->_charsets.end();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is an identifier. This includes all those keywords that are
 * allowed as identifiers too.
 */
bool MySQLRecognitionBase::isIdentifier(ANTLR3_UINT32 type) const {
  bool result = (type == IDENTIFIER) || (type == BACK_TICK_QUOTED_ID);
  if (!result) {
    // Double quoted text represents identifiers only if the ANSI QUOTES sql mode is active.
    result = ((d->_sql_mode & SQL_MODE_ANSI_QUOTES) != 0) && (type == DOUBLE_QUOTED_TEXT);
    if (!result) {
      // Keyword check. See also predefined.tokens in the grammar folder (which contains
      // manually assigned token ids for these keywords) and MySQL.g (keyword and keyword_sp rules).
      // Symbols are sorted so that keywords allowed as identifiers are in a continuous range
      // making this check easy (and reduce the parser size significantly compared to generated token ids).
      result = (type >= ASCII_SYMBOL && type <= YEAR_SYMBOL);
    }
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
* Returns the token value for a given keyword, which can be used to do search/replace operations.
* Returns INVALID_TOKEN if the keyword cannot be found.
*/
static std::map<std::string, uint32_t> keywords;  // One map for all recognizers.
extern "C" pANTLR3_UINT8 MySQLParserTokenNames[]; // Defined in MySQLParser.

uint32_t MySQLRecognitionBase::get_keyword_token(const std::string &keyword) const {
  if (keywords.size() == 0) {
    for (uint32_t i = 4; i <= ZEROFILL_SYMBOL; ++i) {
      std::string name((char *)MySQLParserTokenNames[i]);
      if (base::hasSuffix(name, "_SYMBOL"))
        keywords[name.substr(0, name.size() - 7)] = i;
      else if (base::hasSuffix(name, "_OPERATOR"))
        keywords[name.substr(0, name.size() - 9)] = i;
      else
        keywords[name] = i;
    }
  }

  std::string lookup = base::toupper(keyword);
  if (keywords.find(lookup) == keywords.end())
    return INVALID_TOKEN;
  return keywords[lookup];
}

//--------------------------------------------------------------------------------------------------

/**
* Returns the text of the given node. The result depends on the type of the node. If it represents
* a quoted text entity then two consecutive quote chars are replaced by a single one and if
* escape sequence parsing is not switched off (as per sql mode) then such sequences are handled too.
*/
std::string MySQLRecognitionBase::tokenText(pANTLR3_BASE_TREE node, bool keepQuotes) const {
  pANTLR3_STRING text = node->getText(node);
  if (text == NULL)
    return "";

  std::string chars;
  pANTLR3_COMMON_TOKEN token = node->getToken(node);
  ANTLR3_UINT32 type = (token != NULL) ? token->type : ANTLR3_TOKEN_INVALID;

  if (type == STRING_TOKEN) {
    // STRING is the grouping subtree for multiple consecutive string literals, which
    // must be concatenated.
    for (ANTLR3_UINT32 index = 0; index < node->getChildCount(node); index++) {
      pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)node->getChild(node, index);
      chars += tokenText(child, keepQuotes);
    }

    return chars;
  }

  chars = (const char *)text->chars;
  std::string quote_char;
  switch (type) {
    case BACK_TICK_QUOTED_ID:
      quote_char = "`";
      break;
    case SINGLE_QUOTED_TEXT:
      quote_char = "'";
      break;
    case DOUBLE_QUOTED_TEXT:
      quote_char = "\"";
      break;
    default:
      return chars;
  }

  // First unquote then handle escape squences and double quotes.
  if (chars.size() < 3) {
    if (keepQuotes)
      return chars;
    return ""; // Also handles an invalid single quote char gracefully.
  }

  // Need to unquote even if keepQuotes is true to avoid trouble with replacing the outer quotes.
  // Add them back below.
  chars = base::unquote(chars);

  if ((d->_sql_mode & SQL_MODE_NO_BACKSLASH_ESCAPES) == 0)
    chars = base::unescape_sql_string(chars, quote_char[0]);
  else if (token->user1 > 0) {
    // The field user1 is set by the parser to the number of quote char pairs it found.
    // So we can use it to shortcut our handling here.
    base::replaceStringInplace(chars, quote_char + quote_char, quote_char);
  }

  if (keepQuotes)
    return quote_char + chars + quote_char;
  return chars;
}

//--------------------------------------------------------------------------------------------------

/**
 * If the given node is a subtree this function collects the original text for all tokens in this tree,
 * including all tokens on the hidden channel (whitespaces).
 */
std::string MySQLRecognitionBase::textForTree(pANTLR3_BASE_TREE node) const {
  if (node->getChildCount(node) == 0)
    return "";

  std::string result;

  // Find the start of the first real node (no virtual nodes).
  pANTLR3_BASE_TREE child = node;
  while (child->getChildCount(child) > 0)
    child = (pANTLR3_BASE_TREE)child->getChild(child, 0);

  pANTLR3_COMMON_TOKEN token = child->getToken(child);
  ANTLR3_MARKER start = token->start;

  // Similar for the last node.
  child = node;
  while (child->getChildCount(child) > 0)
    child = (pANTLR3_BASE_TREE)child->getChild(child, child->getChildCount(child) - 1);

  token = child->getToken(child);
  ANTLR3_MARKER stop = token->stop;
  return std::string((char *)start, stop - start + 1);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is a keyword.
 */
bool MySQLRecognitionBase::isKeyword(ANTLR3_UINT32 type) const {
  if (type >= ACTION_SYMBOL && type <= PARTITION_SYMBOL)
    return true;

  switch (type) {
    case AT_SIGN_SYMBOL:
    case AT_AT_SIGN_SYMBOL:
    case BACK_TICK:
    case BACK_TICK_QUOTED_ID:
    case BIN_NUMBER:
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
    case COLUMN_NAME_TOKEN:
    case FLOAT_NUMBER:
    case FUNCTION_CALL_TOKEN:
    case GREATER_OR_EQUAL_OPERATOR:
    case GREATER_THAN_OPERATOR:
    case HEXDIGIT:
    case HEX_NUMBER:
    case IDENTIFIER:
    case INDEX_HINT_LIST_TOKEN:
    case NUMBER:
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
    case TABLE_NAME_TOKEN:
    case UNDERLINE_SYMBOL:
    case UNDERSCORE_CHARSET:
    case VERSION_COMMENT:
    case VERSION_COMMENT_END:
    case VERSION_COMMENT_INTRODUCER:
    case VERSION_COMMENT_START_TOKEN:
    case VERSION_COMMENT_TAIL:
    case WHITESPACE:
    case XA_ID_TOKEN:
    case ANTLR3_TOKEN_EOF:
      return false;

    default:
      return true;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is a relation token.
 */
bool MySQLRecognitionBase::is_relation(ANTLR3_UINT32 type) {
  switch (type) {
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
bool MySQLRecognitionBase::is_number(ANTLR3_UINT32 type) {
  switch (type) {
    case NUMBER:
    case FLOAT_NUMBER:
    case HEX_NUMBER:
    case BIN_NUMBER:
      return true;

    default:
      return false;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given token is an operator or punctuation character.
 */
bool MySQLRecognitionBase::is_operator(ANTLR3_UINT32 type) {
  switch (type) {
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

void MySQLRecognitionBase::reset() {
  d->_error_info.clear();
}

//--------------------------------------------------------------------------------------------------
