include_directories(.
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/ext
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/modules/db.mysql/src
    ${PROJECT_SOURCE_DIR}/library/dbc/src
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/modules/
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/forms/mforms
    ${PROJECT_SOURCE_DIR}/plugins
    ../backend
    ${PROJECT_SOURCE_DIR}
    ${GTK2_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
)

add_library(wb.model.snippets.wbp MODULE
    include_model_wizard.cpp
    merge_model.cpp
    register_plugin.cpp
)

set_target_properties(wb.model.snippets.wbp PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(wb.model.snippets.wbp wbscintilla)

install(TARGETS wb.model.snippets.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
