/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <boost/assign/std/vector.hpp> // for 'operator += ..'

#include "sql_editor_be.h"
#include "sqlide.h"
#include "grt/grt_manager.h"

#include "base/log.h"
#include "base/string_utilities.h"

#include "mforms/code_editor.h"

#include "autocomplete_object_name_cache.h"
#include "mysql-parser.h"

#include "MySQLLexer.h"

DEFAULT_LOG_DOMAIN("Code Completion");

using namespace boost::assign;

using namespace bec;
using namespace grt;
using namespace base;

//--------------------------------------------------------------------------------------------------

void Sql_editor::setup_auto_completion()
{
  _code_editor->auto_completion_options(true, true, false, true, false);
  _code_editor->auto_completion_max_size(40, 15);

  static std::vector<std::pair<int, std::string> > ac_images;
  if (ac_images.size() == 0)
    ac_images +=
      std::make_pair(AC_KEYWORD_IMAGE, "auto-completion-keyword.png"),
      std::make_pair(AC_SCHEMA_IMAGE, "auto-completion-schema.png"),
      std::make_pair(AC_TABLE_IMAGE, "auto-completion-table.png"),
      std::make_pair(AC_ROUTINE_IMAGE, "auto-completion-routine.png"),
      std::make_pair(AC_FUNCTION_IMAGE, "auto-completion-function.png"),
      std::make_pair(AC_VIEW_IMAGE, "auto-completion-view.png"),
      std::make_pair(AC_COLUMN_IMAGE, "auto-completion-column.png"),
      std::make_pair(AC_OPERATOR_IMAGE, "auto-completion-operator.png"),
      std::make_pair(AC_ENGINE_IMAGE, "auto-completion-engine.png");

  _code_editor->auto_completion_register_images(ac_images);
  _code_editor->auto_completion_stops("\t,.*;)"); // Will close ac even if we are in an identifier.
  _code_editor->auto_completion_fillups("");
}

//--------------------------------------------------------------------------------------------------

/**
 * Updates the auto completion list by filtering the determined entries by the text the user
 * already typed. If auto completion is not yet active it becomes active here.
 * Returns the list sent to the editor for unit tests to validate them.
 */
