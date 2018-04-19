/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MSC_VER
#include <sstream>
#endif

#include "sqlide/sql_editor_be.h"
#include "grt_test_utility.h"
#include "db_helpers.h"
#include "testgrt.h"
#include "wb_helpers.h"

using namespace grt;

BEGIN_TEST_DATA_CLASS(sql_editor)
public:
WBTester *tester;
db_mgmt_RdbmsRef rdbms;
TEST_DATA_CONSTRUCTOR(sql_editor) {
  tester = new WBTester;
}
END_TEST_DATA_CLASS

TEST_MODULE(sql_editor, "SQL editor");

TEST_FUNCTION(1) {
  rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->unserialize("data/res/mysql_rdbms_info.xml"));
  ensure("db_mgmt_RdbmsRef initialization", rdbms.is_valid());

  //! ListRef<db_mgmt_Rdbms> rdbms_list= ListRef<db_mgmt_Rdbms>::cast_from(grtm.get_grt()->get("/wb/rdbmsMgmt/rdbms"));
  ListRef<db_mgmt_Rdbms> rdbms_list(true);
  rdbms_list.insert(rdbms);

  GrtVersionRef version = bec::parse_version("5.6.10");
  version->name("MySQL Community Server (GPL)");
  parsers::MySQLParserServices::Ref services = parsers::MySQLParserServices::get();
  parsers::MySQLParserContext::Ref parser = services->createParserContext(rdbms->characterSets(), version, "", 1);
	ensure("failed to retrieve RDBMS list", rdbms_list.is_valid());
	for (size_t n = 0, count = rdbms_list.count(); n < count; ++n)
	{
		db_mgmt_RdbmsRef rdbms = rdbms_list[n];
		MySQLEditor::Ref sql_editor = MySQLEditor::create(parser, parser);
		ensure(("failed to get sql editor for " + rdbms->name().toString() + " RDBMS").c_str(), (NULL != sql_editor.get()));
	}
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
