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

#pragma once

#if defined(_WIN32)
  #ifdef MYSQL_PARSER_EXPORTS
    #define MYSQL_PARSER_PUBLIC __declspec(dllexport)
  #else
    #define MYSQL_PARSER_PUBLIC __declspec(dllimport)
  #endif
#else
  #define MYSQL_PARSER_PUBLIC
#endif

#include "grtpp_module_cpp.h"
#include "grtsqlparser/mysql_parser_services.h"

#define DOC_MYSQLPARSERSERVICESIMPL \
  "Parsing services for MySQL.\n"\
  "\n"\
  "This module provides various functions for processing of MySQL sql code.\n"\
  "It is the second generation module using a fast ANTLR-based recognizer infrastructure."

//--------------------------------------------------------------------------------------------------

class MYSQL_PARSER_PUBLIC MySQLParserServicesImpl : public parser::MySQLParserServices, public grt::ModuleImplBase
{
public:
  MySQLParserServicesImpl(grt::CPPModuleLoader *loader) 
    : grt::ModuleImplBase(loader)
  {
  }
  DEFINE_INIT_MODULE_DOC("1.0", "Oracle Corporation", DOC_MYSQLPARSERSERVICESIMPL, grt::ModuleImplBase,
    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::stopProcessing,
    "Tells the module to stop any ongoing processing as soon as possible. Can be called from any thread.\n"
    "Calling any other module function will reset this flag, so make sure any running task returned"
    "before starting a new one.",
    ""),
/*
    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::parseTrigger,
    "Parses a trigger from the SQL script and applies it to the given view object.",
    "trigger an instantiated trigger object to fill\n"
    "sql the SQL script to be parsed"),
*/
    DECLARE_MODULE_FUNCTION_DOC(MySQLParserServicesImpl::getSqlStatementRanges,
    "Scans the sql code to find start and stop positions of each contained statement. An initial "
    "delimiter must be provided to find a statement's end. Embedded delimiter commands will be taken "
    "into account properly. The found ranges are returned as grt list.\n",
    "sql the sql script to process\n"),

    NULL);

  grt::BaseListRef getSqlStatementRanges(const std::string &sql);

  virtual int stopProcessing();

  virtual int parseTrigger(parser::ParserContext::Ref context, db_TriggerRef trigger, const std::string &sql);

  virtual int checkSqlSyntax(parser::ParserContext::Ref context, const char *sql, size_t length,
    MySQLQueryType type);

  virtual int determineStatementRanges(const char *sql, size_t length, const std::string &initial_delimiter,
    std::vector<std::pair<size_t, size_t> > &ranges, const std::string &line_break = "\n");

private:
  bool _stop;
};