std::vector<std::pair<int, std::string> >  Sql_editor::update_auto_completion(const std::string &typed_part)
{
  log_debug2("Updating auto completion popup in editor\n");

  // Remove all entries that don't start with the typed text before showing the list.
  if (!typed_part.empty())
  {
    gchar *prefix = g_utf8_casefold(typed_part.c_str(), -1);
    
    std::vector<std::pair<int, std::string> > filtered_entries;
    for (std::vector<std::pair<int, std::string> >::iterator iterator = _auto_completion_entries.begin();
      iterator != _auto_completion_entries.end(); ++iterator)
    {
      gchar *entry = g_utf8_casefold(iterator->second.c_str(), -1);
      if (g_str_has_prefix(entry, prefix))
        filtered_entries.push_back(*iterator);
      g_free(entry);
    }
    
    g_free(prefix);

    /* TOOD: We can use this not before we have manual handling what gets inserted by auto completion.
    if (filtered_entries.empty())
      filtered_entries.push_back(std::pair<int, std::string>(0, _("no entry found")));
     */

    if (filtered_entries.size() > 0)
    {
      log_debug2("Showing auto completion popup\n");
      _code_editor->auto_completion_show(typed_part.size(), filtered_entries);
    }
    else
    {
      log_debug2("Nothing to autocomplete - hiding popup if it was active\n");
      _code_editor->auto_completion_cancel();
    }

    return filtered_entries;
  }
  else
  {
    if (_auto_completion_entries.size() > 0)
    {
      log_debug2("Showing auto completion popup\n");
      _code_editor->auto_completion_show(0, _auto_completion_entries);
    }
    else
    {
      log_debug2("Nothing to autocomplete - hiding popup if it was active\n");
      _code_editor->auto_completion_cancel();
    }
  }

  return _auto_completion_entries;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the text in the editor starting at the given position backwards until the line start.
 * If there's a back tick or double quote char then text until this quote char is returned. If there's
 * no quoting char but a space or dot char then everything up to (but not including) this is returned.
 */
std::string Sql_editor::get_written_part(size_t position)
{
  ssize_t line = _code_editor->line_from_position(position);
  ssize_t start, stop;
  _code_editor->get_range_of_line(line, start, stop);
  std::string text = _code_editor->get_text_in_range(start, position);
  if (text.empty())
    return "";
  
  const char *head = text.c_str();
  const char *run = head;

  while (*run != '\0')
  {
    if (*run == '\'' || *run == '"' || *run == '`')
    {
      // Entering a quoted text.
      head = run + 1;
      char quote_char = *run;
      while (true)
      {
        run = g_utf8_next_char(run);
        if (*run == quote_char || *run == '\0')
          break;
        
        // If there's an escape char skip it and the next char too (if we didn't reach the end).
        if (*run == '\\')
        {
          run++;
          if (*run != '\0')
            run = g_utf8_next_char(run);
        }
      }
      if (*run == '\0') // Unfinished quoted text. Return everything.
        return head;
      head = run + 1; // Skip over this quoted text and start over.
    }
    run++;
  }
  
  // If we come here then we are outside any quoted text. Scan back for anything we consider
  // to be a word stopper (for now anything below '0', char code wise).
  while (head < run--)
  {
    if (*run < '0')
      return run + 1;
  }
  return head;
}

//--------------------------------------------------------------------------------------------------

struct CompareAcEntries
{
  bool operator() (const std::pair<int, std::string> &lhs, const std::pair<int, std::string> &rhs) const
  {
    return base::string_compare(lhs.second, rhs.second, false) < 0;
  }
};

//--------------------------------------------------------------------------------------------------

#define INCLUDE_PART(part) \
  context.wanted_parts = (Sql_editor::AutoCompletionWantedParts)(context.wanted_parts | part)
#define EXCLUDE_PART(part) \
  context.wanted_parts = (Sql_editor::AutoCompletionWantedParts)(context.wanted_parts & ~part)

#define PART_IF(condition, part) \
  context.wanted_parts = (Sql_editor::AutoCompletionWantedParts) ((condition) ? (context.wanted_parts | part) : (context.wanted_parts & ~part))

#define IS_PART_INCLUDED(part) \
  ((context.wanted_parts & part) != 0)

//--------------------------------------------------------------------------------------------------

/**
 * Updates the context structure's token info with the current token in the tree walker.
 */
void get_current_token_info(Sql_editor::AutoCompletionContext &context, MySQLRecognizerTreeWalker &walker)
{
  context.token_type = walker.token_type();
  context.token_line = walker.token_line();
  context.token_start = walker.token_start();
  context.token_length = walker.token_length();
  context.token = walker.token_text();
}

//--------------------------------------------------------------------------------------------------

/**
 * Set markers for runtime function names as well as identifiers (schema, table and column names) exclusively.
 */
void want_only_functions_schemas_tables_columns(Sql_editor::AutoCompletionContext &context)
{
  context.wanted_parts = Sql_editor::CompletionWantNothing;
  INCLUDE_PART(Sql_editor::CompletionWantRuntimeFunctions);
  INCLUDE_PART(Sql_editor::CompletionWantSchemas);
  INCLUDE_PART(Sql_editor::CompletionWantTables);
  INCLUDE_PART(Sql_editor::CompletionWantColumns);
}

//--------------------------------------------------------------------------------------------------

/**
 * Set markers for field references (including schemas) exclusively.
 */
void want_only_field_references(Sql_editor::AutoCompletionContext &context)
{
  context.wanted_parts = Sql_editor::CompletionWantNothing;
  INCLUDE_PART(Sql_editor::CompletionWantSchemas);
  INCLUDE_PART(Sql_editor::CompletionWantTables);
  INCLUDE_PART(Sql_editor::CompletionWantColumns);
}

//--------------------------------------------------------------------------------------------------

/**
 * Set markers for references (including schemas) exclusively.
 */
void want_only_table_references(Sql_editor::AutoCompletionContext &context)
{
  context.wanted_parts = Sql_editor::CompletionWantNothing;
  INCLUDE_PART(Sql_editor::CompletionWantSchemas);
  INCLUDE_PART(Sql_editor::CompletionWantTables);
}

//--------------------------------------------------------------------------------------------------

/**
 * Set markers for function names as well as identifiers (schema, table and column names) inclusively.
 */
void want_also_functions_schemas_tables_columns(Sql_editor::AutoCompletionContext &context)
{
  INCLUDE_PART(Sql_editor::CompletionWantRuntimeFunctions);
  INCLUDE_PART(Sql_editor::CompletionWantSchemas);
  INCLUDE_PART(Sql_editor::CompletionWantTables);
  INCLUDE_PART(Sql_editor::CompletionWantColumns);
}

//--------------------------------------------------------------------------------------------------

/**
 * Commonly used function to set markers for function normal and major keywords
 */
void want_only_keywords(Sql_editor::AutoCompletionContext &context)
{
  context.wanted_parts = Sql_editor::CompletionWantNothing;
  INCLUDE_PART(Sql_editor::CompletionWantMajorKeywords);
  INCLUDE_PART(Sql_editor::CompletionWantKeywords);
}

//--------------------------------------------------------------------------------------------------

/**
 * Keywords and functions allowed when starting a new (sub)expression.
 */
void want_also_expression_start(Sql_editor::AutoCompletionContext &context, bool withSelect) 
{
  INCLUDE_PART(Sql_editor::CompletionWantExprStartKeywords);
  INCLUDE_PART(Sql_editor::CompletionWantRuntimeFunctions);
  if (withSelect)
    INCLUDE_PART(Sql_editor::CompletionWantSelect);
}

//--------------------------------------------------------------------------------------------------

void want_only_expression_start(Sql_editor::AutoCompletionContext &context, bool withSelect) 
{
  context.wanted_parts = Sql_editor::CompletionWantExprStartKeywords;
  INCLUDE_PART(Sql_editor::CompletionWantRuntimeFunctions);
  if (withSelect)
    INCLUDE_PART(Sql_editor::CompletionWantSelect);
}

//--------------------------------------------------------------------------------------------------

/**
 * Keywords that can only appear within an outer expression (e.g. "a > ALL (select ...)").
 */
void want_only_expression_continuation(Sql_editor::AutoCompletionContext &context) 
{
  context.wanted_parts = Sql_editor::CompletionWantExprInnerKeywords;
  want_also_functions_schemas_tables_columns(context);
}

//--------------------------------------------------------------------------------------------------

void check_error_context(Sql_editor::AutoCompletionContext &context, MySQLRecognizer &recognizer)
{
  log_debug2("Checking some error situations\n");

  // We got here in case of an error condition. We will try to get a usable context from the last
  // parse error found.
  switch (recognizer.error_info().back().token_type)
  {
    case COMMA_SYMBOL:
      want_only_field_references(context);
      want_also_expression_start(context, false);
      break;

    case MULT_OPERATOR:
      context.wanted_parts = Sql_editor::CompletionWantColumns;
      // fall through.
    case FROM_SYMBOL:
      INCLUDE_PART(Sql_editor::CompletionWantSchemas);
      INCLUDE_PART(Sql_editor::CompletionWantTables);
      break;

    case IDENTIFIER: // Probably starting a new query.
      context.wanted_parts = Sql_editor::CompletionWantMajorKeywords;
      break;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when we are in an identifier which is not specified as table or field ref.
 * Use the query type to know which kind of identifier we need.
 * Returns true if the object type could be found.
 */
bool check_by_query_type(MySQLRecognizerTreeWalker &walker, Sql_editor::AutoCompletionContext &context)
{
  MySQLQueryType type = walker.get_current_query_type();
  switch (type)
  {
  case QtDropDatabase:
    context.wanted_parts = Sql_editor::CompletionWantSchemas;
    break;
  case QtDropEvent:
    context.wanted_parts = Sql_editor::CompletionWantEvents;
    break;
  case QtDropFunction:
    context.wanted_parts = Sql_editor::CompletionWantFunctions;
    break;
  case QtDropProcedure:
    context.wanted_parts = Sql_editor::CompletionWantProcedures;
    break;
  case QtDropTable:
  case QtDropView:
    context.wanted_parts = Sql_editor::CompletionWantTables;
    break;
  case QtDropTrigger:
    context.wanted_parts = Sql_editor::CompletionWantTriggers;
    break;
  case QtDropIndex:
    context.wanted_parts = Sql_editor::CompletionWantIndexes;
    break;

  case QtCall:
    context.wanted_parts = Sql_editor::CompletionWantProcedures;
    break;

  default:
    return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given type is a top level element (e.g. the root node or any of the 
 * major keywords). Sub queries are handle just like top level queries.
 */
bool is_top_level(unsigned type, long version)
{
  switch (type)
  {
  case 0:
  case ANALYZE_SYMBOL:
  case ALTER_SYMBOL:
  case CALL_SYMBOL:
  case CHANGE_SYMBOL:
  case CHECK_SYMBOL:
  case CREATE_SYMBOL:
  case DELETE_SYMBOL:
  case DESC_SYMBOL:
  case DESCRIBE_SYMBOL:
  case DROP_SYMBOL:
  case EXPLAIN_SYMBOL:
  case FLUSH_SYMBOL:
  case GRANT_SYMBOL:
  case HANDLER_SYMBOL:
  case HELP_SYMBOL:
  case INSERT_SYMBOL:
  case KILL_SYMBOL:
  case LOAD_SYMBOL:
  case LOCK_SYMBOL:
  case OPTIMIZE_SYMBOL:
  case PURGE_SYMBOL:
  case RENAME_SYMBOL:
  case REPAIR_SYMBOL:
  case REPLACE_SYMBOL:
  case REVOKE_SYMBOL:
  case SELECT_SYMBOL:
  case SET_SYMBOL:
  case SHOW_SYMBOL:
  case TRUNCATE_SYMBOL:
  case UNLOCK_SYMBOL:
  case UPDATE_SYMBOL:
  case USE_SYMBOL:
    return true;

  case BACKUP_SYMBOL:
  case RESTORE_SYMBOL:
    if (version < 50500)
      return true;

    break;

  case RELEASE_SYMBOL:
    if (version >= 50100)
      return true;

    break;

  case REMOVE_SYMBOL:
    if (version >= 50100)
      return true;

    break;

  case INSTALL_SYMBOL:
  case UNINSTALL_SYMBOL:
  case XA_SYMBOL:
    if (version >= 50100)
      return true;

    break;

  case BINLOG_SYMBOL:
  case CACHE_SYMBOL:
  case CHECKSUM_SYMBOL:
  case COMMIT_SYMBOL:
  case DEALLOCATE_SYMBOL:
  case DO_SYMBOL:
  case EXECUTE_SYMBOL:
  case PARTITION_SYMBOL:
  case PREPARE_SYMBOL:
  case RESET_SYMBOL:
  case ROLLBACK_SYMBOL:
  case SAVEPOINT_SYMBOL:
  case START_SYMBOL:
  case STOP_SYMBOL:
    if (version >= 50500)
      return true;

    break;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * We are after a whitespace at the start of a new token.
 */
void check_new_token_start(MySQLRecognizerTreeWalker &walker, Sql_editor::AutoCompletionContext &context)
{
  if (walker.is_identifier() && !walker.is_keyword()) // Certain keywords can be identifiers too.
  {
    context.check_identifier = false;

    walker.up();
    unsigned type = walker.token_type();
    if (is_top_level(type, context.version))
      type = 0;
    switch (type)
    {
    case 0: // If this is a top level node then we are probably within a chain of identifiers and keywords.
    case TABLE_REF_ID_TOKEN:
      {
        MySQLQueryType type = walker.get_current_query_type();
        switch (type)
        {
        case QtExplainStatement:
          // After a table ref in an explain statement can only be a column.
          context.wanted_parts = Sql_editor::CompletionWantColumns;
          context.check_identifier = false;
          break;

        default:
          context.wanted_parts = Sql_editor::CompletionWantKeywords; 
          break;
        }
        break;
      }

    case DEFINER_SYMBOL:
      context.wanted_parts = Sql_editor::CompletionWantKeywords; 
      break;

    case FIELD_REF_ID_TOKEN:
      // If we are in a function call or par expression then nothing can be shown, as only
      // identifiers or operators are valid.
      // Otherwise however we might just enter the next query part, so we show keywords.
      if (walker.up())
      {
        if (walker.token_type() == SELECT_EXPR_TOKEN)
          context.wanted_parts = Sql_editor::CompletionWantKeywords;
        else
          context.wanted_parts = Sql_editor::CompletionWantNothing;
      }
      else
        context.wanted_parts = Sql_editor::CompletionWantKeywords;
      break;
    }
  }
  else
  {
    switch (walker.token_type())
    {
    case OPEN_PAR_SYMBOL:
      walker.up();
      switch (walker.token_type())
      {
      case KEY_CACHE_LIST_TOKEN:
        context.wanted_parts = Sql_editor::CompletionWantIndexes;
        context.check_identifier = false;
        break;

      case UNION_SYMBOL:
        context.wanted_parts = Sql_editor::CompletionWantSelect;
        context.check_identifier = false;
        break;

      default:
        want_only_functions_schemas_tables_columns(context);

        // If we are not in a function call we can also offer sub queries.
        want_also_expression_start(context, walker.token_type() != FUNCTION_CALL_TOKEN);
        break;
      }
      break;

    case SELECT_SYMBOL:
    case WHERE_SYMBOL:
    case HAVING_SYMBOL:
      want_only_functions_schemas_tables_columns(context);
      want_also_expression_start(context, false);
      break;

    case COMMA_SYMBOL:
      {
        unsigned type = walker.parent_type();
        if (is_top_level(type, context.version))
          type = 0;
        switch (type)
        {
        case 0: // We are already at top level. Normal for simple queries (e.g. grant/revoke).
          {
            MySQLQueryType type = walker.get_current_query_type();
            switch (type)
            {
            case QtAnalyzeTable:
            case QtRepairTable:
              context.wanted_parts = Sql_editor::CompletionWantTables;
              context.check_identifier = false;
              break;

            case QtRenameUser:
              context.wanted_parts = Sql_editor::CompletionWantUsers;
              context.check_identifier = false;
              break;

            case QtFlush:
              context.wanted_parts = Sql_editor::CompletionWantKeywords;
              context.check_identifier = false;
              break;

            default:
              want_only_functions_schemas_tables_columns(context);
              want_also_expression_start(context, false);
              break;
            }
            break;
          }

        case REFERENCES_SYMBOL: // Key definition.
          context.wanted_parts = Sql_editor::CompletionWantColumns;
          context.check_identifier = false;
          break;

        case CHANGE_MASTER_OPTIONS_TOKEN:
        case SLAVE_THREAD_OPTIONS_TOKEN:
          context.wanted_parts = Sql_editor::CompletionWantKeywords;
          context.check_identifier = false;
          break;

        default:
          want_only_functions_schemas_tables_columns(context);
          want_also_expression_start(context, false);
          break;
        }
        break;
      }

    case BY_SYMBOL:
      if (walker.previous_sibling() && (walker.token_type() == ORDER_SYMBOL || walker.token_type() == GROUP_SYMBOL))
        want_only_functions_schemas_tables_columns(context);
      break;

    case CALL_SYMBOL:
      context.wanted_parts = Sql_editor::CompletionWantProcedures;
      context.check_identifier = true;
      break;

    case FROM_SYMBOL:
      {
        MySQLQueryType type = walker.get_current_query_type();
        switch (type)
        {
        case QtRevoke:
          context.wanted_parts = Sql_editor::CompletionWantUsers;
          context.check_identifier = false;
          break;

        default:
          want_only_functions_schemas_tables_columns(context);
          want_also_expression_start(context, false);
          break;
        }
        break;
      }

    case TABLE_REF_ID_TOKEN:
      want_only_table_references(context);
      context.check_identifier = true;
      break;

    case FIELD_REF_ID_TOKEN:
      // Walk up the parent chain jumping over all math subtrees.
      while (walker.up() && walker.is_relation())
        ;
      switch (walker.token_type())
      {
      case SELECT_EXPR_TOKEN:
        want_only_functions_schemas_tables_columns(context);
        want_also_expression_start(context, false);
        break;
      case FUNCTION_CALL_TOKEN:
        want_only_field_references(context);
        context.check_identifier = false;
        break;

      case GROUP_SYMBOL:
      case ORDER_SYMBOL:   // Expressions after ORDER BY and GROUP BY.
      case PAR_EXPRESSION_TOKEN: // Expressions after an opening parenthesis.
        want_only_field_references(context);
        want_also_expression_start(context, walker.token_type() == PAR_EXPRESSION_TOKEN);
        context.check_identifier = false;
        break;
      }
      break;

    case SET_SYMBOL:
      context.wanted_parts = Sql_editor::CompletionWantTables;
      INCLUDE_PART(Sql_editor::CompletionWantColumns);
      break;

    case FUNCTION_CALL_TOKEN:
      want_only_functions_schemas_tables_columns(context);
      want_also_expression_start(context, false);
      break;

    case PAR_EXPRESSION_TOKEN: // At the beginning of a par expression. See where we come from.
      if (walker.previous())
      {
        switch (walker.token_type())
        {
        case WHERE_SYMBOL:
          want_only_functions_schemas_tables_columns(context);
          INCLUDE_PART(Sql_editor::CompletionWantExprStartKeywords);
          break;
        }
      }
      break;

    case CLOSE_PAR_SYMBOL:
      // Finishing a par expression or function call. Could be part of an expression (we don't show operators),
      // an alias could follow (nothing to show) or the next query part comes next, so we show keywords.
      context.wanted_parts = Sql_editor::CompletionWantKeywords;
      context.check_identifier = false;
      break;

    case GROUP_SYMBOL:
    case ORDER_SYMBOL:
    case IDENTIFIED_SYMBOL:
      context.wanted_parts = Sql_editor::CompletionWantBy;
      context.check_identifier = false;
      break;

    case DATABASE_SYMBOL:
      context.wanted_parts = Sql_editor::CompletionWantSchemas;
      context.check_identifier = false;
      break;

    case DOT_SYMBOL:
      switch (walker.parent_type())
      {
      case TABLE_REF_ID_TOKEN:
      case FIELD_REF_ID_TOKEN:
        context.check_identifier = true;
        want_only_field_references(context);
        break;
      default:
        // Other kind of references, e.g. when dropping objects.
        if (check_by_query_type(walker, context))
        {
          if (walker.previous())
          {
            context.check_identifier = false;
            context.table_schema = walker.token_text();
          }
        }
      }
      break;

    case OPEN_CURLY_SYMBOL: // At the start of an ODBC query.
    case SAVEPOINT_SYMBOL:  // After e.g. RELEASE SAVEPOINT.
      // Expecting any identifier (no reference).
      context.wanted_parts = Sql_editor::CompletionWantNothing;
      context.check_identifier = false;
      break;

      break;

    case FUNCTION_SYMBOL:
      if (walker.parent_type() == PRIVILEGE_TARGET_TOKEN)
      {
        context.wanted_parts = Sql_editor::CompletionWantFunctions;
        context.check_identifier = true;
      }
      break;

    case PROCEDURE_SYMBOL:
      if (walker.parent_type() == PRIVILEGE_TARGET_TOKEN)
      {
        context.wanted_parts = Sql_editor::CompletionWantProcedures;
        context.check_identifier = true;
      }
      break;

    case TABLE_SYMBOL:
    case ON_SYMBOL:
      if (walker.parent_type() == PRIVILEGE_TARGET_TOKEN)
      {
        context.wanted_parts = Sql_editor::CompletionWantTables;
        context.check_identifier = true;
      }
      break;

    case MULT_OPERATOR: // Either wildcard or multiplication.
      switch (walker.parent_type())
      {
      case 0:
      case OPEN_PAR_SYMBOL:
        // On top level of a query, so it's a wildcard.
        context.wanted_parts = Sql_editor::CompletionWantKeywords;
        context.check_identifier = false;
        break;

      default:
        want_only_expression_continuation(context);
        break;
      }
      break;

    default:
      if (walker.is_keyword())
      {
        // After any of a keyword not handled above. Continuing with another keyword.
        context.wanted_parts = Sql_editor::CompletionWantKeywords;
        context.check_identifier = false;
      }
      else
      {
        if (walker.is_number())
        {
          // Within an expression, probably starting a new rhs.
          context.wanted_parts = Sql_editor::CompletionWantExprInnerKeywords;
          context.check_identifier = false;
        }
        else
          if (walker.is_relation())
            want_only_expression_continuation(context);
      }
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * We are within a token (not at the first position though, but including the position directly after
 * the last char of the token).
 */
void check_current_token(MySQLRecognizerTreeWalker &walker, Sql_editor::AutoCompletionContext &context)
{
  bool look_at_previous = false;
  bool look_at_previous_sibling = false;
  switch (walker.token_type())
  {
  case ANTLR3_TOKEN_INVALID: // We are at the start of a query.
    context.check_identifier = false;
    break;

  case COMMA_SYMBOL:
    look_at_previous_sibling = true;
    break;

  default:
    if (walker.is_relation())
    {
      context.wanted_parts = Sql_editor::CompletionWantExprInnerKeywords;
      context.check_identifier = false;
      return;
    }
    else
      look_at_previous = walker.is_identifier() || walker.is_keyword();
  }

  if (look_at_previous || look_at_previous_sibling)
  {
    // If this is the first token then we are starting a query and only show major keywords
    // (which is on by default).
    if (!(look_at_previous ? walker.previous() : walker.previous_sibling()))
    {
      walker.remove_tos();
      return;
    }

    // Second round.
    switch (walker.token_type())
    {
    case ANTLR3_TOKEN_INVALID: // We are at the start of a query.
      context.check_identifier = false;
      break;

    case DOT_SYMBOL:
      switch (walker.parent_type())
      {
      case TABLE_REF_ID_TOKEN:
      case FIELD_REF_ID_TOKEN:
        context.check_identifier = true;
        want_only_field_references(context);
        break;
      default:
        // Other kind of references, e.g. when dropping objects.
        if (check_by_query_type(walker, context))
        {
          if (walker.previous())
          {
            context.check_identifier = false;
            context.table_schema = walker.token_text();
          }
        }
      }
      break;

    case OPEN_PAR_SYMBOL: // Subquery, function parameters or par expression (including element lists).
      {
        switch (walker.parent_type())
        {
        case FUNCTION_CALL_TOKEN:
          // Some functions allow keywords, like count(distinct ...).
          context.wanted_parts = Sql_editor::CompletionWantKeywords;
          INCLUDE_PART(Sql_editor::CompletionWantRuntimeFunctions);
          context.check_identifier = true;
          break;

        case PAR_EXPRESSION_TOKEN:
          // Nested function calls or field references.
          want_only_functions_schemas_tables_columns(context);
          want_also_expression_start(context, false);
          context.check_identifier = true;

          break;
        case SUBQUERY_TOKEN:
          context.wanted_parts = Sql_editor::CompletionWantSelect;
          context.check_identifier = false;
          break;

        case ALTER_TABLE_ITEM_TOKEN:
          context.wanted_parts = Sql_editor::CompletionWantColumns;
          context.check_identifier = false;
          break;

        case UNION_SYMBOL:
          context.wanted_parts = Sql_editor::CompletionWantSelect;
          context.check_identifier = false;
          break;
        }
      }
      break;

    case SELECT_SYMBOL:
    case WHERE_SYMBOL:
    case HAVING_SYMBOL:
    case PLUS_OPERATOR:
    case MINUS_OPERATOR:
    case MULT_OPERATOR:
    case DIV_OPERATOR:
    case MOD_OPERATOR:
    case DIV_SYMBOL:
    case MOD_SYMBOL:
    case EQUAL_OPERATOR:
    case COMMA_SYMBOL:
    case FUNCTION_CALL_TOKEN:
    case SELECT_EXPR_TOKEN:
      // The parent type gives additional info here.
      switch (walker.parent_type())
      {
      case OPTIONS_SYMBOL: // Server options (create/alter server). Nothing to show.
        context.wanted_parts = Sql_editor::CompletionWantNothing;
        context.check_identifier = false;
        break;

      case ON_SYMBOL: // In column list of an index target table.
        context.wanted_parts = Sql_editor::CompletionWantColumns;
        context.check_identifier = false;
        break;

      case LOGFILE_GROUP_OPTIONS_TOKEN:
        context.wanted_parts = Sql_editor::CompletionWantKeywords;
        context.check_identifier = false;
        break;

      default:
        // By default assume we are dealing with expressions.
        want_only_functions_schemas_tables_columns(context);
        want_also_expression_start(context, false);
        
        // Some more checks for finer granularity.
        if (walker.token_type() == EQUAL_OPERATOR)
          if (!walker.previous_by_index())
            break;

        switch (walker.token_type())
        {
        case ENGINE_SYMBOL:
          context.wanted_parts = Sql_editor::CompletionWantEngines;
          context.check_identifier = false;
          break;
        }
      }
      break;

    case TABLE_REF_ID_TOKEN:
      want_only_table_references(context);
      context.check_identifier = true;
      break;

    case FIELD_REF_ID_TOKEN:
      // At the start of a reference. This can also mean we are in an expression or an assignment list.
      if (walker.parent_type() == COLUMN_ASSIGNMENT_LIST_TOKEN)
      {
        // Often we need just a column reference (e.g. on the left hand side of an assignment).
        // So start with this.
        want_only_field_references(context);
        context.check_identifier = true;
        if (!walker.previous_sibling() || walker.token_type() == COMMA_SYMBOL)
          return; // At the begin of the list or a new assignment.

        want_also_expression_start(context, false);
      }

      // For the next check we move forward in tree order and back again in index order to get the token
      // that is "physically" before the current one.
      walker.next();

      context.wanted_parts = Sql_editor::CompletionWantNothing;
      context.check_identifier = true;
      if (walker.previous_by_index())
      {
        if (walker.is_operator())
          want_also_expression_start(context, walker.token_type() == OPEN_PAR_SYMBOL);
        else
        {
          switch (walker.token_type())
          {
          case SET_SYMBOL: // In an update list. We only need columns.
            context.wanted_parts = Sql_editor::CompletionWantColumns;
            context.check_identifier = false;
            break;

          default:
            want_only_functions_schemas_tables_columns(context);
            want_also_expression_start(context, false);
            context.check_identifier = false;
            break;
          }
        }
      }
      break;

    case GROUP_SYMBOL:
    case ORDER_SYMBOL:
    case IDENTIFIED_SYMBOL:
      context.wanted_parts = Sql_editor::CompletionWantBy;
      context.check_identifier = false;
      break;

    case DATABASE_SYMBOL:
      context.wanted_parts = Sql_editor::CompletionWantSchemas;
      context.check_identifier = false;
      break;

    case COLUMN_SYMBOL: // Alter table items.
      context.wanted_parts = Sql_editor::CompletionWantColumns;
      context.check_identifier = false;
      break;

    case DO_SYMBOL: // Starting compound statement in an event.
      context.wanted_parts = Sql_editor::CompletionWantMajorKeywords;
      context.check_identifier = false;
      break;

    case AS_SYMBOL: // AS outside a field ref is used for view definitions.
      context.wanted_parts = Sql_editor::CompletionWantSelect;
      context.check_identifier = false;
      break;

    case ON_SYMBOL: // CREATE TRIGGER ... ON
      context.wanted_parts = Sql_editor::CompletionWantTables;
      context.check_identifier = false;
      break;

    case EXISTS_SYMBOL: // After an "if exists".
    case FUNCTION_SYMBOL:
    case PROCEDURE_SYMBOL:
    case EVENT_SYMBOL:
    case TABLE_SYMBOL:
    case TABLES_SYMBOL:
    case TRIGGER_SYMBOL:
    case VIEW_SYMBOL:
    case INDEX_SYMBOL:
      context.check_identifier = false;
        if (!check_by_query_type(walker, context))
          context.wanted_parts = Sql_editor::CompletionWantKeywords;
        break;

    case FOR_SYMBOL:
      context.check_identifier = false;
      if (walker.get_current_query_type() == QtSetPassword)
        context.wanted_parts = Sql_editor::CompletionWantUsers;
      else
        context.wanted_parts = Sql_editor::CompletionWantKeywords;

      break;

    default:
      context.wanted_parts = Sql_editor::CompletionWantKeywords;
      context.check_identifier = false;
      break;
    }
  }
  else
  {
    if (walker.is_number())
    {
      // If the token is a number we are in an expression and have nothing to offer for completion.
      context.wanted_parts = Sql_editor::CompletionWantNothing;
      context.check_identifier = false;
      return;
    }

    if (walker.token_type() == CLOSE_PAR_SYMBOL)
      // Finishing a par expression or function call. Could be part of an expression (we don't show operators),
      // an alias could follow (nothing to show) or the next query part comes next, so we show keywords.
      context.wanted_parts = Sql_editor::CompletionWantKeywords;
    else
    {
      // Check if we are in a subtree.
      if (walker.up())
      {
        switch (walker.token_type())
        {
        case FUNCTION_CALL_TOKEN:
          want_only_functions_schemas_tables_columns(context);
          break;

        case TABLE_REF_ID_TOKEN:
          want_only_table_references(context);
          context.check_identifier = true;
          break;

        case FIELD_REF_ID_TOKEN:
          context.wanted_parts = Sql_editor::CompletionWantNothing;
          context.check_identifier = true;
          break;

        default:
          context.wanted_parts = Sql_editor::CompletionWantKeywords;
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Check for common cases.
 * Important: when the caret position is equal to a char position it is displayed as being
 *            in front of that character. This has consequences which token to consider, especially
 *            at the start of a token.
 */
void check_general_context(Sql_editor::AutoCompletionContext &context, MySQLRecognizerTreeWalker &walker)
{
  log_debug2("Checking some general situations\n");

  // Three cases here:
  //   1) Directly at the start of the token, i.e. the caret is visually before the first token char.
  //      Handled like case 3 but for the token before this one.
  //   2) Within the token. This includes the position directly after the last token char.
  //      Find options for this very token position. If however this is a token where it doesn't matter
  //      if there's a whitespace or not after it (e.g. operators) then handle it like case 3.
  //   3) In the whitespaces after a token. Offer all possible options for the next position.

  // Case 1.
  if (context.line == context.token_line && context.offset == context.token_start)
  {
    // First check if the previous token is a virtual token. If so use this instead of the
    // one that is physically located before the current one.
    unsigned int previous_type = walker.previous_type();
    walker.push();
    if (!walker.previous())
    {
      walker.pop();
      context.check_identifier = false;
      return; // If there's no previous token then we act as we do when starting a new statement.
    }

    bool check_parent_type = false;
    switch (walker.token_type())
    {
    case TABLE_REF_ID_TOKEN:
    case FIELD_REF_ID_TOKEN:
      walker.remove_tos();
      check_parent_type = true;
      break;
    default:
      walker.pop();
      if (!walker.previous_by_index())
      {
        context.check_identifier = false;
        return; // If there's no previous token then we act as we do when starting a new statement.
      }
    }

    // Special case: if the previous token is a relation we know exactly what we need.
    // Include the parent type in this check as field/table refs could be in between.
    if (walker.is_relation() || (check_parent_type && walker.recognizer()->is_relation(walker.parent_type())))
    {
      want_only_field_references(context);
      want_also_expression_start(context, previous_type == OPEN_PAR_SYMBOL);
      context.check_identifier = false;
      return;
    }
  }

  walker.push();

  // Case 3.
  {
    // For expressions like "(id" it does not matter if there's a space or not if the caret is between
    // "(" and "id". In this case we want to check a new token start.
    // For expressions like "a.b" with the caret between "a" and "." it matters however. In this case
    // we want to continue finding a completion for the previous token (the "a").
    if (walker.is_operator() || context.line > walker.token_line() ||
      context.offset > walker.token_start() + walker.token_length())
      check_new_token_start(walker, context);
    else
    {
      // Case 2.
      check_current_token(walker, context);
    }
  }

  walker.pop();
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses a schema/table/column id, collecting all specified values. The current location
 * within the id is used to determine what to show.
 */
void check_reference(Sql_editor::AutoCompletionContext &context, MySQLRecognizerTreeWalker &walker)
{
  log_debug2("Checking table references\n");

  EXCLUDE_PART(Sql_editor::CompletionWantMajorKeywords);

  bool in_table_ref = false;
  bool in_field_ref = false;
  
  // Walk the parent chain to see if we are in a table reference, but do not go higher than
  // the current (sub) statement (to avoid wrong info when we are in a sub select).
  bool done = walker.token_type() == TABLE_REF_ID_TOKEN;
  if (done)
  {
    // We arrive here if the walker was moved one token backwards from a real token
    // (we can never be at a virtual token at the start).
    in_table_ref = true;
    EXCLUDE_PART(Sql_editor::CompletionWantRuntimeFunctions);

    // This could be wrong if the reference is actually complete.
    // We do another check below.
    EXCLUDE_PART(Sql_editor::CompletionWantKeywords);
  }

  walker.push();

  while (!done)
  {
    if (!walker.up())
      break;
    
    unsigned int type = walker.token_type();
    switch (type)
    {
      case TABLE_REF_ID_TOKEN:
        EXCLUDE_PART(Sql_editor::CompletionWantRuntimeFunctions);
        EXCLUDE_PART(Sql_editor::CompletionWantKeywords);
        in_table_ref = true;
        done = true;
        break;

      case SUBQUERY_TOKEN:
      //case EXPRESSION:
      //case JOIN_EXPR:
      //case OPEN_PAR_SYMBOL:
        done = true;
        break;
    }
  }
  walker.pop();
  
  walker.push();

  std::string id2, id1, id0;
  enum {inPos2, inPos1, inPos0} caret_position = inPos0;

  // Collect the 3 possible identifier parts and determine where the caret is.
  // First advance to the rightmost part. The star is per definition the rightmost part.
  while (true)
  {
    if (walker.token_type() == MULT_OPERATOR)
      break;

    unsigned int next_token = walker.look_ahead(false);
    if (walker.is_identifier() && next_token != DOT_SYMBOL)
      break;

    if (walker.token_type() == DOT_SYMBOL && !walker.recognizer()->is_identifier(next_token))
      break;

    if (!walker.next_sibling())
      break;
  }

  // Initially we assume the caret is at position 0 (the right most one).
  bool has_more = true;
  if (walker.is_identifier() || walker.token_type() == MULT_OPERATOR)
  {
    id0 = walker.token_text(); // Unquoting/concatenating/processing is done in the lexer.
    has_more = walker.previous_sibling();
  }

  if (has_more && (walker.token_type() == DOT_SYMBOL))
  {
    id1 = '.'; // Identifiers can just be leading dot + the name (e.g. select * from .city).
    has_more = walker.previous_sibling();
  }
  
  // Second id, if there's one. At this point we cannot have a missing ID or a wildcard.
  if (has_more && walker.is_identifier())
  {
    unsigned int token_start = walker.token_start();
    unsigned int token_end = walker.token_start() + walker.token_length();

    id1 = walker.token_text();

    // See if the caret is within the range of this id.
    if (token_start <= context.offset && context.offset <= token_end)
      caret_position = inPos1;

    has_more = walker.previous_sibling();
    if (has_more && walker.token_type() == DOT_SYMBOL)
    {
      id2 = '.';
      if (walker.previous_sibling())
      {
        // Finally the third id.
        token_start = walker.token_start();
        token_end = walker.token_start() + walker.token_length();
        if (walker.is_identifier())
          id2 = walker.token_text();

        if (token_start <= context.offset && context.offset <= token_end)
          caret_position = inPos2;
      }
    }
  }

  // Given the id parts and where we are in the id we can now conclude what we want.
  if (caret_position == inPos2
      || (id2.empty() && caret_position == inPos1)
      || (id1.empty() && id2.empty())
    )
  {
    INCLUDE_PART(Sql_editor::CompletionWantSchemas);
  }
  else
  {
    EXCLUDE_PART(Sql_editor::CompletionWantSchemas);
  }

  if (caret_position == inPos1
    || (id2.empty() && !id1.empty() && caret_position == inPos0)
    || (id2.empty() && id1.empty()))
  {
    INCLUDE_PART(Sql_editor::CompletionWantTables);
    if (caret_position == inPos1)
      context.table_schema = id2; // Could be empty in which case we use the default schema.
    else
      context.table_schema = id1;
    context.table = id1;
  }
  else
  {
    EXCLUDE_PART(Sql_editor::CompletionWantTables);
  }

  if (!in_table_ref)
  {
    if (caret_position == inPos0)
    {
      INCLUDE_PART(Sql_editor::CompletionWantColumns);
      context.column_schema = id2;
      context.table = id1;
      context.column = (id0 == "*") ? "" : id0;
    }
  }
  else
  {
    EXCLUDE_PART(Sql_editor::CompletionWantColumns);
  }

  // If we are in a table ref or an identifier with more than one part we know showing expression
  // start keywords and function names are meaningless. Hence take them out.
  if (in_table_ref || !id2.empty() || !id1.empty())
  {
    EXCLUDE_PART(Sql_editor::CompletionWantRuntimeFunctions);
    EXCLUDE_PART(Sql_editor::CompletionWantExprStartKeywords);
    EXCLUDE_PART(Sql_editor::CompletionWantSelect);
  }

  // If the caret is in a part that has another part in front of it (so it is a qualified part)
  // then mark this to avoid including non-qualified parts.
  if ((caret_position == inPos0 && (!id1.empty() || !id2.empty())) ||
    (caret_position == inPos1 && !id2.empty()))
    context.qualified_identifier = true;

  walker.pop();
}
  
//--------------------------------------------------------------------------------------------------

/**
 * Reads a single TABLE_REF_ID_TOKEN subtree and checks for a following alias.
 */
void read_table_ref_id(Sql_editor::AutoCompletionContext &context, MySQLRecognizerTreeWalker &walker)
{
  walker.next();
  
  std::string schema;
  std::string table = walker.token_text();
  std::string alias;

  bool has_more = walker.next_sibling();
  if (has_more && walker.token_type() == DOT_SYMBOL)
  {
    has_more = walker.next_sibling();
    if (has_more && walker.is_identifier())
    {
      schema = table;
      table = walker.token_text();
    }
  }

  // Continue with the next token after the table ref subtree.
  has_more = walker.next();

  if (has_more && walker.token_type() == AS_SYMBOL)
    has_more = walker.next_sibling();

  if (has_more && walker.is_identifier())
    alias = walker.token_text();
  
  if (!table.empty())
  {
    Sql_editor::TableReference reference = {schema, table, alias};
    context.references.push_back(reference);
  }
}

//--------------------------------------------------------------------------------------------------

void scan_sub_tree(Sql_editor::AutoCompletionContext &context, MySQLRecognizerTreeWalker &walker)
{
  bool has_more = walker.next(); // Go to the first child node.

  while (has_more)
  {
    walker.push();
    if (walker.token_type() == TABLE_REF_ID_TOKEN)
      read_table_ref_id(context, walker);
    else
    {
      if (walker.is_subtree() && walker.token_type() != SUBQUERY_TOKEN)
        scan_sub_tree(context, walker);
    }
    walker.pop();
    has_more = walker.next_sibling();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Collects all table references (from, update etc.) in the current query.
 * The tree walker must be positioned at the caret location already.
 */
void collect_table_references(Sql_editor::AutoCompletionContext &context, MySQLRecognizerTreeWalker &walker)
{
  // Step up the tree to our owning select, update ... query.
  bool done = false;
  while (!done)
  {
    if (!walker.up() || walker.is_nil())
      break;

    switch (walker.token_type())
    {
    case SUBQUERY_TOKEN:
    case REFERENCES_SYMBOL:
      done = true;
      break;

    case TABLE_REF_ID_TOKEN:
      context.in_table_reference = true;
      break;
    }
  }

  // There's a dedicated token type for table reference identifiers, so simply scan this in the
  // current query but don't follow sub queries.
  scan_sub_tree(context, walker);
}

//--------------------------------------------------------------------------------------------------

/**
 * A separate routine to actually collect all auto completion entries. Works with a passed in context
 * to allow unit tests checking the result.
 * 
 * Returns false if there was a syntax error, otherwise true.
 */
bool Sql_editor::create_auto_completion_list(AutoCompletionContext &context)
{
  log_debug("Creating new code completion list\n");

  std::set<std::pair<int, std::string>, CompareAcEntries> new_entries;
  _auto_completion_entries.clear();
  context.version = 50501; //_server_version; TODO: switch to parser services

  bool found_errors = false;
  if (!context.statement.empty())
  {
    std::set<std::string> charsets; // TODO: switch to parser services.
    MySQLRecognizer recognizer(50501 /* _server_version*/, _sql_mode, charsets);
    recognizer.parse(context.statement.c_str(), context.statement.length(), true, QtUnknown);
    MySQLRecognizerTreeWalker walker = recognizer.tree_walker();

    //log_debug3("Parse tree:\n%s\n", recognizer.dump_tree().c_str());

    found_errors = recognizer.has_errors();

    /*
    for (std::vector<MySQLParserErrorInfo>::const_iterator iter = recognizer.error_info().begin(); iter != recognizer.error_info().end(); iter++)
      log_debug3("===[PARSE TREE ERROR]===\nQuery: %s\nError [%d]: %s\n\tLine: %d\n\tOffset: %d\n\tLength: %d\n", 
        context.statement.c_str(), iter->error, iter->message.c_str(), iter->line, iter->offset, iter->length);
        */
    bool found_token = walker.advance_to_position((int)context.line, (int)context.offset);

    if (!found_token)
    {
      // No useful parse info found so we show at least show keywords.
      context.wanted_parts = Sql_editor::CompletionWantKeywords;

      // See if we can get a better result by examining the errors.
      if (recognizer.error_info().size() > 0)
        check_error_context(context, recognizer);
    }
    else
    {
      get_current_token_info(context, walker);

      // If we are currently in a string then we don't show any auto completion.
      if ((context.token_type == SINGLE_QUOTED_TEXT) ||
        ((recognizer.sql_mode() & SQL_MODE_ANSI_QUOTES) == 0) && (context.token_type == DOUBLE_QUOTED_TEXT))
      {
        context.wanted_parts = CompletionWantNothing;
        return !recognizer.has_errors();
      }

      // If there's a syntax error with a token between the one we found in advance_to_position and
      // before the current caret position then we switch to the last error token to take this into account.
      if (recognizer.error_info().size() > 0)
      {
        MySQLParserErrorInfo error = recognizer.error_info().back();
        if ((context.token_line < error.line || context.token_line == error.line && context.token_start < error.charOffset) &&
          (error.line < context.line|| error.line == context.line && error.charOffset < context.offset))
        {
          context.token_type = error.token_type;
          context.token_line = error.line;
          context.token_start = error.charOffset;
          context.token_length = error.length;
        }
      }

      // The walker is now at the token at the given position or in the white spaces following it.
      check_general_context(context, walker);

      // Identifiers are a bit more complex. We cannot check them however if there was an error and
      // we replaced the token as we could be at a totally different position.
      if (context.check_identifier)
      {
        if (recognizer.is_identifier(context.token_type)  || recognizer.is_keyword(context.token_type)
          || context.token_type == DOT_SYMBOL || context.token_type == MULT_OPERATOR)
        {
          // Found an id, dot or star. Can be either wildcard, schema, table or column. Check which it is.
          // We might be wrong with the star here if we are in an math expression, but that's good enough for now.
          // Helpful fact: we get here only if there is a valid qualified identifier of the form
          // id[.id[.(id|*)]]. Everything else (inluding something like id.id.id.id) is a syntax error.
          check_reference(context, walker);
        }
      }
    }

    if (IS_PART_INCLUDED(Sql_editor::CompletionWantTables) || IS_PART_INCLUDED(Sql_editor::CompletionWantColumns))
      collect_table_references(context, walker);
  }

  // Let descendants fill their keywords, functions and engines into the list.
  if (IS_PART_INCLUDED(CompletionWantAKeyword) ||
    IS_PART_INCLUDED(Sql_editor::CompletionWantRuntimeFunctions) ||
    IS_PART_INCLUDED(Sql_editor::CompletionWantEngines))
  {
    std::vector<std::pair<int, std::string> > rdbms_specific;
    fill_auto_completion_keywords(rdbms_specific, context.wanted_parts, make_keywords_uppercase());
    
    for (size_t i = 0; i < rdbms_specific.size(); i++)
      new_entries.insert(rdbms_specific[i]);
  }

  if (_auto_completion_cache != NULL)
  {
    if (context.table_schema.empty() || context.table_schema == ".")
      context.table_schema = _current_schema;
    if (context.column_schema.empty() || context.column_schema == ".")
      context.column_schema = _current_schema;
    
    // This is syntactically not correct. Column references cannot be made with a leading dot (like for tables).
    // But for more flexible handling we pretend it would be possible.
    if (context.table == ".")
      context.table = "";

    if (IS_PART_INCLUDED(Sql_editor::CompletionWantSchemas))
    {
      log_debug3("Adding schema names from cache\n");

      std::vector<std::string> object_names = _auto_completion_cache->get_matching_schema_names(context.typed_part);
      for (std::vector<std::string>::const_iterator iterator = object_names.begin(); iterator != object_names.end(); ++iterator)
        new_entries.insert(std::make_pair(AC_SCHEMA_IMAGE, *iterator));
    }
    
    if (IS_PART_INCLUDED(Sql_editor::CompletionWantTables))
    {
      log_debug3("Adding table names from cache\n");

      std::vector<std::string> object_names = _auto_completion_cache->get_matching_table_names(context.table_schema, context.typed_part);
      for (std::vector<std::string>::const_iterator iterator = object_names.begin(); iterator != object_names.end(); ++iterator)
        new_entries.insert(std::make_pair(AC_TABLE_IMAGE, *iterator));

      log_debug3("Adding table + alias names from reference list\n");

      // Add tables and aliases from the references list.
      // Aliases only if we are not in a table reference, however.
      // If we are in a qualified identifier then adding the tables doesn't make sense either.
      if (!context.qualified_identifier)
      {
        for (std::vector<Sql_editor::TableReference>::const_iterator iterator = context.references.begin();
          iterator != context.references.end(); ++iterator)
        {
          if (iterator->schema.empty() || base::same_string(iterator->schema, context.table_schema, true))
          {
            new_entries.insert(std::make_pair(AC_TABLE_IMAGE, iterator->table));
            if (!context.in_table_reference && !iterator->alias.empty())
              new_entries.insert(std::make_pair(AC_TABLE_IMAGE, iterator->alias));
          }
        }
      }
    }
    
    if (IS_PART_INCLUDED(Sql_editor::CompletionWantProcedures))
    {
      log_debug3("Adding procedure names from cache\n");

      std::vector<std::string> object_names = _auto_completion_cache->get_matching_procedure_names(context.table_schema, context.typed_part);
      for (std::vector<std::string>::const_iterator iterator = object_names.begin(); iterator != object_names.end(); ++iterator)
        new_entries.insert(std::make_pair(AC_ROUTINE_IMAGE, *iterator));
    }
    
    if (IS_PART_INCLUDED(Sql_editor::CompletionWantFunctions))
    {
      log_debug3("Adding function names from cache\n");

      std::vector<std::string> object_names = _auto_completion_cache->get_matching_function_names(context.table_schema, context.typed_part);
      for (std::vector<std::string>::const_iterator iterator = object_names.begin(); iterator != object_names.end(); ++iterator)
        new_entries.insert(std::make_pair(AC_ROUTINE_IMAGE, *iterator));
    }

    if (IS_PART_INCLUDED(Sql_editor::CompletionWantColumns))
    {
      log_debug3("Adding column names from cache\n");

      std::vector<std::string> object_names = _auto_completion_cache->get_matching_column_names(context.column_schema, context.table, context.typed_part);
      for (std::vector<std::string>::const_iterator iterator = object_names.begin(); iterator != object_names.end(); ++iterator)
        new_entries.insert(std::make_pair(AC_COLUMN_IMAGE, *iterator));
      
      // Additionally, check the references list if the given table name is an alias. If so take columns from
      // the aliased table too.
      for (std::vector<Sql_editor::TableReference>::const_iterator iterator = context.references.begin();
        iterator != context.references.end(); ++iterator)
      {
        if (base::same_string(iterator->alias, context.table, true) || base::same_string(iterator->table, context.table, true))
        {
          std::string schema = iterator->schema;
          if (schema.empty())
            schema = _current_schema;
          std::vector<std::string> object_names = _auto_completion_cache->get_matching_column_names(schema, iterator->table, context.typed_part);
          for (std::vector<std::string>::const_iterator iterator = object_names.begin(); iterator != object_names.end(); ++iterator)
            new_entries.insert(std::make_pair(AC_COLUMN_IMAGE, *iterator));
        }
      }
    }
  }

  // Copy sorted and unique entries to the actual list.
  std::copy(new_entries.begin(), new_entries.end(), std::back_inserter(_auto_completion_entries));

  return !found_errors;
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::show_auto_completion(bool auto_choose_single)
{
  // With the new splitter we can probably leave auto completion enabled even for large files.
  if (/*_have_large_content ||*/ !code_completion_enabled())
    return;

  log_debug("Invoking code completion\n");

  _code_editor->auto_completion_options(true, auto_choose_single, false, true, false);

  AutoCompletionContext context;

  // Get the statement and its absolute position.
  size_t caret_position = _code_editor->get_caret_pos();
  context.line = _code_editor->line_from_position(caret_position);
  ssize_t line_start, line_end;
  _code_editor->get_range_of_line(context.line, line_start, line_end);
  context.line++; // ANTLR parser is one-based.
  size_t offset = caret_position - line_start; // This is a byte offset.

  size_t min;
  size_t max;
  if (get_current_statement_range(min, max))
  {
    context.line -= _code_editor->line_from_position(min);
    context.statement += _code_editor->get_text_in_range(min, max);
    _last_ac_statement = context.statement;
  }
  else
    context.statement = _last_ac_statement;

  // Convert current caret position into a position of the single statement. ANTLR uses one-based line numbers.
  // The byte-based offset in the line must be converted to a character offset.
  std::string line_text = _code_editor->get_text_in_range(line_start, line_end);
  context.offset = g_utf8_pointer_to_offset(line_text.c_str(), line_text.c_str() + offset);

  // Determine the word letters written up to the current caret position. If the caret is in the white
  // space behind a token then nothing is typed.
  context.typed_part = get_written_part(caret_position);

  // Remove the escape character from the typed part so we have the pure text.
  context.typed_part.erase(std::remove(context.typed_part.begin(), context.typed_part.end(), '\\'),
    context.typed_part.end());

  create_auto_completion_list(context);
  update_auto_completion(context.typed_part);
}

//--------------------------------------------------------------------------------------------------

void Sql_editor::cancel_auto_completion()
{
  _code_editor->auto_completion_cancel();
}

//--------------------------------------------------------------------------------------------------

/**
 * The auto completion cache is connection dependent so it must be set by the owner of the editor
 * if there is a connection at all. Ownership of the cache remains with the owner of the editor.
 */
void Sql_editor::set_auto_completion_cache(AutoCompleteCache *cache)
{
  log_debug2("Auto completion cache set to: %p\n", cache);

  _auto_completion_cache = cache;
}

//--------------------------------------------------------------------------------------------------

bool Sql_editor::fill_auto_completion_keywords(std::vector<std::pair<int, std::string> > &entries,
  AutoCompletionWantedParts parts, bool upcase_keywords)
{
  log_debug("Request for filling the keyword auto completion list with no specialized editor.\n");

  return false;
}

//--------------------------------------------------------------------------------------------------

