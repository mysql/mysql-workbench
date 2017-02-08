set(PY_FILES 
    db_mysql_re_grt.py
    db_mysql_fe_grt.py
    db_mysql_migration_grt.py
)
install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})

set(MODULE_DATA_FILES
    res/mysql_engines.xml
    res/mysql_rdbms_info.xml
    res/mysql_reserved.xml
)
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})

set(BASIC_FILES
    res/db_mysql_catalog_reporting/Basic_Text.tpl/basic_text_report.txt.tpl
)
install(FILES ${BASIC_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data/db_mysql_catalog_reporting/Basic_Text.tpl)


include_directories(.
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/base/base
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff 
    ${PROJECT_SOURCE_DIR}/library/sql-parser/include
    ${PROJECT_SOURCE_DIR}/library/sql-parser/source
    ${PROJECT_SOURCE_DIR}/modules/db.mysql/src
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(db.mysql.grt
    src/db_mysql_catalog_report.cpp
    src/db_mysql_diffsqlgen.cpp
    src/db_mysql_params.cpp
    src/module_db_mysql.cpp
)

target_link_libraries(db.mysql.grt wbpublic ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${PCRE_LIBRARIES} ${SIGC++_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(db.mysql.grt gcov)
endif()

set_target_properties(db.mysql.grt
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(db.mysql.grt PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.mysql.grt)
endif()

install(TARGETS db.mysql.grt DESTINATION ${WB_PYTHON_MODULES_DIR})  #TODO: Is this where it should go?
