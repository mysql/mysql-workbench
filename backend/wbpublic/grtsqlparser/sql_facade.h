/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#ifndef _SQL_FACADE_H_
#define _SQL_FACADE_H_

#include <memory>
#include <set>
#include "wbpublic_public_interface.h"
#include "grtsqlparser/sql_parser.h"
#include "grtsqlparser/sql_syntax_check.h"
#include "grtsqlparser/sql_semantic_check.h"
#include "grtsqlparser/sql_specifics.h"
#include "grtsqlparser/sql_normalizer.h"
#include "grtsqlparser/sql_inserts_loader.h"
#include "grtsqlparser/invalid_sql_parser.h"
#include "grtsqlparser/sql_statement_decomposer.h"
#include "grtsqlparser/sql_schema_rename.h"
#include "grts/structs.db.mgmt.h"

/**
 * Serves as a single point of access to all functionality related to SQL processing.
 * Provides helper methods to perform most of the tasks.
 * Singleton.
 */
class WBPUBLICBACKEND_PUBLIC_FUNC SqlFacade {
protected:
  std::set<std::string> _charsets;

public:
  typedef SqlFacade *Ref;
  typedef std::list<std::pair<std::string, std::string> > String_tuple_list;

public:
  static SqlFacade::Ref instance_for_rdbms(db_mgmt_RdbmsRef rdbms);
  static SqlFacade::Ref instance_for_rdbms_name(const std::string &name);

  virtual int splitSqlScript(const std::string &sql, std::list<std::string> &statements) = 0;
  virtual int splitSqlScript(const char *sql, size_t length, const std::string &intial_delimiter,
                             std::vector<std::pair<size_t, size_t> > &borders,
                             const std::string &line_break = "\n") = 0;

  virtual Sql_parser::Ref sqlParser() = 0;
  virtual int parseSqlScriptString(grt::Ref<db_Catalog> catalog, const std::string sql) = 0;
  virtual int parseSqlScriptStringEx(grt::Ref<db_Catalog> catalog, const std::string sql,
                                     const grt::DictRef options) = 0;
  virtual int parseSqlScriptFile(grt::Ref<db_Catalog> catalog, const std::string filename) = 0;
  virtual int parseSqlScriptFileEx(grt::Ref<db_Catalog> catalog, const std::string filename,
                                   const grt::DictRef options) = 0;

  virtual Invalid_sql_parser::Ref invalidSqlParser() = 0;
  virtual int parseInserts(db_TableRef table, const std::string &sql) = 0;
  /* done */ virtual int parseTrigger(db_TriggerRef trigger, const std::string &sql) = 0;
  /* done */ virtual int parseRoutine(db_RoutineRef routine, const std::string &sql) = 0;
  /* done */ virtual int parseRoutines(db_RoutineGroupRef routineGroup, const std::string &sql) = 0;
  /* done */ virtual int parseView(db_ViewRef view, const std::string &sql) = 0;

  virtual Sql_syntax_check::Ref sqlSyntaxCheck() = 0;
  virtual int checkSqlSyntax(const std::string &sql) = 0;
  virtual int checkTriggerSyntax(const std::string &sql) = 0;
  virtual int checkViewSyntax(const std::string &sql) = 0;
  virtual int checkRoutineSyntax(const std::string &sql) = 0;

  virtual Sql_semantic_check::Ref sqlSemanticCheck() = 0;

  virtual Sql_specifics::Ref sqlSpecifics() = 0;

  virtual Sql_normalizer::Ref sqlNormalizer() = 0;
  virtual std::string normalizeSqlStatement(const std::string sql, const std::string schema_name) = 0;
  virtual std::string removeInterTokenSpaces(const std::string sql) = 0;

  virtual Sql_inserts_loader::Ref sqlInsertsLoader() = 0;

  virtual Sql_schema_rename::Ref sqlSchemaRenamer() = 0;
  /* done */ virtual int renameSchemaReferences(grt::Ref<db_Catalog> catalog, const std::string old_schema_name,
                                                const std::string new_schema_name) = 0;

  virtual Sql_statement_decomposer::Ref sqlStatementDecomposer(grt::DictRef db_opts = grt::DictRef()) = 0;

  virtual bool parseRoutineDetails(const std::string &sql, std::string &type, std::string &name,
                                   String_tuple_list &parameters, std::string &return_value, std::string &comments) = 0;
  virtual bool parseSelectStatementForEdit(const std::string &sql, std::string &schema_name, std::string &table_name,
                                           String_tuple_list &column_names) = 0;
  virtual bool parseDropStatement(const std::string &sql, std::string &object_type,
                                  std::vector<std::pair<std::string, std::string> > &object_names) = 0;

  virtual void stop_processing() = 0;
};

#endif /* _SQL_FACADE_H_ */
