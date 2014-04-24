
pkglib_LTLIBRARIES=libgrtui.la


libgrtui_la_SOURCES=\
grtdb_connect_panel.cpp\
grtdb_object_filter.cpp\
grt_wizard_form.cpp\
wizard_progress_page.cpp\
wizard_finished_page.cpp\
wizard_schema_filter_page.cpp\
wizard_object_filter_page.cpp\
wizard_view_text_page.cpp\
db_conn_be.cpp\
gui_plugin_base.cpp\
text_input_dialog.cpp\
checkbox_list_control.cpp\
tests/wiztest.cpp

INCLUDES=@CAIRO_CFLAGS@ @GNOME_CFLAGS@ @SIGC_CFLAGS@ @MYSQL_INCLUDE@\
-I$(top_srcdir)/backend\
-I$(top_srcdir)/library/dbc/src\
-I$(top_srcdir)/library/cppconn\
-I$(top_srcdir)/library/forms\
-I$(top_srcdir)/generated\
-I$(top_srcdir)/library/grt/src\
-I$(top_srcdir)/library/utilities/src\
-I$(top_srcdir)/library/sql-parser/include\
-I$(top_srcdir)/library/sql-parser/source

noinst_PROGRAMS=boxtest
#wiztest_SOURCES=tests/wiztest.cpp ../workbench/metaclasses.cpp

#wiztest_LDADD=libgrtui.la ../../library/forms/libmforms.a\
#	../../backend/wbcanvas/libcanvasbe.la\
#	../../backend/grtdb/libgrtdbbe.la\
#	../../backend/grt/libgrtbe.la\
#	../../library/canvas/src/libmdcanvas.la\
#	../../library/grt/src/libgrt.la ../../library/utilities/src/libguiutil.la \
#	../../library/dbc/common/libcdbc.la ../../library/dbc/mysql/libcdbc.mysql.la\
#	../../library/sql-parser/source/libsqlparser.la\
#	 @GRT_LIBS@ @GNOME_LIBS@ @CAIRO_LIBS@ -lgthread-2.0 @MYSQL_LIBS@

boxtest_SOURCES=tests/boxtest.cpp

boxtest_LDADD=../../library/forms/libmforms.la ../../library/grt/src/libgrt.la ../../library/utilities/src/libguiutil.la @PCRE_LIBS@ @GRT_LIBS@ @GNOME_LIBS@ @MYSQL_LIBS@


