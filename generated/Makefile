top_srcdir = ..

genobj = $(top_srcdir)/wb-build/tools/genobj/genobj

genwrap = $(top_srcdir)/wb-build/tools/genwrap/genwrap

#BUILT_SOURCES=\
#  grts/structs.app.h\
#  grts/structs.db.mgmt.h\
#  grts/structs.db.mysql.h\
#  grts/structs.db.mssql.h\
#  grts/structs.db.sybase.h\
#  grts/structs.db.migration.h\
#  grts/structs.db.query.h\
#  grts/structs.db.ng.h\
#  grts/structs.db.h\
#  grts/structs.eer.h\
#  grts/structs.workbench.h\
#  grts/structs.meta.h\
#  grts/structs.model.h\
#  grts/structs.workbench.model.h\
#  grts/structs.workbench.model.reporting.h\
#  grts/structs.workbench.logical.h\
#  grts/structs.workbench.physical.h\
#  grts/structs.h\
#  grts/structs.ui.h\
#  grti/plugin.h\
#  grti/wb_model_reporting.h\
#  grti/wbvalidation.h

all: grts/*.h grts/structs.h grti/*.h modules

grts/structs.h: $(top_srcdir)/res/grt/structs.xml
	$(genobj) $< $(top_srcdir)/res/grt grts ../backend/wbpublic/objimpl

grts/%.h: $(top_srcdir)/res/grt/%.xml 
	$(genobj) $< $(top_srcdir)/res/grt grts ../backend/wbpublic/objimpl/`basename $<|sed -e 's/structs.\(.*\).xml/\1/'`

grti/%.h: $(top_srcdir)/modules/interfaces/%.h $(genwrap)
	$(genwrap) interfaces $< $@ 

modules:
	$(genwrap) wrappers generated/grtm 


grts/structs.app.h: $(top_srcdir)/res/grt/structs.app.xml

grts/structs.db.h: $(top_srcdir)/res/grt/structs.db.xml

grts/structs.db.mgmt.h: $(top_srcdir)/res/grt/structs.db.mgmt.xml

grts/structs.db.mysql.h: $(top_srcdir)/res/grt/structs.db.mysql.xml

grts/structs.db.mssql.h: $(top_srcdir)/res/grt/structs.db.mssql.xml

grts/structs.db.oracle.h: $(top_srcdir)/res/grt/structs.db.oracle.xml

grts/structs.db.sybase.h: $(top_srcdir)/res/grt/structs.db.sybase.xml

grts/structs.db.query.h: $(top_srcdir)/res/grt/structs.db.query.xml

grts/structs.db.ng.h: $(top_srcdir)/res/grt/structs.db.ng.xml

grts/structs.eer.h: $(top_srcdir)/res/grt/structs.eer.xml

grts/structs.ui.h: $(top_srcdir)/res/grt/structs.ui.xml

grts/structs.model.h: $(top_srcdir)/res/grt/structs.model.xml

grts/structs.workbench.h: $(top_srcdir)/res/grt/structs.workbench.xml

grts/structs.workbench.logical.h: $(top_srcdir)/res/grt/structs.workbench.logical.xml

grts/structs.workbench.model.h: $(top_srcdir)/res/grt/structs.workbench.model.xml

grts/structs.workbench.model.reporting.h: $(top_srcdir)/res/grt/structs.workbench.model.reporting.xml

grts/structs.workbench.physical.h: $(top_srcdir)/res/grt/structs.workbench.physical.xml

grts/structs.db.migration.h: $(top_srcdir)/res/grt/structs.db.migration.xml
