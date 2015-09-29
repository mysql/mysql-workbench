include_directories(. ../include ${PROJECT_SOURCE_DIR}/modules/db.mysql.sqlparser/src ${MYSQL_INCLUDE_DIRS})

if (UNIX)
	include_directories(./linux)
endif ()

if (UNIX)
  add_definitions(-DHAVE_MEMCPY=1 -DHAVE_MEMMOVE=1 -DUSE_MB=1 -DUSE_MB_IDENT=1 -DHAVE_STRTOUL=1)
endif()

#TODO: Set other compiler flags - doesn't work in gcc 4.6
#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas)
#endif ()

add_library(sqlparser
    charset-def.cpp
    charset.cpp
    ctype-big5.cpp
    ctype-bin.cpp
    ctype-cp932.cpp
    ctype-czech.cpp
    ctype-eucjpms.cpp
    ctype-euc_kr.cpp
    ctype-extra.cpp
    ctype-gb2312.cpp
    ctype-gbk.cpp
    ctype-latin1.cpp
    ctype-mb.cpp
    ctype-simple.cpp
    ctype-sjis.cpp
    ctype-tis620.cpp
    ctype-uca.cpp
    ctype-ucs2.cpp
    ctype-ujis.cpp
    ctype-utf8.cpp
    ctype-win1250ch.cpp
    ctype.cpp
    int2str.cpp
    myx_lex_helpers.cpp
    myx_sql_tree_item.cpp
    myx_statement_parser.cpp
    myx_unicode_scanner.cpp
    sql_parser_symbols.cpp
    my_messnc.cpp
    my_static.cpp
    my_strtoll10.cpp
    my_vsnprintf.cpp
    my_lib.cpp
    strmake.cpp
    strnmov.cpp
    strtod.cpp
    strxmov.cpp
    str_alloc.cpp
    linux/myx_sql_parser.cpp
    xml.cpp
)

set_target_properties(sqlparser
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS sqlparser DESTINATION ${WB_INSTALL_LIB_DIR})
