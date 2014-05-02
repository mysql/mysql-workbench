/* 
 * Copyright (c) 2010, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"
#include "base/string_utilities.h"

#include "mysql_sql_editor_be.h"

#include "mforms/code_editor.h"

#include "grts/structs.db.mysql.h"

using namespace bec;
using namespace grt;

DEFAULT_LOG_DOMAIN("MySQL editor")

//--------------------------------------------------------------------------------------------------

Mysql_sql_editor::Mysql_sql_editor(db_mgmt_RdbmsRef rdbms, GrtVersionRef version)
  : Sql_editor(rdbms, version)
{
  mforms::SyntaxHighlighterLanguage lang = mforms::LanguageMySQL;
  
  if (version.is_valid() && *version->majorNumber() == 5)
  {
    switch (*version->minorNumber())
    {
      case 0: lang = mforms::LanguageMySQL50; break;
      case 1: lang = mforms::LanguageMySQL51; break;
      case 5: lang = mforms::LanguageMySQL55; break;
      case 6: lang = mforms::LanguageMySQL56; break;
    }
  }
  get_editor_control()->set_language(lang);
  _editor_config = new mforms::CodeEditorConfig(lang);
}

//--------------------------------------------------------------------------------------------------

Mysql_sql_editor::~Mysql_sql_editor()
{
  delete _editor_config;
}

//--------------------------------------------------------------------------------------------------

#define IS_PART_INCLUDED(part) \
  ((parts & part) == part)

bool Mysql_sql_editor::fill_auto_completion_keywords(std::vector<std::pair<int, std::string> > &entries,
  AutoCompletionWantedParts parts, bool upcase_keywords)
{
  log_debug2("Filling keywords auto completion list for MySQL.\n");

  if (_editor_config != NULL)
  {
    log_debug2("Adding keywords + function names\n");

    std::map<std::string, std::string> keyword_map = _editor_config->get_keywords();

    // MySQL keywords are split into two sets. Major keywords are those that can start a statement.
    // All other keywords appear within a statement.
    if (IS_PART_INCLUDED(Sql_editor::CompletionWantMajorKeywords))
    {
      std::vector<std::string> words = base::split_by_set(keyword_map["Major Keywords"], " \t\n");
      for (std::vector<std::string>::const_iterator iterator = words.begin(); iterator != words.end(); ++iterator)
        entries.push_back(std::make_pair(AC_KEYWORD_IMAGE, *iterator));
    }
    else
    {
      if (IS_PART_INCLUDED(Sql_editor::CompletionWantSelect))
        entries.push_back(std::make_pair(AC_KEYWORD_IMAGE, "select"));
      if (IS_PART_INCLUDED(Sql_editor::CompletionWantBy))
        entries.push_back(std::make_pair(AC_KEYWORD_IMAGE, "by"));
    }

    if (IS_PART_INCLUDED(Sql_editor::CompletionWantKeywords))
    {
      std::vector<std::string> words = base::split_by_set(keyword_map["Keywords"], " \t\n");
      for (std::vector<std::string>::const_iterator iterator = words.begin(); iterator != words.end(); ++iterator)
        entries.push_back(std::make_pair(AC_KEYWORD_IMAGE, *iterator));
      words = base::split_by_set(keyword_map["Procedure keywords"], " \t\n");
      for (std::vector<std::string>::const_iterator iterator = words.begin(); iterator != words.end(); ++iterator)
        entries.push_back(std::make_pair(AC_KEYWORD_IMAGE, *iterator));
      words = base::split_by_set(keyword_map["User Keywords 1"], " \t\n");
      for (std::vector<std::string>::const_iterator iterator = words.begin(); iterator != words.end(); ++iterator)
        entries.push_back(std::make_pair(AC_KEYWORD_IMAGE, *iterator));
    }
    else
    {
      // Expression keywords are a subset of the non-major keywords, so we only need to add them
      // if non-major keywords are not wanted.
      if (IS_PART_INCLUDED(Sql_editor::CompletionWantExprStartKeywords) ||
        IS_PART_INCLUDED(Sql_editor::CompletionWantExprInnerKeywords))
      {
        std::vector<std::string> words = base::split_by_set(keyword_map["User Keywords 2"], " \t\n");
        for (std::vector<std::string>::const_iterator iterator = words.begin(); iterator != words.end(); ++iterator)
          entries.push_back(std::pair<int, std::string>(AC_KEYWORD_IMAGE, *iterator));

        if (IS_PART_INCLUDED(Sql_editor::CompletionWantExprInnerKeywords))
        {
          std::vector<std::string> words = base::split_by_set(keyword_map["User Keywords 3"], " \t\n");
          for (std::vector<std::string>::const_iterator iterator = words.begin(); iterator != words.end(); ++iterator)
            entries.push_back(std::make_pair(AC_KEYWORD_IMAGE, *iterator));
        }
      }
    }

    if (upcase_keywords)
    {
      for (std::vector<std::pair<int, std::string> >::iterator iterator = entries.begin(); iterator != entries.end(); ++iterator)
        iterator->second = base::toupper(iterator->second);
    }

    if (IS_PART_INCLUDED(Sql_editor::CompletionWantRuntimeFunctions))
    {
      std::vector<std::string> words = base::split_by_set(keyword_map["Functions"], " \t\n");
      for (std::vector<std::string>::const_iterator iterator = words.begin(); iterator != words.end(); ++iterator)
        entries.push_back(std::make_pair(AC_FUNCTION_IMAGE, *iterator + "()"));
    }

    if (IS_PART_INCLUDED(Sql_editor::CompletionWantEngines))
    {
      grt::GRT *grt = grtm()->get_grt();
      grt::Module *module = grt->get_module("DbMySQL");
      if (module != NULL)
      {
        grt::BaseListRef args(grt);
        grt::ListRef<db_mysql_StorageEngine> engines =
          grt::ListRef<db_mysql_StorageEngine>::cast_from(module->call_function("getKnownEngines", args));

        if (engines.is_valid())
        {
          for (size_t c = engines.count(), i= 0; i < c; i++)
            entries.push_back(std::make_pair(AC_ENGINE_IMAGE, engines[i]->name()));
          }
      }
    }
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
