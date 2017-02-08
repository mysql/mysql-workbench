/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_DB_SCHEMA_H_
#define _WB_DB_SCHEMA_H_

#include "cppdbc.h"

namespace wb {

  class InternalSchema {
  private:
    sql::Dbc_connection_handler::Ref &_connection;
    std::string _schema_name;
    bool check_table_or_view_exists(const std::string object_name, bool check_view);
    bool check_function_or_sp_exists(const std::string object_name, bool check_function);

  public:
    InternalSchema(const std::string &schema_name, sql::Dbc_connection_handler::Ref &conn);
    ~InternalSchema(void);

    std::string schema_name() const {
      return _schema_name;
    }

    bool check_schema_exist();
    bool check_function_exists(const std::string &function_name);
    bool check_stored_procedure_exists(const std::string &spname);
    bool check_table_exists(const std::string &table_name);
    bool check_view_exists(const std::string &view_name);

    bool is_remote_search_deployed();

    bool check_snippets_table_exist();
    std::string create_snippets_table_exist();
    int insert_snippet(const std::string &title, const std::string &code);
    void delete_snippet(int snippet_id);
    void set_snippet_title(int snippet_id, const std::string &title);
    void set_snippet_code(int snippet_id, const std::string &code);

    std::string create_schema();
    std::string deploy_remote_search();
    std::string deploy_get_objects_sp();
    std::string deploy_get_tables_and_views_sp();
    std::string deploy_get_routines();
    std::string execute_sql(const std::string &statement);
  };
};

#endif
