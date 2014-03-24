#include "stdafx.h"


#ifndef _WIN32
#include <sstream>
#endif


#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/recordset_be.h"
#include "connection_helpers.h"
#include "testgrt.h"


BEGIN_TEST_DATA_CLASS(db_sql_editor)
public:
	GRTManagerTest grtm;
	SqlEditorForm::Ref db_sql_editor;
END_TEST_DATA_CLASS


TEST_MODULE(db_sql_editor, "DB SQL Editor");


TEST_FUNCTION(1)
{
  /*
  	db_mgmt_ConnectionRef conn(grtm.get_grt());
	setup_env(grtm.get_grt(), conn);  
	db_sql_editor.reset(new SqlEditorForm(&grtm, conn));

	try
	{
		db_sql_editor->exec_sql("select count(*) from information_schema.engines", true);
	}
	catch(const std::exception &)
	{
		ensure("error on query execution", false);
	}

	ensure_equals("wrong recordset count", db_sql_editor->recordset_count(), 1);
	Recordset *rs= db_sql_editor->recordset(0);
	ensure("failed to retrieve recordset", (NULL != rs));
	ensure_equals("wrong records count", rs->row_count(), 1);
	ensure_equals("wrong fields count", rs->get_column_count(), 1);
	ensure_equals("wrong field name", rs->get_column_caption(0), "count(*)");
   */
}


END_TESTS
