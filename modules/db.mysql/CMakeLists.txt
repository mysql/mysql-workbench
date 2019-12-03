set(PY_FILES 
    db_mysql_re_grt.py
    db_mysql_fe_grt.py
    db_mysql_migration_grt.py
)
install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})

set(MODULE_DATA_FILES
    res/mysql_engines.xml
    res/mysql_rdbms_info.xml
)
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})

set(BASIC_FILES
    res/db_mysql_catalog_reporting/Basic_Text.tpl/basic_text_report.txt.tpl
)
install(FILES ${BASIC_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data/db_mysql_catalog_reporting/Basic_Text.tpl)

add_library(db.mysql.grt
    src/db_mysql_catalog_report.cpp
    src/db_mysql_diffsqlgen.cpp
    src/db_mysql_params.cpp
    src/module_db_mysql.cpp
)

target_include_directories(db.mysql.grt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  PRIVATE
    ${PROJECT_SOURCE_DIR}/library
    ${PROJECT_SOURCE_DIR}/modules
)

target_link_libraries(db.mysql.grt 
  PRIVATE 
    wbpublic
    grt
    wbbase
    parsers
    mtemplate
    Boost::boost
)

if(BUILD_FOR_GCOV)
  target_link_libraries(db.mysql.grt PRIVATE gcov)
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
