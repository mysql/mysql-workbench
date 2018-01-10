/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/config_file.h"
#include "wb_helpers.h"

TEST_MODULE(config_file_test, "Base library config file handling");

using namespace base;

TEST_FUNCTION(10) {
  // No config file. Creates a default section.
  ConfigurationFile file("", AutoCreateNothing);
  ensure_equals("Only 1 (default) section exists", file.section_count(), 1);
  ensure("Default section exists", file.has_section(""));
  ensure_equals("No key exists", file.key_count(), 0);
}

TEST_FUNCTION(20) {
  // Empty config file. No keys, no fun.
  ConfigurationFile file("data/base/my-1.ini", AutoCreateNothing);
  ensure_equals("Only 1 (default) section exists", file.section_count(), 1);
  ensure("Default section exists", file.has_section(""));
  ensure_equals("No key exists", file.key_count(), 0);
}

TEST_FUNCTION(30) {
  // Pretty standard ini file (Windows). No includes or other specialties.
  ConfigurationFile file("data/base/my-2.ini", AutoCreateNothing);
  ensure_equals("30.1 Check section count", file.section_count(), 4);
  ensure("30.2 Section check", file.has_section("client"));
  ensure("30.3 Section check", file.has_section("MYSQLd"));
  ensure("30.4 Section check", file.has_section("mYsQl"));
  ensure("30.5 Invalid section check", !file.has_section("mysqlx"));
  ensure_equals("Check key count", file.key_count(), 32);

  file.clear();
  ensure_equals("30.5 Only 1 (default) section exists", file.section_count(), 1);
  ensure("30.6 Default section exists", file.has_section(""));
  ensure_equals("30.7 No key exists", file.key_count(), 0);

  file.create_section("mine", "A manually added section");
  ensure_equals("30.8 New section was added", file.section_count(), 2);
  ensure("30.9 New section exists", file.has_section("mine"));

  file.load("data/base/my-2.ini");
  ensure_equals("30.10 Check section count", file.section_count(), 5);
  ensure("30.11 Section check", file.has_section("client"));
  ensure("30.12 Section check", file.has_section("MYSQLd"));
  ensure("30.13 Section check", file.has_section("mYsQl"));
  ensure("30.14 Section check", file.has_section("mine")); // Must still be there.
  ensure("30.15 Invalid section check", !file.has_section("mysqlx"));
  ensure_equals("30.16 Check key count", file.key_count(), 32);

  // Save and reload. We cannot directly compare the original file and its generated copy
  // as there are differences in whitespaces. But we can compare content.
  file.save("data/base/my-2-copy.ini");
  ConfigurationFile file_copy("data/base/my-2-copy.ini", AutoCreateNothing);
  ensure_equals("30.17 Check section count", file_copy.section_count(), 5);
  ensure("30.18 Section check", file_copy.has_section("client"));
  ensure("30.19 Section check", file_copy.has_section("MYSQLd"));
  ensure("30.20 Section check", file_copy.has_section("mYsQl"));
  ensure_equals("30.21 Check key count", file_copy.key_count(), 32);
}

