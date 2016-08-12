add_definitions(-Wno-sign-compare -Wno-type-limits -Wno-unused
-Wno-missing-field-initializers)

include_directories(.
    ${GTK3_INCLUDE_DIRS}
    ${ANTLR3C_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/library/base
    ${Boost_INCLUDE_DIRS}
)

add_subdirectory(grammars)

add_library(parsers
    mysql/*.cpp
    ecma/*.cpp
    parsers-common.cpp
    code-completion/mysql-code-completion.cpp
    code-completion/mysql_object_names_cache.cpp
    grammar-parser/ANTLRv3Lexer.c
    grammar-parser/ANTLRv3Parser.c
)

add_dependencies(parsers
    antlr-wb
    generate_parser_mysql
    generate_parser_ecma
)

if (SKIP_BUNDLED_ANTLR)
else ()
    link_directories(${PROJECT_BINARY_DIR}/antlr-wb)
endif ()

target_link_libraries(parsers ${GTK3_LIBRARIES} ${ANTLR3C_LIBRARIES})

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
