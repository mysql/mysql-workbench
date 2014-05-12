include_directories(.
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/forms/mforms
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/modules/
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/cdbc/src
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${VSQLITE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${GRT_INCLUDE_DIRS}
    ${GTK2_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
)

add_library(db.search.wbp MODULE
    DbSearchFilterPanel.cpp
    DbSearchPanel.cpp
    register_plugin.cpp
)

set_target_properties(db.search.wbp PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

#target_link_libraries(db.mysql.wbp linux_utilities wbscintilla)

install(TARGETS db.search.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
