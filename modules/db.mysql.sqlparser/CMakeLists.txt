include_directories(.
    ./src
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/sql.parser/include
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/linux
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime/include
    ${PROJECT_SOURCE_DIR}/library/parsers
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${MySQL_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

if (MYSQL_DEFINITIONS)
	add_definitions(${MYSQL_DEFINITIONS})
endif()

if (UNIX)
  add_definitions(-DHAVE_MEMCPY=1 -DHAVE_MEMMOVE=1 -DUSE_MB=1 -DUSE_MB_IDENT=1 -DHAVE_STRTOUL=1)
endif()

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
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/charset-def.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/charset.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-big5.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-bin.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-cp932.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-czech.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-eucjpms.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-euc_kr.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-extra.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-gb2312.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-gbk.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-latin1.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-mb.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-simple.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-sjis.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-tis620.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-uca.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-ucs2.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-ujis.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-utf8.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype-win1250ch.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/ctype.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/int2str.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/myx_lex_helpers.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/myx_sql_tree_item.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/myx_statement_parser.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/myx_unicode_scanner.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/sql_parser_symbols.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/my_messnc.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/my_static.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/my_strtoll10.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/my_vsnprintf.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/strmake.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/strnmov.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/strtod.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/strxmov.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/str_alloc.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/linux/myx_sql_parser.cpp
    ${PROJECT_SOURCE_DIR}/library/sql.parser/source/xml.cpp
)

target_compile_options(db.mysql.sqlparser.grt PUBLIC ${WB_CXXFLAGS})

#We need to wait for parsers, because we're including some antlr headers which parser depends on
add_dependencies(db.mysql.sqlparser.grt parsers)


target_link_libraries(db.mysql.sqlparser.grt ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${SIGC++_LIBRARIES} ${PCRE_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(db.mysql.sqlparser.grt gcov)
endif()

set_source_files_properties(src/mysql_sql_parser_utils.cpp PROPERTIES COMPILE_FLAGS -Wno-varargs)
set_source_files_properties(${PROJECT_SOURCE_DIR}/library/sql.parser/source/myx_sql_tree_item.cpp PROPERTIES COMPILE_FLAGS -Wno-varargs)


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
