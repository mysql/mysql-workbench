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

#include "base/string_utilities.h"
#include "base/util_functions.h"

#include "mysql_parser_module.h"
#include "MySQLLexer.h"

using namespace grt;
using namespace parser;

GRT_MODULE_ENTRY_POINT(MySQLParserServicesImpl);

//--------------------------------------------------------------------------------------------------

grt::BaseListRef MySQLParserServicesImpl::getSqlStatementRanges(const std::string &sql)
{
  grt::BaseListRef list(get_grt());
  std::vector<std::pair<size_t, size_t> > ranges;

  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);

  for (std::vector<std::pair<size_t,size_t> >::const_iterator i = ranges.begin(); i != ranges.end(); ++i)
  {
    grt::BaseListRef item(get_grt());
    item.ginsert(grt::IntegerRef(i->first));
    item.ginsert(grt::IntegerRef(i->second));
    list.ginsert(item);
  }
  return list;
}

//--------------------------------------------------------------------------------------------------

/**
 * Signals any ongoing process to stop. This must be called from a different thread than from where
 * the processing was started to make it work.
 */
int MySQLParserServicesImpl::stopProcessing()
{
  _stop = true;
  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given sql as trigger create script and fills all found details in the given trigger ref.
 * If there's an error nothing is changed.
 * Returns the number of errors.
 */
int MySQLParserServicesImpl::parseTrigger(parser::ParserContext::Ref context, db_TriggerRef trigger,
                                          const std::string &sql)
{
  trigger->sqlDefinition(sql);
  trigger->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, QtCreateTrigger);
  int error_count = (int)context->recognizer()->error_info().size();
  int result_flag = 0;
  if (error_count == 0)
  {
    // There's no need for checks if any of the walker calls fail.
    //  If we arrive here the syntax must be correct.
    trigger->enabled(1);

    MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
    walker.next(2); // Skip root + CREATE.

    if (walker.token_type() == DEFINER_SYMBOL)
    {
      walker.next(2); // Skip DEFINER + equal sign.
      trigger->definer(walker.token_text());
      walker.next();
      if (walker.token_type() == OPEN_PAR_SYMBOL) // Optional parentheses.
        walker.next(2);
    }
    walker.next(); // skip TRIGGER
    std::string name = walker.token_text();
    walker.next();
    if (walker.token_type() == DOT_SYMBOL)
    {
      // A qualified name. Ignore the schema part.
      walker.next();
      name = walker.token_text();
      walker.next();
    }
    trigger->name(name);

    trigger->timing(walker.token_text());
    walker.next();
    trigger->event(walker.token_text());

    // The referenced table is not stored in the trigger object as that is defined by it's position
    // in the grt tree.
    walker.skip_token_sequence(ON_SYMBOL, TABLE_REF_ID_TOKEN, IDENTIFIER, FOR_SYMBOL, EACH_SYMBOL, ROW_SYMBOL, 0);
    ANTLR3_UINT32 type = walker.token_type();
    if (type == FOLLOWS_SYMBOL || type == PRECEDES_SYMBOL)
    {
      trigger->ordering(walker.token_text());
      walker.next();
      trigger->otherTrigger(walker.token_text());
      walker.next();
    }

    // sqlBody is obsolete.
  }
  else
  {
    result_flag = 1;

    // Finished with errors. See if we can get at least the trigger name out.
    MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
    if (walker.advance_to_type(TRIGGER_SYMBOL, true))
    {
      walker.next();
      std::string name = walker.token_text();
      walker.next();
      if (walker.token_type() == DOT_SYMBOL)
      {
        // A qualified name. Ignore the schema part.
        walker.next();
        name = walker.token_text();
        walker.next();
      }
      trigger->name(name);
    }
  }

  trigger->modelOnly(result_flag);
  if (trigger->owner().is_valid())
  {
    db_TableRef table = db_TableRef::cast_from(trigger->owner());
    table->customData().set("triggerInvalid", grt::IntegerRef(result_flag));
  }
  return error_count;
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given text as a specific query type (see parser for supported types).
 * Returns the error count.
 */
int MySQLParserServicesImpl::checkSqlSyntax(ParserContext::Ref context, const char *sql,
                                            size_t length, MySQLQueryType type)
{
  context->recognizer()->parse(sql, length, true, type);
  return (int)context->recognizer()->error_info().size();
}

//--------------------------------------------------------------------------------------------------

static const unsigned char* skip_leading_whitespace(const unsigned char *head, const unsigned char *tail)
{
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//--------------------------------------------------------------------------------------------------

bool is_line_break(const unsigned char *head, const unsigned char *line_break)
{
  if (*line_break == '\0')
    return false;

  while (*head != '\0' && *line_break != '\0' && *head == *line_break)
  {
    head++;
    line_break++;
  }
  return *line_break == '\0';
}

//--------------------------------------------------------------------------------------------------

/**
 * A statement splitter to take a list of sql statements and split them into individual statements,
 * return their position and length in the original string (instead the copied strings).
 */
int MySQLParserServicesImpl::determineStatementRanges(const char *sql, size_t length,
                                                      const std::string &initial_delimiter,
                                                      std::vector<std::pair<size_t, size_t> > &ranges,
                                                      const std::string &line_break)
{
  _stop = false;
  std::string delimiter = initial_delimiter.empty() ? ";" : initial_delimiter;
  const unsigned char *delimiter_head = (unsigned char*)delimiter.c_str();

  const unsigned char keyword[] = "delimiter";

  const unsigned char *head = (unsigned char *)sql;
  const unsigned char *tail = head;
  const unsigned char *end = head + length;
  const unsigned char *new_line = (unsigned char*)line_break.c_str();
  bool have_content = false; // Set when anything else but comments were found for the current statement.

  while (!_stop && tail < end)
  {
    switch (*tail)
    {
      case '/': // Possible multi line comment or hidden (conditional) command.
        if (*(tail + 1) == '*')
        {
          tail += 2;
          bool is_hidden_command = (*tail == '!');
          while (true)
          {
            while (tail < end && *tail != '*')
              tail++;
            if (tail == end) // Unfinished comment.
              break;
            else
            {
              if (*++tail == '/')
              {
                tail++; // Skip the slash too.
                break;
              }
            }
          }

          if (!is_hidden_command && !have_content)
            head = tail; // Skip over the comment.
        }
        else
          tail++;

        break;

      case '-': // Possible single line comment.
      {
        const unsigned char *end_char = tail + 2;
        if (*(tail + 1) == '-' && (*end_char == ' ' || *end_char == '\t' || is_line_break(end_char, new_line)))
        {
          // Skip everything until the end of the line.
          tail += 2;
          while (tail < end && !is_line_break(tail, new_line))
            tail++;
          if (!have_content)
            head = tail;
        }
        else
          tail++;

        break;
      }

      case '#': // MySQL single line comment.
        while (tail < end && !is_line_break(tail, new_line))
          tail++;
        if (!have_content)
          head = tail;
        break;

      case '"':
      case '\'':
      case '`': // Quoted string/id. Skip this in a local loop.
      {
        have_content = true;
        char quote = *tail++;
        while (tail < end && *tail != quote)
        {
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
      case 'D':
      {
        have_content = true;

        // Possible start of the keyword DELIMITER. Must be at the start of the text or a character,
        // which is not part of a regular MySQL identifier (0-9, A-Z, a-z, _, $, \u0080-\uffff).
        unsigned char previous = tail > (unsigned char *)sql ? *(tail - 1) : 0;
        bool is_identifier_char = previous >= 0x80
        || (previous >= '0' && previous <= '9')
        || ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z')
        || previous == '$'
        || previous == '_';
        if (tail == (unsigned char *)sql || !is_identifier_char)
        {
          const unsigned char *run = tail + 1;
          const unsigned char *kw = keyword + 1;
          int count = 9;
          while (count-- > 1 && (*run++ | 0x20) == *kw++)
            ;
          if (count == 0 && *run == ' ')
          {
            // Delimiter keyword found. Get the new delimiter (everything until the end of the line).
            tail = run++;
            while (run < end && !is_line_break(run, new_line))
              run++;
            delimiter = base::trim(std::string((char *)tail, run - tail));
            delimiter_head = (unsigned char*)delimiter.c_str();

            // Skip over the delimiter statement and any following line breaks.
            while (is_line_break(run, new_line))
              run++;
            tail = run;
            head = tail;
          }
          else
            tail++;
        }
        else
          tail++;

        break;
      }

      default:
        if (*tail > ' ')
          have_content = true;
        tail++;
        break;
    }

    if (*tail == *delimiter_head)
    {
      // Found possible start of the delimiter. Check if it really is.
      size_t count = delimiter.size();
      if (count == 1)
      {
        // Most common case. Trim the statement and check if it is not empty before adding the range.
        head = skip_leading_whitespace(head, tail);
        if (head < tail)
          ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
        head = ++tail;
        have_content = false;
      }
      else
      {
        const unsigned char *run = tail + 1;
        const unsigned char *del = delimiter_head + 1;
        while (count-- > 1 && (*run++ == *del++))
          ;

        if (count == 0)
        {
          // Multi char delimiter is complete. Tail still points to the start of the delimiter.
          // Run points to the first character after the delimiter.
          head = skip_leading_whitespace(head, tail);
          if (head < tail)
            ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
          tail = run;
          head = run;
          have_content = false;
        }
      }
    }
  }

  // Add remaining text to the range list.
  head = skip_leading_whitespace(head, tail);
  if (head < tail)
    ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
  
  return 0;
}

//--------------------------------------------------------------------------------------------------
