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

#include "base/config_file.h"

#include "workbench/wb_overview.h"
#include "grts/structs.workbench.h"
#include "grts/structs.workbench.logical.h"
#include "grts/structs.workbench.physical.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

using namespace base;

$describe("Base library config file handling")  {
  $it("No config file. Creates a default section.", []() {
    ConfigurationFile file("", AutoCreateNothing);
    $expect(file.section_count()).toEqual(1);
    $expect(file.has_section("")).toBeTrue();
    $expect(file.key_count()).toEqual(0);
  });

  $it("Empty config file. No keys, no fun.", []() {
    ConfigurationFile file("data/base/my-1.ini", AutoCreateNothing);
    $expect(file.section_count()).toEqual(1);
    $expect(file.has_section("")).toBeTrue();
    $expect(file.key_count()).toEqual(0);
  });

  $it("Pretty standard ini file (Windows). No includes or other specialties.", []() {
    ConfigurationFile file("data/base/my-2.ini", AutoCreateNothing);
    $expect(file.section_count()).toEqual(4);
    $expect(file.has_section("client")).toBeTrue();
    $expect(file.has_section("MYSQLd")).toBeTrue();
    $expect(file.has_section("mYsQl")).toBeTrue();
    $expect(!file.has_section("mysqlx")).toBeTrue();
    $expect(file.key_count()).toBe(32);

    file.clear();
    $expect(file.section_count()).toEqual(1);
    $expect(file.has_section("")).toBeTrue();
    $expect(file.key_count()).toEqual(0);

    file.create_section("mine", "A manually added section");
    $expect(file.section_count()).toEqual(2);
    $expect(file.has_section("mine")).toBeTrue();

    file.load("data/base/my-2.ini");
    $expect(file.section_count()).toEqual(5);
    $expect(file.has_section("client")).toBeTrue();
    $expect(file.has_section("MYSQLd")).toBeTrue();
    $expect(file.has_section("mYsQl")).toBeTrue();
    $expect(file.has_section("mine")).toBeTrue(); // Must still be there.
    $expect(!file.has_section("mysqlx")).toBeTrue();
    $expect(file.key_count()).toBe(32);

    // Save and reload. We cannot directly compare the original file and its generated copy
    // as there are differences in whitespaces. But we can compare content.
    file.save("data/base/my-2-copy.ini");
    ConfigurationFile file_copy("data/base/my-2-copy.ini", AutoCreateNothing);
    $expect(file_copy.section_count()).toEqual(5);
    $expect(file_copy.has_section("client")).toBeTrue();
    $expect(file_copy.has_section("MYSQLd")).toBeTrue();
    $expect(file_copy.has_section("mYsQl")).toBeTrue();
    $expect(file_copy.key_count()).toBe(32);
  });

  $it("Detailed check of loaded content.", []() {
    ConfigurationFile file("data/base/my-2.ini", AutoCreateNothing);

    $expect(file.get_int("port", "client")).toBe(3306);

    $expect(file.get_value("default-character-set", "mysql")).toBe("utf8");

    $expect(file.get_value("basedir", "mysqld")).toBe("\"C:/Program Files/MySQL/MySQL Server 5.5/\"");
    $expect(file.get_value("datadir", "mysqld")).toBe("\"C:/ProgramData/MySQL/MySQL Server 5.5/Data/\"");
    $expect(file.get_value("character-set-server", "mysqld")).toBe("utf8");
    $expect(file.get_value("default-storage-engine", "mysqld")).toBe("INNODB");
    $expect(file.get_value("sql-mode", "mysqld"))
      .toBe("\"STRICT_TRANS_TABLES,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION\"");
    $expect(file.get_int("max_connections", "mysqld")).toBe(100);

    $expect(file.get_int("query_cache_size", "mysqld")).toEqual(0);
    $expect(file.get_bool("query_cache_size", "mysqld")).toBe(false);

    $expect(file.get_int("table_cache", "mysqld")).toBe(256);

    $expect(file.get_value("tmp_table_size", "mysqld")).toBe("35M");
    $expect(file.get_int("tmp_table_size", "mysqld")).toBe(35 * 1024 * 1024);

    $expect(file.get_int("thread_cache_size", "mysqld")).toEqual(8);

    $expect(file.get_value("myisam_max_sort_file_size", "mysqld")).toBe("100G");
    $expect(file.get_float("myisam_max_sort_file_size", "mysqld")).toBe(100.0 * 1024 * 1024 * 1024);

    $expect(file.get_value("myisam_sort_buffer_size", "mysqld")).toBe("69M");
    $expect(file.get_float("myisam_sort_buffer_size", "mysqld")).toBe(69.0 * 1024 * 1024);

    $expect(file.get_value("key_buffer_size", "mysqld")).toBe("55M");
    $expect(file.get_int("key_buffer_size", "mysqld")).toBe(55 * 1024 * 1024);

    $expect(file.get_value("read_buffer_size", "mysqld")).toBe("64K");
    $expect(file.get_float("read_buffer_size", "mysqld")).toBe(64.0 * 1024);

    $expect(file.get_value("read_rnd_buffer_size", "mysqld")).toBe("256K");
    $expect(file.get_float("read_rnd_buffer_size", "mysqld")).toBe(256.0 * 1024);
    $expect(file.get_bool("read_rnd_buffer_size", "mysqld")).toBe(true);

    $expect(file.get_value("sort_buffer_size", "mysqld")).toBe("256K");
    $expect(file.get_int("sort_buffer_size", "mysqld")).toBe(256 * 1024);

    $expect(file.get_value("innodb_additional_mem_pool_size", "mysqld")).toBe("3M");
    $expect(file.get_int("innodb_additional_mem_pool_size", "mysqld")).toBe(3 * 1024 * 1024);

    $expect(file.get_int("innodb_flush_log_at_trx_commit", "mysqld")).toEqual(1);

    $expect(file.get_value("innodb_log_buffer_size", "mysqld")).toBe("2M");
    $expect(file.get_int("innodb_log_buffer_size", "mysqld")).toBe(2 * 1024 * 1024);

    $expect(file.get_value("innodb_buffer_pool_size", "mysqld")).toBe("107M");
    $expect(file.get_int("innodb_buffer_pool_size", "mysqld")).toBe(107 * 1024 * 1024);

    $expect(file.get_value("innodb_log_file_size", "mysqld")).toBe("54M");
    $expect(file.get_int("innodb_log_file_size", "mysqld")).toBe(54 * 1024 * 1024);

    $expect(file.get_int("innodb_thread_concurrency", "mysqld")).toEqual(8);

    $expect(file.has_key("slow-query-log", "mysqld")).toBeTrue();
    $expect(file.get_value("slow-query-log", "mysqld")).toBe(""); // No value for this key.

    $expect(file.get_value("log-output", "mysqld")).toBe("TABLE");
    $expect(file.has_key("log-slow-admin-statements", "mysqld")).toBeTrue();
    $expect(file.get_int("long_query_time", "mysqld")).toEqual(10);
    $expect(file.has_key("log-slow-slave-statements", "mysqld")).toBeTrue();

    $expect(file.get_bool("dummy-with-bool", "mysqld")).toBe(true);
    $expect(file.has_key("general-log", "mysqld")).toBeTrue();
  });

  $it("Specific tests for !include and !includedir entries.", []() {
    ConfigurationFile file("data/base/my-3.ini", AutoCreateNothing);
    std::vector<std::string> includes = file.get_includes("");
    $expect(includes.size()).toBe(1U);
    $expect(includes[0]).toBe("C:\\\\test.cnf");

    includes = file.get_includes("mysqld");
    $expect(includes.size()).toBe(2U);
    $expect(includes[0]).toBe("C:\\\\test.cnf");
    $expect(includes[1]).toBe("C:/config-files/");

    file.clear_includes("xxx");
    includes = file.get_includes("mysqld");
    $expect(includes.size()).toBe(2U);

    file.clear_includes("mysqld");
    includes = file.get_includes("mysqld");
    $expect(includes.size()).toBe(0U);

    file.add_include("mysqld", "abc");
    file.add_include_dir("mysqld", "def");
    includes = file.get_includes("mysqld");
    $expect(includes.size()).toBe(2U);
    $expect(includes[0]).toBe("abc");
    $expect(includes[1]).toBe("def");
  });
}

//----------------------------------------------------------------------------------------------------------------------

}
