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
#include "mysql-parser.h"
#include "mysql-syntax-check.h"

#include "objimpl/wrapper/parser_ContextReference_impl.h"

using namespace grt;
using namespace parser;

DEFAULT_LOG_DOMAIN("parser")

GRT_MODULE_ENTRY_POINT(MySQLParserServicesImpl);

//--------------------------------------------------------------------------------------------------

parser_ContextReferenceRef MySQLParserServicesImpl::createParserContext(const GrtCharacterSetsRef &charsets,
  const GrtVersionRef &version, const std::string &sql_mode, int case_sensitive)
{
  ParserContext::Ref context = MySQLParserServices::createParserContext(charsets, version, case_sensitive != 0);
  context->use_sql_mode(sql_mode);
  return parser_context_to_grt(version.get_grt(), context);
}

//--------------------------------------------------------------------------------------------------

/**
 * Signals any ongoing process to stop. This must be called from a different thread than from where
 * the processing was started to make it work.
 */
size_t MySQLParserServicesImpl::stopProcessing()
{
  _stop = true;
  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * If the current token is a definer clause collect the details and return it as string.
 */
std::string get_definer(MySQLRecognizerTreeWalker &walker)
{
  std::string definer;
  if (walker.token_type() == DEFINER_SYMBOL)
  {
    walker.next(2); // Skip DEFINER + equal sign.
    definer = walker.token_text();
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
  }

  return definer;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseTriggerSql(parser_ContextReferenceRef context_ref,
  const db_mysql_TriggerRef &trigger, const std::string &sql)
{
  ParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseTrigger(context, trigger, sql);
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given sql as trigger create script and fills all found details in the given trigger ref.
 * If there's an error nothing is changed.
 * Returns the number of errors.
 */
size_t MySQLParserServicesImpl::parseTrigger(const ParserContext::Ref &context,
  const db_mysql_TriggerRef &trigger, const std::string &sql)
{
  log_debug2("Parse trigger\n");

  trigger->sqlDefinition(sql);
  trigger->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, QtCreateTrigger);
  size_t error_count = context->recognizer()->error_info().size();
  int result_flag = 0;
  if (error_count == 0)
  {
    // There's no need for checks if any of the walker calls fail.
    //  If we arrive here the syntax must be correct.
    trigger->enabled(1);

    MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
    walker.next(); // Skip CREATE.

    trigger->definer(get_definer(walker));

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

size_t MySQLParserServicesImpl::parseViewSql(parser_ContextReferenceRef context_ref,
  const db_mysql_ViewRef &view, const std::string &sql)
{
  ParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseView(context, view, sql);
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given sql as a create view script and fills all found details in the given view ref.
 * If there's an error nothing changes. If the sql contains a schema reference other than that the
 * the view is in the view's name will be changed (adds _WRONG_SCHEMA) to indicate that.
 */
size_t MySQLParserServicesImpl::parseView(const ParserContext::Ref &context,
  const db_mysql_ViewRef &view, const std::string &sql)
{
  log_debug2("Parse view\n");

  view->sqlDefinition(sql);
  view->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, QtCreateView);
  size_t error_count = context->recognizer()->error_info().size();
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

    view->definer(get_definer(walker));

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

    walker.next(2); // AS + SELECT subtree.
    if (walker.token_type() == WITH_SYMBOL)
      view->withCheckCondition(1);
    else
      view->withCheckCondition(0); // WITH_SYMBOL (CASCADED_SYMBOL | LOCAL_SYMBOL)? CHECK_SYMBOL OPTION_SYMBOL

    view->modelOnly(0);
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

void fill_routine_details(MySQLRecognizerTreeWalker &walker, db_mysql_RoutineRef routine)
{
  walker.next(); // Skip CREATE.
  routine->definer(get_definer(walker));

  // A UDF is also a function and will be handled as such here.
  walker.skip_if(AGGREGATE_SYMBOL);
  bool is_function = false;
  if (walker.token_type() == FUNCTION_SYMBOL)
    is_function = true;
  if (is_function)
    routine->routineType("function");
  else
    routine->routineType("procedure");

  walker.next(2); // skip FUNCTION/PROCEDURE + *_NAME_TOKEN
  std::string name = walker.token_text();
  walker.next();
  if (walker.token_type() == DOT_SYMBOL)
  {
    // A qualified name. Check schema part.
    walker.next();
    db_SchemaRef schema = db_SchemaRef::cast_from(routine->owner());
    if (!base::same_string(schema->name(), name, false)) // Routine names are never case sensitive.
      name = walker.token_text() + "_WRONG_SCHEMA";
    else
      name = walker.token_text();

    walker.next();
  }
  routine->name(name);

  if (walker.token_type() == RETURNS_SYMBOL)
  {
    // UDF.
    routine->routineType("udf");
    walker.next();
    routine->returnDatatype(walker.token_text());
  }
  else
  {
    // Parameters.
    ListRef<db_mysql_RoutineParam> params = routine->params();
    params.remove_all();
    walker.next(); // Open par.

    while (true)
    {
      db_mysql_RoutineParamRef param(routine->get_grt());
      param->owner(routine);

      switch (walker.token_type())
      {
        case IN_SYMBOL:
        case OUT_SYMBOL:
        case INOUT_SYMBOL:
          param->paramType(base::tolower(walker.token_text()));
          walker.next();
          break;
      }

      param->name(walker.token_text());
      walker.next();

      // DATA_TYPE_TOKEN.
      param->datatype(walker.text_for_tree());
      params.insert(param);

      walker.next_sibling();
      if (walker.token_type() != COMMA_SYMBOL)
        break;
      walker.next();
    }
    walker.next(); // Closing par.

    if (walker.token_type() == RETURNS_SYMBOL)
    {
      walker.next();

      // DATA_TYPE_TOKEN.
      routine->returnDatatype(walker.text_for_tree());
      walker.next();
    }

    if (walker.token_type() == ROUTINE_CREATE_OPTIONS)
    {
      // For now we only store comments and security settings.
      walker.next();
      do
      {
        switch (walker.token_type())
        {
          case SQL_SYMBOL:
            walker.next(2); // SQL + SECURITY (both are siblings)
            routine->security(walker.token_text());
            break;

          case COMMENT_SYMBOL:
            walker.next();
            routine->comment(walker.token_text());
            break;
        }
      } while (walker.next_sibling());
    }
  }

  routine->modelOnly(0);
}

//--------------------------------------------------------------------------------------------------

std::string read_routine_name_nfqn(MySQLRecognizerTreeWalker &walker)
{
  // On enter the walker must be on a *_NAME_TOKEN subtree.
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
  return name;
}

//--------------------------------------------------------------------------------------------------

/**
 * Tries to find the name of a routine from the parse tree by examining the possible subtrees.
 * Returns a tuple with name and the found routine type. Both can be empty.
 */
std::pair<std::string, std::string> get_routine_name_and_type(MySQLRecognizerTreeWalker &walker)
{
  std::pair<std::string, std::string> result;

  if (walker.advance_to_type(PROCEDURE_NAME_TOKEN, true))
  {
    result.second = "procedure";
    result.first = read_routine_name_nfqn(walker);
  }
  else
  {
    walker.reset();
    if (walker.advance_to_type(FUNCTION_NAME_TOKEN, true))
    {
      result.second = "function";
      result.first = read_routine_name_nfqn(walker);
    }
    else
    {
      walker.reset();
      if (walker.advance_to_type(UDF_NAME_TOKEN, true))
      {
        result.second = "udf";
        result.first = read_routine_name_nfqn(walker);
      }
    }
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseRoutineSql(parser_ContextReferenceRef context_ref,
  const db_mysql_RoutineRef &routine, const std::string &sql)
{
  ParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseRoutine(context, routine, sql);
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given sql as a create function/procedure script and fills all found details in the given routine ref.
 * If there's an error nothing changes. If the sql contains a schema reference other than that the
 * the routine is in the routine's name will be changed (adds _WRONG_SCHEMA) to indicate that.
 */
size_t MySQLParserServicesImpl::parseRoutine(const ParserContext::Ref &context,
  const db_mysql_RoutineRef &routine, const std::string &sql)
{
  log_debug2("Parse routine\n");

  routine->sqlDefinition(sql);
  routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, QtCreateRoutine);
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  size_t error_count = context->recognizer()->error_info().size();
  if (error_count == 0)
    fill_routine_details(walker, routine);
  else
  {
    // Finished with errors. See if we can get at least the routine name out.
    std::pair<std::string, std::string> values = get_routine_name_and_type(walker);
    routine->name(values.first + "_SYNTAX_ERROR");
    routine->routineType(values.second);

    routine->modelOnly(1);
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

bool consider_as_same_type(std::string type1, std::string type2)
{
  if (type1 == type2)
    return true;

  if (type1 == "function" && type2 == "udf")
    return true;

  if (type2 == "function" && type1 == "udf")
    return true;

  return false;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseRoutinesSql(parser_ContextReferenceRef context_ref,
  const db_mysql_RoutineGroupRef &group, const std::string &sql)
{
  ParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseRoutines(context, group, sql);
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given sql as a list of create function/procedure statements.
 * In case of an error handling depends on the error position. We try to get most of the routines out
 * of the script.
 *
 * This process has two parts attached:
 *   - Update the sql text + properties for any routine that is in the script in the owning schema.
 *   - Update the list of routines in the given routine group to what is in the script.
 */
size_t MySQLParserServicesImpl::parseRoutines(const ParserContext::Ref &context,
  const db_mysql_RoutineGroupRef &group, const std::string &sql)
{
  log_debug2("Parse routine group\n");

  size_t error_count = 0;

  std::vector<std::pair<size_t, size_t> > ranges;
  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges, "\n");

  grt::ListRef<db_Routine> routines = group->routines();
  routines.remove_all();

  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(group->owner());
  grt::ListRef<db_Routine> schema_routines = schema->routines();

  int sequence_number = 0;
  int syntax_error_counter = 1;
  for (std::vector<std::pair<size_t, size_t> >::iterator iterator = ranges.begin(); iterator != ranges.end(); ++iterator)
  {
    std::string routine_sql = sql.substr(iterator->first, iterator->second);
    context->recognizer()->parse(routine_sql.c_str(), routine_sql.length(), true, QtCreateRoutine);
    size_t local_error_count = context->recognizer()->error_info().size();
    error_count += local_error_count;

    // Before filling a routine we need to know if there's already one with that name in the schema.
    // Hence we first extract the name and act based on that.
    MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
    std::pair<std::string, std::string> values = get_routine_name_and_type(walker);

    // If there's no usable info from parsing preserve at least the code and generate a
    // name for the routine using a counter.
    if (values.first.empty())
    {
      // Create a new routine instance.
      db_mysql_RoutineRef routine = db_mysql_RoutineRef(group->get_grt());
      routine->createDate(base::fmttime(0, DATETIME_FMT));
      routine->owner(schema);
      schema_routines.insert(routine);

      routine->name(*group->name() + "_SYNTAX_ERROR_" + base::to_string(syntax_error_counter++));
      routine->routineType("unknown");
      routine->modelOnly(1);
      routine->sqlDefinition(routine_sql);
      routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

      routines.insert(routine);
  }
    else
    {
      db_mysql_RoutineRef routine;
      for (size_t i = 0; i < schema_routines.count(); ++i)
      {
        // Stored functions and UDFs share the same namespace.
        // Stored routine names are not case sensitive.
        db_RoutineRef candidate = schema_routines[i];
        std::string name = candidate->name();

        // Remove automatically added appendixes before comparing names.
        if (base::ends_with(name, "_WRONG_SCHEMA"))
          name.resize(name.size() - 13);
        if (base::ends_with(name, "_SYNTAX_ERROR"))
          name.resize(name.size() - 13);

        if (base::same_string(values.first, name, false) && consider_as_same_type(values.second, candidate->routineType()))
        {
          routine = db_mysql_RoutineRef::cast_from(candidate);
          break;
        }
      }

      walker.reset();
      if (!routine.is_valid())
      {
        // Create a new routine instance.
        routine = db_mysql_RoutineRef(group->get_grt());
        routine->createDate(base::fmttime(0, DATETIME_FMT));
        routine->owner(schema);
        schema_routines.insert(routine);
      }

      if (local_error_count == 0)
        fill_routine_details(walker, routine);
      else
      {
        routine->name(values.first + "_SYNTAX_ERROR");
        routine->routineType(values.second);

        routine->modelOnly(1);
      }

      routine->sqlDefinition(routine_sql);
      routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

      // Finally add the routine to the group if it isn't already there.
      bool found = false;
      for (size_t i = 0; i < routines.count(); ++i)
      {
        if (base::same_string(routine->name(), routines[i]->name(), false))
        {
          found = true;
          break;
        }
      }
      if (!found)
        routines.insert(routine);
    }

  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::doSyntaxCheck(parser_ContextReferenceRef context_ref,
  const std::string &sql, const std::string &type)
{
  ParserContext::Ref context = parser_context_from_grt(context_ref);
  MySQLQueryType query_type = QtUnknown;
  if (type == "view")
    query_type = QtCreateView;
  else
    if (type == "routine")
      query_type = QtCreateRoutine;
    else
      if (type == "trigger")
        query_type = QtCreateTrigger;
      else
        if (type == "event")
          query_type = QtCreateEvent;

  return checkSqlSyntax(context, sql.c_str(), sql.size(), query_type);
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given text as a specific query type (see parser for supported types).
 * Returns the error count.
 */
size_t MySQLParserServicesImpl::checkSqlSyntax(const ParserContext::Ref &context, const char *sql,
  size_t length, MySQLQueryType type)
{
  context->syntax_checker()->parse(sql, length, true, type);
  return context->syntax_checker()->error_info().size();
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper to collect text positions to references of the given schema.
 * We only come here if there was no syntax error.
 */
void collect_schema_name_offsets(ParserContext::Ref context, std::list<size_t> &offsets, const std::string schema_name)
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
        {
          size_t pos = walker.token_offset();
          if (walker.token_type() == BACK_TICK_QUOTED_ID || walker.token_type() == SINGLE_QUOTED_TEXT)
            ++pos;
          offsets.push_back(pos);
        }
        break;

      case TABLE_NAME_TOKEN:
      {
        walker.next();
        if (walker.token_type() != DOT_SYMBOL && walker.look_ahead(false) == DOT_SYMBOL)
        {
           // A table ref not with leading dot but a qualified identifier.
          if (base::same_string(walker.token_text(), schema_name, case_sensitive))
          {
            size_t pos = walker.token_offset();
            if (walker.token_type() == BACK_TICK_QUOTED_ID || walker.token_type() == SINGLE_QUOTED_TEXT)
              ++pos;
            offsets.push_back(pos);
          }
        }
        break;
      }

      case FIELD_NAME_TOKEN: // Field names only if they are fully qualified (schema.table.field/*).
        walker.next();
        if (walker.token_type() != DOT_SYMBOL && walker.look_ahead(false) == DOT_SYMBOL)
        {
          // A leading dot means no schema.
          std::string name = walker.token_text();
          size_t pos = walker.token_offset();
          walker.next(2);
          if (walker.look_ahead(false) == DOT_SYMBOL) // Fully qualified.
          {
            if (base::same_string(name, schema_name, case_sensitive))
            {
              size_t pos = walker.token_offset();
              if (walker.token_type() == BACK_TICK_QUOTED_ID || walker.token_type() == SINGLE_QUOTED_TEXT)
                ++pos;
              offsets.push_back(pos);
            }
          }
        }
        break;

        // All those can have schema.id or only id.
      case VIEW_NAME_TOKEN:
      case TRIGGER_NAME_TOKEN:
      case PROCEDURE_NAME_TOKEN:
      case FUNCTION_NAME_TOKEN:
        walker.next();
        if (walker.look_ahead(false) == DOT_SYMBOL)
        {
          if (base::same_string(walker.token_text(), schema_name, case_sensitive))
          {
            size_t pos = walker.token_offset();
            if (walker.token_type() == BACK_TICK_QUOTED_ID || walker.token_type() == SINGLE_QUOTED_TEXT)
              ++pos;
            offsets.push_back(pos);
          }
        }
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Replace all occurrences of the old by the new name according to the offsets list.
 */
void replace_schema_names(std::string &sql, const std::list<size_t> &offsets, size_t length,
                          const std::string new_name)
{
  bool remove_schema = new_name.empty();
  for (std::list<size_t>::const_reverse_iterator iterator = offsets.rbegin(); iterator != offsets.rend(); ++iterator)
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

void rename_in_list(grt::ListRef<db_DatabaseDdlObject> list, const ParserContext::Ref &context,
  MySQLQueryType type, const std::string old_name, const std::string new_name)
{
  for (size_t i = 0; i < list.count(); ++i)
  {
    std::string sql = list[i]->sqlDefinition();
    context->recognizer()->parse(sql.c_str(), sql.size(), true, type);
    size_t error_count = context->recognizer()->error_info().size();
    if (error_count == 0)
    {
      MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();

      std::list<size_t> offsets;
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

size_t MySQLParserServicesImpl::doSchemaRefRename(parser_ContextReferenceRef context_ref,
  const db_mysql_CatalogRef &catalog, const std::string old_name, const std::string new_name)
{
  ParserContext::Ref context = parser_context_from_grt(context_ref);
  return renameSchemaReferences(context, catalog, old_name, new_name);
}

//--------------------------------------------------------------------------------------------------

/**
 * Goes through all schemas in the catalog and changes all db objects to refer to the new name if they
 * currently refer to the old name. We also iterate non-related schemas in order to have some
 * consolidation/sanitizing in effect where wrong schema references were used.
 */
size_t MySQLParserServicesImpl::renameSchemaReferences(const ParserContext::Ref &context,
  const db_mysql_CatalogRef &catalog, const std::string old_name, const std::string new_name)
{
  log_debug("Rename schema references\n");

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

grt::BaseListRef MySQLParserServicesImpl::getSqlStatementRanges(const std::string &sql)
{
  grt::BaseListRef list(get_grt());
  std::vector<std::pair<size_t, size_t> > ranges;

  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);

  for (std::vector<std::pair<size_t, size_t> >::const_iterator i = ranges.begin(); i != ranges.end(); ++i)
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
 * A statement splitter to take a list of sql statements and split them into individual statements,
 * return their position and length in the original string (instead the copied strings).
 */
size_t MySQLParserServicesImpl::determineStatementRanges(const char *sql, size_t length,
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
