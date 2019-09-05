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

#ifndef _MSVC_VER
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "mysql_sql_parser_fe.h"
#include "grt/grt_manager.h"

#include "casmine.h"

namespace {

static const char *test_function_1_input[] = {
  "CREATE DATABASE IF NOT EXISTS `i-flow_dev` CHARACTER SET latin1 COLLATE latin1_swedish_ci;",
  "select * from a; select * from b; insert into c values (1,2,3); /* comment ; */; ;",
  "select * from a1; delimiter %%%\n delimiter $$$%%%\n select * from a2 $$$%%% select ",
  "select * from x; # comment ; \n select * from b",
  "/* comment ; ",
  "\" string ",
  "\' string ",
  "-- comment ; something ",
  "# comment ; something ",
  "select * from z; // blablabla ",
  "DELIMITER |\n CREATE1 |CREATE2|",
  "SELECT `тест`",
  "DELIMI",
  "DELIMITER",
  "DELIMITER ",
  "DELIMITER |\nx|",
  "DELIMITER //\nSELECTz1//SELECTz2",
  NULL
};

static const char *test_function_1_output[] = {
  "CREATE DATABASE IF NOT EXISTS `i-flow_dev` CHARACTER SET latin1 COLLATE latin1_swedish_ci",

  "select * from a",
  " select * from b",
  " insert into c values (1,2,3)",
  //" /* comment ; */", // comments are never returned
  //" ",  // empty statments are never returned

  "select * from a1",
  " select * from a2 ",
  " select ",

  "select * from x",
  " # comment ; \n select * from b",

  //"/* comment ; ",

  "\" string ",

  "\' string ",

  //"-- comment ; something ",

  //"# comment ; something ",

  "select * from z",
  " // blablabla ",

  " CREATE1 ",
  "CREATE2",

  "SELECT `тест`",

  "DELIMI",

  "DELIMITER",

  "DELIMITER ",

  "x",

  "SELECTz1",
  "SELECTz2"
};

//----------------------------------------------------------------------------------------------------------------------

static int test_function_1_output_index;
static bool test_function_1_success_flag;

int test_function_1_cb(void* user_data, const MyxStatementParser *splitter, const char *sql, const SqlAstNode *tree,
    int stmt_begin_lineno, int stmt_begin_line_pos, int stmt_end_lineno, int stmt_end_line_pos,
    int err_tok_lineno, int err_tok_line_pos, int err_tok_len, const std::string &err_msg) {
  test_function_1_success_flag &= (strcmp(sql, test_function_1_output[test_function_1_output_index++]) == 0);
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static int test_function_2_counter;

int test_function_2_cb(void* user_data, const MyxStatementParser *splitter, const char *sql, const SqlAstNode *tree,
    int stmt_begin_lineno, int stmt_begin_line_pos, int stmt_end_lineno, int stmt_end_line_pos,
    int err_tok_lineno, int err_tok_line_pos, int err_tok_len, const std::string &err_msg) {
  test_function_2_counter++;
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

int test_function_30_cb(void* user_data, const MyxStatementParser *splitter, const char *sql, const SqlAstNode *tree,
  int stmt_begin_lineno, int stmt_begin_line_pos, int stmt_end_lineno, int stmt_end_line_pos,
  int err_tok_lineno, int err_tok_line_pos, int err_tok_len, const std::string &err_msg) {
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

int test_function_5_cb(void* user_data, const MyxStatementParser *splitter, const char *sql, const SqlAstNode *tree,
  int stmt_begin_lineno, int stmt_begin_line_pos, int stmt_end_lineno, int stmt_end_line_pos,
  int err_tok_lineno, int err_tok_line_pos, int err_tok_len, const std::string &err_msg) {
  int *count = (int*)user_data;
  *count += 1;
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

class GRTManagerTest : public bec::GRTManager {
private:
  GRTManagerTest(bool verbose = true) : GRTManager(false) {
    setVerbose(verbose);
  }
  GRTManagerTest(const GRTManagerTest &) = delete;
  GRTManagerTest &operator=(GRTManagerTest &) = delete;

public:
  static std::shared_ptr<GRTManagerTest> get() {
    static std::shared_ptr<GRTManagerTest> instance(new GRTManagerTest());
    return instance;
  }
};

//----------------------------------------------------------------------------------------------------------------------

$ModuleEnvironment() {};

$describe("Old SQL parser tests") {
  $it("Simple SQL parsing", []() {
    test_function_1_output_index = 0;
    test_function_1_success_flag = true;

    Mysql_sql_parser_fe sql_parser_fe(GRTManagerTest::get()->get_app_option_string("SqlMode"));
    sql_parser_fe.ignore_dml = false;

    for (ssize_t i= 0; test_function_1_input[i] != NULL; i++)     {
      sql_parser_fe.parse_sql_script(test_function_1_input[i], test_function_1_cb, reinterpret_cast<void *>(i));
      $expect(test_function_1_success_flag).toBeTrue();
    }
  });

  $it("Parsing of different scripts", []() {
    Mysql_sql_parser_fe sql_parser_fe(GRTManagerTest::get()->get_app_option_string("SqlMode"));
    sql_parser_fe.ignore_dml = false;

    test_function_2_counter= 0;
    sql_parser_fe.parse_sql_script_file("data/db/menagerie-db/cr_event_tbl.sql", test_function_2_cb, NULL);
    $expect(test_function_2_counter).toEqual(2);

    test_function_2_counter= 0;
    sql_parser_fe.parse_sql_script_file("data/db/menagerie-db/cr_pet_tbl.sql", test_function_2_cb, NULL);
    $expect(test_function_2_counter).toEqual(2);

    test_function_2_counter= 0;
    sql_parser_fe.parse_sql_script_file("data/db/menagerie-db/ins_puff_rec.sql", test_function_2_cb, NULL);
    $expect(test_function_2_counter).toEqual(1);

    test_function_2_counter= 0;
    sql_parser_fe.parse_sql_script_file("data/db/menagerie-db/load_pet_tbl.sql", test_function_2_cb, NULL);
    $expect(test_function_2_counter).toEqual(2);

    test_function_2_counter= 0;
    sql_parser_fe.parse_sql_script_file("data/db/sakila-db/sakila-schema.sql", test_function_2_cb, NULL);
    $expect(test_function_2_counter).toBe(41);
  });

  $it("Parsing Sakila SQL dump", []() {
    const char *filename= "data/db/sakila-db/sakila-data.sql";

    $expect(g_file_test(filename, G_FILE_TEST_EXISTS)).toBeTrue();

  #if VERBOSE_OUTPUT
    test_time_point t1;
  #endif

    Mysql_sql_parser_fe sql_parser_fe(GRTManagerTest::get()->get_app_option_string("SqlMode"));
    sql_parser_fe.ignore_dml = false;
    sql_parser_fe.is_ast_generation_enabled = false; // Leave AST creation off. This adds a *huge* burden.
    sql_parser_fe.parse_sql_script_file(filename, test_function_30_cb, NULL);

  #if VERBOSE_OUTPUT
    test_time_point t2;

    struct _stat	statbuf;
    _stat((const char *)filename, &statbuf);

    float time_rate= ((float)1000.)/(t2 - t1).get_ticks();
    float size_per_sec= ((float)statbuf.st_size)*time_rate/1024/1024;
    std::cout << "Combined splitter + parser performance test (no AST): " << std::endl
      << "sakila-data.sql was processed in " << (t2 - t1) << " [" << size_per_sec << " MB/sec]" << std::endl;
  #endif
  });

  $it("tests bug #65749", []() {
    const char *filename= "data/db/empty_firstline.sql";
    int count = 0;

    $expect(g_file_test(filename, G_FILE_TEST_EXISTS)).toBeTrue();

    Mysql_sql_parser_fe sql_parser_fe(GRTManagerTest::get()->get_app_option_string("SqlMode"));
    sql_parser_fe.ignore_dml = false;
    sql_parser_fe.is_ast_generation_enabled = false; // Leave AST creation off. This adds a *huge* burden.
    sql_parser_fe.parse_sql_script_file(filename, test_function_5_cb, &count);

    $expect(count).toEqual(1);
  });

}

//----------------------------------------------------------------------------------------------------------------------

}
