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

#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/recordset_be.h"
#include "connection_helpers.h"
#include "testgrt.h"

BEGIN_TEST_DATA_CLASS(db_sql_editor)
public:
SqlEditorForm::Ref db_sql_editor;
END_TEST_DATA_CLASS

TEST_MODULE(db_sql_editor, "DB SQL Editor");

TEST_FUNCTION(1) {
  /*
        db_mgmt_ConnectionRef conn(grtm.get_grt());
        setup_env(conn);
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
