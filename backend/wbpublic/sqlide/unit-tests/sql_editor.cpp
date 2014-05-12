/*
* Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WIN32
#include <sstream>
#endif


#include "sqlide/sql_editor_be.h"
#include "testgrt.h"


BEGIN_TEST_DATA_CLASS(sql_editor)
public:
	GRTManagerTest grtm;
	db_mgmt_RdbmsRef rdbms;
END_TEST_DATA_CLASS


TEST_MODULE(sql_editor, "SQL Editor");


TEST_FUNCTION(1)
{
	rdbms= db_mgmt_RdbmsRef::cast_from(grtm.get_grt()->unserialize("../../modules/db.mysql/res/mysql_rdbms_info.xml"));
	ensure("db_mgmt_RdbmsRef initialization", rdbms.is_valid());

	//!ListRef<db_mgmt_Rdbms> rdbms_list= ListRef<db_mgmt_Rdbms>::cast_from(grtm.get_grt()->get("/wb/rdbmsMgmt/rdbms"));
	ListRef<db_mgmt_Rdbms> rdbms_list(grtm.get_grt());
	rdbms_list.insert(rdbms);

	ensure("failed to retrieve RDBMS list", rdbms_list.is_valid());
	for (int n= 0, count= rdbms_list.count(); n < count; ++n)
	{
		db_mgmt_RdbmsRef rdbms= rdbms_list[n];
		Sql_editor::Ref sql_editor= Sql_editor::create(rdbms);
		ensure(("failed to get sql editor for " + rdbms->name().repr() + " RDBMS").c_str(), (NULL != sql_editor.get()));
	}
}


END_TESTS
