
set(SOURCES_TO_GENERATE
    grts/structs.app.h
    grts/structs.db.mgmt.h
    grts/structs.db.mysql.h
    grts/structs.db.mssql.h
    grts/structs.db.sybase.h
    grts/structs.db.migration.h
    grts/structs.db.query.h
    grts/structs.db.ng.h
    grts/structs.db.h
    grts/structs.eer.h
    grts/structs.workbench.h
    grts/structs.meta.h
    grts/structs.model.h
    grts/structs.workbench.model.h
    grts/structs.workbench.model.reporting.h
    grts/structs.workbench.logical.h
    grts/structs.workbench.physical.h
    grts/structs.h
    grts/structs.ui.h
    grts/structs.wrapper.h
    #    grti/plugin.h
    #    grti/wb_model_reporting.h
    #    grti/wbvalidation.h
)

#grts/%.h: $(top_srcdir)/res/grt/%.xml 
#	$(genobj) $< $(top_srcdir)/res/grt grts ../backend/wbpublic/objimpl/`basename $<|sed -e 's/structs.\(.*\).xml/\1/'`
#
#grti/%.h: $(top_srcdir)/modules/interfaces/%.h $(genwrap)
#	$(genwrap) interfaces $< $@ 
#
#modules:
#	$(genwrap) wrappers generated/grtm 

set(GENERATED_SOURCES)
foreach(out_file ${SOURCES_TO_GENERATE})
	string(REGEX REPLACE "^.+/(.*)\\.h" "${PROJECT_SOURCE_DIR}/res/grt/\\1.xml" in_file ${out_file})
	string(REGEX REPLACE "^.+/structs\\.(.*)\\.h" "${PROJECT_SOURCE_DIR}/backend/wbpublic/objimpl/\\1" gen_dir ${out_file})
	#	message(WARNING "gen_dir computed as ${gen_dir}")
	if(NOT IS_DIRECTORY ${gen_dir})
		set(gen_dir "${PROJECT_SOURCE_DIR}/backend/wbpublic/objimpl")
	endif()
	set(comm "${PROJECT_BINARY_DIR}/tools/genobj/genobj ${in_file} ${PROJECT_SOURCE_DIR}/res/grt  ${PROJECT_SOURCE_DIR}/generated/grts ${gen_dir}")
	set(out_file "${PROJECT_SOURCE_DIR}/generated/${out_file}")
	add_custom_command(
		OUTPUT ${out_file}
		COMMAND ${comm}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		MAIN_DEPENDENCY ${in_file}
		COMMENT "Executing ${comm}"
	)
	list(APPEND GENERATED_SOURCES ${out_file})
	#	message(STATUS "will generate source ${out_file}")
endforeach()

add_custom_target(generated_objects DEPENDS ${GENERATED_SOURCES})

if(ENABLE_GRT_GENERATION)
	set_target_properties(generated_objects PROPERTIES
		EXCLUDE_FROM_ALL false
	)
endif()

add_dependencies(generated_objects gentool genwrap)
