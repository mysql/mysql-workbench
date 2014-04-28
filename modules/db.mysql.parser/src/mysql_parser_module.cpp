/* 
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include "base/string_utilities.h"

#include "mysql_parser_module.h"

GRT_MODULE_ENTRY_POINT(MySQLParserServicesImpl);

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

static const char* skip_leading_whitespace(const char *head, const char *tail)
{
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines the range for each statement in the script and adds them to the ranges parameter.
 * Even though there's no Unicode specific handling here this function is still working well for any
 * Unicode character.
 */
int MySQLParserServicesImpl::determineStatementRanges(const char *sql, size_t length, const std::string &initial_delimiter, 
  std::vector<std::pair<size_t, size_t> > &ranges)
{
  _stop = false;
  std::string delimiter = initial_delimiter.empty() ? ";" : initial_delimiter;
  const char *delimiter_head = delimiter.c_str();

  const char keyword[] = "delimiter";

  const char *head = sql;
  const char *tail = head;
  const char *end = head + length;
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
        char end_char = *(tail + 2);
        if (*(tail + 1) == '-' && (end_char == ' ' || end_char == '\t' || end_char == '\n' || end_char == '\r'))
        {
          // Skip everything until the end of the line.
          tail += 2;
          while (tail < end && *tail != '\n' && *tail != '\r')
            tail++;
          if (!have_content)
            head = tail;
        }
        else
          tail++;

        break;
      }

    case '#': // MySQL single line comment.
      while (tail < end && *tail != '\n' && *tail != '\r')
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
        unsigned char previous = tail > sql ? *(tail - 1) : 0;
        bool is_identifier_char = previous >= 0x80 
          || (previous >= '0' && previous <= '9')
          || ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z')
          || previous == '$'
          || previous == '_';
        if (tail == sql || !is_identifier_char)
        {
          const char *run = tail + 1;
          const char *kw = keyword + 1;
          int count = 9;
          while (count-- > 1 && (*run++ | 0x20) == *kw++)
            ;
          if (count == 0 && *run == ' ')
          {
            // Delimiter keyword found. Get the new delimiter (everything til the end of the line).
            tail = run++;
            while (run < end && *run != '\n' && *run != '\r')
              run++;
            delimiter = base::trim(std::string(tail, run - tail));
            delimiter_head = delimiter.c_str();

            // Skip over the delimiter statement and any following line breaks.
            while (*run == '\r' || *run == '\n')
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
          ranges.push_back(std::make_pair<size_t, size_t>(head - sql, tail - head));
        head = ++tail;
        have_content = false;
      }
      else
      {
        const char *run = tail + 1;
        const char *del = delimiter_head + 1;
        while (count-- > 1 && (*run++ == *del++))
          ;

        if (count == 0)
        {
          // Multi char delimiter is complete. Tail still points to the start of the delimiter.
          // Run points to the first character after the delimiter.
          head = skip_leading_whitespace(head, tail);
          if (head < tail)
            ranges.push_back(std::make_pair<size_t, size_t>(head - sql, tail - head));
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
    ranges.push_back(std::make_pair<size_t, size_t>(head - sql, tail - head));

  return 0;
}

//--------------------------------------------------------------------------------------------------

grt::BaseListRef MySQLParserServicesImpl::getSqlStatementRanges(const std::string &sql)
{
  grt::BaseListRef list(get_grt());
  std::vector<std::pair<size_t, size_t> > ranges;

  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);

  for (std::vector<std::pair<size_t,size_t> >::const_iterator i = ranges.begin(); i != ranges.end(); ++i)
  {
    grt::BaseListRef item(get_grt());
    item.ginsert(grt::IntegerRef((long)i->first));
    item.ginsert(grt::IntegerRef((long)i->second));
    list.ginsert(item);
  }
  return list;
}

//--------------------------------------------------------------------------------------------------

