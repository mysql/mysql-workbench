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

target_include_directories(sqlparser
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:$<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/linux>>
  PRIVATE
    $<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/linux>
)

if (UNIX)
  target_compile_definitions(sqlparser PRIVATE -DHAVE_MEMCPY=1 -DHAVE_MEMMOVE=1 -DUSE_MB=1 -DUSE_MB_IDENT=1 -DHAVE_STRTOUL=1)
endif ()

target_link_libraries(sqlparser PUBLIC db.mysql.sqlparser.grt::db.mysql.sqlparser.grt)

if(BUILD_FOR_GCOV)
  target_link_libraries(sqlparser PRIVATE gcov)
endif()


set_source_files_properties(myx_sql_tree_item.cpp PROPERTIES COMPILE_FLAGS -Wno-varargs)
set_source_files_properties(myx_unicode_scanner.cpp PROPERTIES COMPILE_FLAGS "-Wno-empty-body -Wno-pragmas -Wno-implicit-fallthrough")
set_source_files_properties(ctype-utf8.cpp PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)
set_source_files_properties(myx_statement_parser.cpp PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)
set_source_files_properties(linux/myx_sql_parser.cpp PROPERTIES COMPILE_FLAGS -Wno-free-nonheap-object)


target_compile_options(sqlparser PRIVATE ${WB_CXXFLAGS})

set_target_properties(sqlparser PROPERTIES 
                      COMPILE_FLAGS -Wno-register
                      VERSION   ${WB_VERSION}
                      SOVERSION ${WB_VERSION})
if(COMMAND cotire)
  set_target_properties(sqlparser PROPERTIES 
      COTIRE_ENABLE_PRECOMPILED_HEADER FALSE
      COTIRE_ADD_UNITY_BUILD FALSE
      COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")
    cotire(sqlparser)
endif()
install(TARGETS sqlparser DESTINATION ${WB_INSTALL_LIB_DIR})
