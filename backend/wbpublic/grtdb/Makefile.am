pkglib_LTLIBRARIES=libgrtdbbe.la

INCLUDES=@GLIB_CFLAGS@ @SIGC_CFLAGS@ @GRT_CFLAGS@ @MYSQL_INCLUDE@\
	-I$(top_srcdir)/library/grt/src -I$(top_srcdir)/library/utilities/src \
	-I$(top_srcdir)/library/\
	-I$(top_srcdir)/backend -I$(top_srcdir)/generated

libgrtdbbe_la_SOURCES=\
	charset_list.cpp\
	charset_utils.cpp\
	db_object_filter.cpp\
	db_object_helpers.cpp\
  db_object_master_filter.cpp\
	dbobject_roles.cpp\
	diff_dbobjectmatch.cpp\
	editor_dbobject.cpp\
	editor_routine.cpp\
	editor_routinegroup.cpp\
	editor_schema.cpp\
	editor_table.cpp\
	editor_table_inserts.cpp\
	editor_user.cpp\
	editor_user_role.cpp\
	editor_view.cpp\
	role_tree_model.cpp\
	../objimpl/db/db_DatabaseObject.cpp\
	../objimpl/db/db_Schema.cpp\
	../objimpl/db/db_Table.cpp\
	../objimpl/db/db_ForeignKey.cpp\
	../objimpl/db/db_Column.cpp\
	../grtsqlparser/module_utils.cpp\
	../grtsqlparser/sql_facade.cpp\
	../grtsqlparser/sql_parser_base.cpp\
	../sqlide/recordset.cpp\
	../sqlide/sql_editor.cpp