TEST_FUNCTION(40) {
  // Detailed check of loaded content.
  ConfigurationFile file("data/base/my-2.ini", AutoCreateNothing);

  ensure_equals("40.1 value check", file.get_int("port", "client"), 3306);

  ensure_equals("40.2 value check", file.get_value("default-character-set", "mysql"), "utf8");

  ensure_equals("40.3 value check", file.get_value("basedir", "mysqld"),
                "\"C:/Program Files/MySQL/MySQL Server 5.5/\"");
  ensure_equals("40.4 value check", file.get_value("datadir", "mysqld"),
                "\"C:/ProgramData/MySQL/MySQL Server 5.5/Data/\"");
  ensure_equals("40.5 value check", file.get_value("character-set-server", "mysqld"), "utf8");
  ensure_equals("40.6 value check", file.get_value("default-storage-engine", "mysqld"), "INNODB");
  ensure_equals("40.7 value check", file.get_value("sql-mode", "mysqld"),
                "\"STRICT_TRANS_TABLES,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION\"");
  ensure_equals("40.8 value check", file.get_int("max_connections", "mysqld"), 100);

  ensure_equals("40.9 value check", file.get_int("query_cache_size", "mysqld"), 0);
  ensure_equals("40.10 value check", file.get_bool("query_cache_size", "mysqld"), false);

  ensure_equals("40.11 value check", file.get_int("table_cache", "mysqld"), 256);

  ensure_equals("40.12 value check", file.get_value("tmp_table_size", "mysqld"), "35M");
  ensure_equals("40.13 value check", file.get_int("tmp_table_size", "mysqld"), 35 * 1024 * 1024);

  ensure_equals("40.14 value check", file.get_int("thread_cache_size", "mysqld"), 8);

  ensure_equals("40.15 value check", file.get_value("myisam_max_sort_file_size", "mysqld"), "100G");
  ensure_equals("40.16 value check", file.get_float("myisam_max_sort_file_size", "mysqld"), 100.0 * 1024 * 1024 * 1024);

  ensure_equals("40.17 value check", file.get_value("myisam_sort_buffer_size", "mysqld"), "69M");
  ensure_equals("40.18 value check", file.get_float("myisam_sort_buffer_size", "mysqld"), 69.0 * 1024 * 1024);

  ensure_equals("40.19 value check", file.get_value("key_buffer_size", "mysqld"), "55M");
  ensure_equals("40.20 value check", file.get_int("key_buffer_size", "mysqld"), 55 * 1024 * 1024);

  ensure_equals("40.21 value check", file.get_value("read_buffer_size", "mysqld"), "64K");
  ensure_equals("40.22 value check", file.get_float("read_buffer_size", "mysqld"), 64.0 * 1024);

  ensure_equals("40.23 value check", file.get_value("read_rnd_buffer_size", "mysqld"), "256K");
  ensure_equals("40.24 value check", file.get_float("read_rnd_buffer_size", "mysqld"), 256.0 * 1024);
  ensure_equals("40.25 value check", file.get_bool("read_rnd_buffer_size", "mysqld"), true);

  ensure_equals("40.26 value check", file.get_value("sort_buffer_size", "mysqld"), "256K");
  ensure_equals("40.27 value check", file.get_int("sort_buffer_size", "mysqld"), 256 * 1024);

  ensure_equals("40.28 value check", file.get_value("innodb_additional_mem_pool_size", "mysqld"), "3M");
  ensure_equals("40.29 value check", file.get_int("innodb_additional_mem_pool_size", "mysqld"), 3 * 1024 * 1024);

  ensure_equals("40.30 value check", file.get_int("innodb_flush_log_at_trx_commit", "mysqld"), 1);

  ensure_equals("40.31 value check", file.get_value("innodb_log_buffer_size", "mysqld"), "2M");
  ensure_equals("40.32 value check", file.get_int("innodb_log_buffer_size", "mysqld"), 2 * 1024 * 1024);

  ensure_equals("40.33 value check", file.get_value("innodb_buffer_pool_size", "mysqld"), "107M");
  ensure_equals("40.34 value check", file.get_int("innodb_buffer_pool_size", "mysqld"), 107 * 1024 * 1024);

  ensure_equals("40.35 value check", file.get_value("innodb_log_file_size", "mysqld"), "54M");
  ensure_equals("40.36 value check", file.get_int("innodb_log_file_size", "mysqld"), 54 * 1024 * 1024);

  ensure_equals("40.37 value check", file.get_int("innodb_thread_concurrency", "mysqld"), 8);

  ensure("40.38 value check", file.has_key("slow-query-log", "mysqld"));
  ensure_equals("40.39 value check", file.get_value("slow-query-log", "mysqld"), ""); // No value for this key.

  ensure_equals("40.40 value check", file.get_value("log-output", "mysqld"), "TABLE");
  ensure("40.41 value check", file.has_key("log-slow-admin-statements", "mysqld"));
  ensure_equals("40.42 value check", file.get_int("long_query_time", "mysqld"), 10);
  ensure("40.43 value check", file.has_key("log-slow-slave-statements", "mysqld"));

  ensure_equals("40.44 value check", file.get_bool("dummy-with-bool", "mysqld"), true);
  ensure("40.45 value check", file.has_key("general-log", "mysqld"));
}

TEST_FUNCTION(50) {
  // Specific tests for !include and !includedir entries.
  ConfigurationFile file("data/base/my-3.ini", AutoCreateNothing);
  std::vector<std::string> includes = file.get_includes("");
  ensure_equals("50.1 Number of includes", includes.size(), 1U);
  ensure_equals("50.2 Check include", includes[0], "C:\\\\test.cnf");

  includes = file.get_includes("mysqld");
  ensure_equals("50.3 Number of includes", includes.size(), 2U);
  ensure_equals("50.4 Check include", includes[0], "C:\\\\test.cnf");
  ensure_equals("50.5 Check include", includes[1], "C:/config-files/");

  file.clear_includes("xxx");
  includes = file.get_includes("mysqld");
  ensure_equals("50.6 Number of includes", includes.size(), 2U);

  file.clear_includes("mysqld");
  includes = file.get_includes("mysqld");
  ensure_equals("50.7 Number of includes", includes.size(), 0U);

  file.add_include("mysqld", "abc");
  file.add_include_dir("mysqld", "def");
  includes = file.get_includes("mysqld");
  ensure_equals("50.8 Number of includes", includes.size(), 2U);
  ensure_equals("50.9 Check include", includes[0], "abc");
  ensure_equals("50.10 Check include", includes[1], "def");
}

END_TESTS;

//----------------------------------------------------------------------------------------------------------------------
