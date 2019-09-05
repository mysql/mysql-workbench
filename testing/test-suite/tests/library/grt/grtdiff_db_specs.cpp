/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "casmine.h"
#include "wb_test_helpers.h"

#include "grt/grt_manager.h"
#include "grt.h"

#include "grts/structs.h"
#include "grts/structs.workbench.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.db.mgmt.h"

#include "cppdbc.h"
#include "backend/db_rev_eng_be.h"

#include "db_mysql_diffsqlgen.h"

#include "diff/diffchange.h"
#include "diff/grtdiff.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"
#include "db.mysql.sqlparser/src/mysql_sql_parser_fe.h"

namespace {

$ModuleEnvironment() {};

struct A {
  std::string _res;

  std::string convert(const char *s, int *err_count = 0) {
    _res = "";
    int _err_count = 0;

    _err_count = Mysql_sql_parser_fe(bec::GRTManager::get()->get_app_option_string("SqlMode"))
                   .parse_sql_script(s, &process_sql_statement_cb, this);

    if (err_count)
      *err_count = _err_count;

    return _res;
  }

private:
  static int process_sql_statement_cb(void *user_data, const MyxStatementParser *splitter, const char *sql,
                                      const SqlAstNode *tree, int stmt_begin_lineno, int stmt_begin_line_pos,
                                      int stmt_end_lineno, int stmt_end_line_pos, int err_tok_lineno,
                                      int err_tok_line_pos, int err_tok_len, const std::string &err_msg) {
    A &parser_be = *(reinterpret_cast<A *>(user_data));
    if (tree) {
      tree->build_sql(parser_be._res);
      //     std::ofstream("c:/1.xml") << *tree;
    }
    return (tree == 0) ? 1 : 0;
  }
};

const char *sql =
  "DELIMITER //\n"
  "CREATE TRIGGER `sakila`.`ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN\n"
  "    INSERT INTO film_text (film_id, title, description)\n"
  "        VALUES (new.film_id, new.title, new.description);\n"
  "  END//\n";

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};

$describe("GRT diff db") {
  $beforeAll([&]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();
  });

  $it("SQL test", []() {
    const char *sql_after_formatting_change =
      "DELIMITER //\n CREATE TRIGGER `sakila`.`ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN INSERT INTO film_text "
      "(film_id, title, description) VALUES (new.film_id, new.title, new.description);\n"
      "  END//\n";
    const char *sql_after_expression_change =
      "DELIMITER //\n CREATE TRIGGER `sakila`.`ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN INSERT INTO film_text "
      "(film_id, title, description) VALUES (new.film_id, new.title, 'new.description');\n"
      "  END//\n";
    const char *sql_after_same = sql;

    std::string before = A().convert(sql);

    $expect(A().convert(sql_after_formatting_change)).toEqual(before);
    $expect(A().convert(sql_after_expression_change)).Not.toEqual(before);
    $expect(A().convert(sql_after_same)).toEqual(before);
  });

}
}
