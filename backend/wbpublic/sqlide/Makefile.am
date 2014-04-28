pkglib_LTLIBRARIES=sqlide.be.la

INCLUDES=-I$(top_srcdir)/generated\
	-I$(top_srcdir)/backend\
	-I$(top_srcdir)/backend/grtsqlparser\
	-I$(top_srcdir)/library/grt/src \
	-I$(top_srcdir)/library/utilities/src\
	-I$(top_srcdir)/modules/interfaces\
	-I$(top_srcdir)/modules\
	-I$(top_srcdir)/library/grt/src/diff\
	-I$(top_srcdir)/library/sql-parser/include\
	-I$(top_srcdir)/library/sql-parser/source\
	-I$(top_srcdir)/library/sql-parser/source/linux\
	-I$(top_srcdir)/backend/grtdb\
	-I$(top_srcdir)/library/tinyxml\
	@MYSQL_INCLUDE@ @GRT_CFLAGS@ @GLIB_CFLAGS@ @SIGC_CFLAGS@

sqlide_be_la_SOURCES=\
	db_sql_editor.cpp\
	recordset_cdbc_storage.cpp\
	recordset_data_storage.cpp\
	sqlide.cpp
