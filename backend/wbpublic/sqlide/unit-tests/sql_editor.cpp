#include "stdafx.h"


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
