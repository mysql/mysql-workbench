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
#include "base/log.h"

#include "mysql_parser_module.h"
#include "MySQLLexer.h"

#include "grts/structs.db.mysql.h"

using namespace grt;
using namespace parser;

DEFAULT_LOG_DOMAIN("parser")

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
int MySQLParserServicesImpl::parseTrigger(parser::ParserContext::Ref context, db_mysql_TriggerRef trigger,
                                          const std::string &sql)
{
  log_debug("Parse trigger");

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
    walker.next(); // Skip CREATE.

    if (walker.token_type() == DEFINER_SYMBOL)
    {
      walker.next(2); // Skip DEFINER + equal sign.
      std::string definer = walker.token_text();
      walker.next();
      switch (walker.token_type())
      {
        case OPEN_PAR_SYMBOL: // Optional parentheses.
          walker.next(2);
          break;
        case AT_SIGN_SYMBOL: // A user@host entry.
          walker.next();
          definer += '@' + walker.token_text();
          walker.next();
          break;
      }
      trigger->definer(definer);
    }
    walker.next(2); // skip TRIGGER + TRIGGER_NAME_TOKEN
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
    walker.skip_token_sequence(ON_SYMBOL, TABLE_NAME_TOKEN, IDENTIFIER, FOR_SYMBOL, EACH_SYMBOL, ROW_SYMBOL, 0);
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
    if (walker.advance_to_type(TRIGGER_NAME_TOKEN, true))
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
 * Parses the given sql as create view script and fills all found details in the given view ref.
 * If there's an error nothing changes. If the sql contains a schema reference other than that the
 * the view is in the view's name will be changed (adds _WRONG_SCHEMA) to indicate that.
 */
int MySQLParserServicesImpl::parseView(parser::ParserContext::Ref context, db_mysql_ViewRef view, const std::string &sql)
{
  log_debug("Parse View");

  view->sqlDefinition(sql);
  view->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, QtCreateView);
  int error_count = (int)context->recognizer()->error_info().size();
  if (error_count == 0)
  {
    MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
    walker.next(); // Skip CREATE.
    walker.skip_if(OR_SYMBOL, 2);

    if (walker.token_type() == ALGORITHM_SYMBOL)
    {
      walker.next(2); // ALGORITHM and EQUAL.
      switch (walker.token_type())
      {
        case MERGE_SYMBOL:
          view->algorithm(1);
          break;
        case TEMPTABLE_SYMBOL:
          view->algorithm(2);
          break;
        default:
          view->algorithm(0);
          break;
      }
      walker.next();
    }
    else
      view->algorithm(0);

    if (walker.token_type() == DEFINER_SYMBOL)
    {
      walker.next(2); // Skip DEFINER + equal sign.
      std::string definer = walker.token_text();
      walker.next();
      switch (walker.token_type())
      {
        case OPEN_PAR_SYMBOL: // Optional parentheses.
          walker.next(2);
          break;
        case AT_SIGN_SYMBOL: // A user@host entry.
          walker.next();
          definer += '@' + walker.token_text();
          walker.next();
          break;
      }
      view->definer(definer);
    }

    walker.skip_if(SQL_SYMBOL, 3); // SQL SECURITY (DEFINER | INVOKER)

    walker.next(2); // skip VIEW + VIEW_NAME_TOKEN
    std::string name = walker.token_text();
    walker.next();
    if (walker.token_type() == DOT_SYMBOL)
    {
      // A qualified name. Check schema part.
      walker.next();
      db_SchemaRef schema = db_SchemaRef::cast_from(view->owner());
      if (!base::same_string(schema->name(), name, context->case_sensitive()))
        name = walker.token_text() + "_WRONG_SCHEMA";
      else
        name = walker.token_text();

      walker.next();
    }
    view->name(name);
    view->modelOnly(0);

    walker.next(2); // AS + SELECT subtree.
    if (walker.token_type() == WITH_SYMBOL)
      view->withCheckCondition(1);
    else
      view->withCheckCondition(0); // WITH_SYMBOL (CASCADED_SYMBOL | LOCAL_SYMBOL)? CHECK_SYMBOL OPTION_SYMBOL
  }
  else
  {
    // Finished with errors. See if we can get at least the view name out.
    MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
    if (walker.advance_to_type(VIEW_NAME_TOKEN, true))
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
      view->name(name);
    }
    view->modelOnly(1);
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

/**
 * Helper to collect text positions to references of the given schema.
 * We only come here if there was no syntax error.
 */
void collect_schema_name_offsets(ParserContext::Ref context, std::list<int> &offsets, const std::string schema_name)
{
  // Don't try to optimize the creation of the walker. There must be a new instance for each parse run
  // as it stores references to results in the parser.
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  bool case_sensitive = context->case_sensitive();
  while (walker.next()) {
    switch (walker.token_type())
    {
      case SCHEMA_NAME_TOKEN:
        if (base::same_string(walker.token_text(), schema_name, case_sensitive))
          offsets.push_back(walker.token_offset());
        break;

      case TABLE_NAME_TOKEN:
      {
        walker.next();
        if (walker.token_type() != DOT_SYMBOL && walker.look_ahead(DOT_SYMBOL))
        {
           // A table ref not with leading dot but a qualified identifier.
          if (base::same_string(walker.token_text(), schema_name, case_sensitive))
            offsets.push_back(walker.token_offset());
        }
        break;
      }

      case FIELD_NAME_TOKEN: // Field names only if they are fully qualified (schema.table.field/*).
        walker.next();
        if (walker.token_type() != DOT_SYMBOL && walker.look_ahead(DOT_SYMBOL))
        {
          // A leading dot means no schema.
          std::string name = walker.token_text();
          unsigned pos = walker.token_offset();
          walker.next(2);
          if (walker.look_ahead(DOT_SYMBOL)) // Fully qualified.
          {
            if (base::same_string(name, schema_name, case_sensitive))
              offsets.push_back(pos);
          }
        }
        break;

        // All those can have schema.id or only id.
      case VIEW_NAME_TOKEN:
      case TRIGGER_NAME_TOKEN:
      case PROCEDURE_NAME_TOKEN:
      case FUNCTION_NAME_TOKEN:
        walker.next();
        if (walker.look_ahead(DOT_SYMBOL))
        {
          if (base::same_string(walker.token_text(), schema_name, case_sensitive))
            offsets.push_back(walker.token_offset());
        }
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Replace all occurrences of the old by the new name according to the offsets list.
 */
void replace_schema_names(std::string &sql, const std::list<int> &offsets, size_t length,
                          const std::string new_name)
{
  bool remove_schema = new_name.empty();
  for (std::list<int>::const_reverse_iterator iterator = offsets.rbegin(); iterator != offsets.rend(); ++iterator)
  {
    std::string::size_type start = *iterator;
    std::string::size_type replace_length = length;
    if (remove_schema)
    {
      // Make sure we also remove quotes and the dot.
      if (start > 0 && sql[start - 1] == '`' || sql[start - 1] == '"')
      {
        --start;
        ++replace_length;
      }
      ++replace_length;
    }
    sql.replace(start, replace_length, new_name);
  }
}

//--------------------------------------------------------------------------------------------------

void rename_in_list(grt::ListRef<db_DatabaseDdlObject> list, ParserContext::Ref context, MySQLQueryType type,
                    const std::string old_name, const std::string new_name)
{
  for (size_t i = 0; i < list.count(); ++i)
  {
    std::string sql = list[i]->sqlDefinition();
    context->recognizer()->parse(sql.c_str(), sql.size(), true, type);
    size_t error_count = (int)context->recognizer()->error_info().size();
    if (error_count == 0)
    {
      MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();

      std::list<int> offsets;
      collect_schema_name_offsets(context, offsets, old_name);
      if (!offsets.empty())
      {
        replace_schema_names(sql, offsets, old_name.size(), new_name);
        list[i]->sqlDefinition(sql);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Goes through all schemas in the catalog and changes all db objects to refer to the new name if they
 * currently refer to the old name. We also iterate non-related schemas in order to have some
 * consolidation/sanitzing in effect where wrong schema references were used.
 */
int MySQLParserServicesImpl::renameSchemaReferences(ParserContext::Ref context, db_mysql_CatalogRef catalog,
                                                    const std::string old_name, const std::string new_name)
{
  log_debug("Rename schema references");

  ListRef<db_mysql_Schema> schemas = catalog->schemata();
  for (size_t i = 0; i < schemas.count(); ++i)
  {
    db_mysql_SchemaRef schema = schemas[i];
    rename_in_list(schema->views(), context, QtCreateView, old_name, new_name);
    rename_in_list(schema->routines(), context, QtCreateRoutine, old_name, new_name);

    grt::ListRef<db_mysql_Table> tables = schemas[i]->tables();
    for (grt::ListRef<db_mysql_Table>::const_iterator iterator = tables.begin(); iterator != tables.end(); ++iterator)
      rename_in_list((*iterator)->triggers(), context, QtCreateTrigger, old_name, new_name);
  }
  
  return 0;
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
