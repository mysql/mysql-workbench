add_definitions(-Wno-sign-compare -Wno-type-limits -Wno-unused
-Wno-missing-field-initializers)

include_directories(.
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${ANTLR3C_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/library/base
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_subdirectory(grammars)

set(PARSERS_SOURCES
    MySQLLexer.c
    MySQLSimpleParser.c
    MySQLParser.c
    parsers-common.cpp
    mysql-parser-common.cpp
    mysql-parser.cpp
    mysql-scanner.cpp
    mysql-syntax-check.cpp
    code-completion/mysql-code-completion.cpp
    code-completion/mysql_object_names_cache.cpp
    grammar-parser/ANTLRv3Lexer.c
    grammar-parser/ANTLRv3Parser.c
)

add_library(parsers
  ${PARSERS_SOURCES}
)

set(NEW_WB_CFLAGS ${WB_CFLAGS})
list(REMOVE_ITEM NEW_WB_CFLAGS -Werror)
target_compile_options(parsers PUBLIC ${NEW_WB_CFLAGS})


foreach(ITEM in ${PARSERS_SOURCES})
  get_filename_component(FILEEXT ${ITEM} EXT)
  if ("${FILEEXT}" STREQUAL ".cpp")
    set_source_files_properties(${ITEM} PROPERTIES COMPILE_FLAGS -std=c++11)
  endif()
endforeach(ITEM in ${PARSERS_SOURCES})

add_dependencies(parsers
    antlr-wb
    generate_parser_mysql
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
set_source_files_properties(MySQLLexer.c 
    PROPERTIES COMPILE_FLAGS "-Wno-tautological-compare")
set_source_files_properties(grammar-parser/ANTLRv3Lexer.c
     PROPERTIES COMPILE_FLAGS "-Wno-tautological-compare")
endif()


if (SKIP_BUNDLED_ANTLR)
else ()
    link_directories(${PROJECT_BINARY_DIR}/antlr-wb)
endif ()

target_link_libraries(parsers ${GTK3_LIBRARIES} ${ANTLR3C_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(parsers gcov)
endif()

set_target_properties(parsers
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})
if(COMMAND cotire)
    set_target_properties(parsers PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(parsers)
endif()

install(TARGETS parsers DESTINATION ${WB_INSTALL_LIB_DIR})

install(DIRECTORY ${PROJECT_BINARY_DIR}/antlr-wb/ DESTINATION ${WB_INSTALL_LIB_DIR} FILES_MATCHING
    PATTERN "*.so"
    PATTERN "*.so.*"
)

set(DATA_FILES 
    MySQL.tokens
)

install(FILES ${DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/data)
