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
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(db.search.wbp MODULE
    DbSearchFilterPanel.cpp
    DbSearchPanel.cpp
    register_plugin.cpp
)

target_compile_options(db.search.wbp PUBLIC ${WB_CXXFLAGS})

set_target_properties(db.search.wbp PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
  VERSION   ${WB_VERSION}
  SOVERSION ${WB_VERSION}
)

#target_link_libraries(db.mysql.wbp linux_utilities wbscintilla)

if(BUILD_FOR_TESTS)
  target_link_libraries(db.search.wbp gcov)
endif()

if(COMMAND cotire)
    set_target_properties(db.search.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.search.wbp)
endif()
install(TARGETS db.search.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
