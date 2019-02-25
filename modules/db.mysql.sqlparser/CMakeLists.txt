add_library(db.mysql.sqlparser.grt
    src/db.mysql.sqlparser.cpp
    src/mysql_invalid_sql_parser.cpp
    src/mysql_sql_facade.cpp
    src/mysql_sql_parser_base.cpp
    src/mysql_sql_specifics.cpp
    src/mysql_sql_normalizer.cpp
    src/mysql_sql_parser.cpp
    src/mysql_sql_parser_fe.cpp
    src/mysql_sql_parser_utils.cpp
    src/mysql_sql_schema_rename.cpp
    src/mysql_sql_script_splitter.cpp
    src/mysql_sql_inserts_loader.cpp
    src/mysql_sql_syntax_check.cpp
    src/mysql_sql_semantic_check.cpp
    src/mysql_sql_statement_decomposer.cpp
)

target_include_directories(db.mysql.sqlparser.grt
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
)

target_include_directories(db.mysql.sqlparser.grt
 SYSTEM
  PRIVATE
    ${VSQLITE_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)


target_compile_options(db.mysql.sqlparser.grt PRIVATE ${WB_CXXFLAGS})

if (MYSQL_DEFINITIONS)
  target_compile_definitions(db.mysql.sqlparser.grt PRIVATE ${MYSQL_DEFINITIONS})
endif()

if (UNIX)
  target_compile_definitions(db.mysql.sqlparser.grt PRIVATE -DHAVE_MEMCPY=1 -DHAVE_MEMMOVE=1 -DUSE_MB=1 -DUSE_MB_IDENT=1 -DHAVE_STRTOUL=1)
endif()

add_dependencies(db.mysql.sqlparser.grt parsers)

target_link_libraries(db.mysql.sqlparser.grt 
  PUBLIC
    db.mysql.sqlparser.grt_Iface
  PRIVATE 
    sqlparser
    wbpublic
    grt
    Boost::boost
  )

add_library(db.mysql.sqlparser.grt_Iface INTERFACE)
add_library(db.mysql.sqlparser.grt::db.mysql.sqlparser.grt ALIAS db.mysql.sqlparser.grt_Iface)

target_include_directories(db.mysql.sqlparser.grt_Iface INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)


if(BUILD_FOR_GCOV)
  target_link_libraries(db.mysql.sqlparser.grt PRIVATE gcov)
endif()

set_source_files_properties(src/mysql_sql_parser_utils.cpp PROPERTIES COMPILE_FLAGS -Wno-varargs)

set_target_properties(db.mysql.sqlparser.grt
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})


if(COMMAND cotire)
    set_target_properties(db.mysql.sqlparser.grt PROPERTIES 
        COTIRE_ENABLE_PRECOMPILED_HEADER FALSE
        COTIRE_ADD_UNITY_BUILD FALSE
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")
    cotire(db.mysql.sqlparser.grt)
endif()


install(TARGETS db.mysql.sqlparser.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
