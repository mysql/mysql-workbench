

pkglib_LTLIBRARIES=libgrtbe.la

INCLUDES=@GLIB_CFLAGS@ @SIGC_CFLAGS@ @GRT_CFLAGS@\
	-I$(top_srcdir)/library/grt/src -I$(top_srcdir)/library/utilities/src\
	-I$(top_srcdir)/backend -I$(top_srcdir)/generated\
	-I$(top_srcdir)/library -I$(top_srcdir)/modules\
	-I$(top_srcdir)/ext

libgrtbe_la_SOURCES=\
	editor_base.cpp\
	parse_utils.cpp\
	common.cpp\
	grt_dispatcher.cpp\
	grt_manager.cpp\
	grt_reporter.cpp\
	grt_message_list.cpp\
	grt_modules_tree.cpp\
	grt_shell.cpp\
	grt_string_list_model.cpp\
	grt_structs_tree.cpp\
	grt_value_inspector.cpp\
	grt_value_tree.cpp\
	icon_manager.cpp\
	plugin_manager.cpp\
	tree_model.cpp\
	ui_form.cpp

