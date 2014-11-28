add_definitions(-Wno-sign-compare -Wno-type-limits -Wno-unused
-Wno-missing-field-initializers)

include_directories(.
    ${GTK2_INCLUDE_DIRS}
    ${ANTLR3C_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/library/base
)

add_subdirectory(grammar)

add_library(mysqlparser
    MySQLLexer.c
    MySQLSimpleParser.c
    MySQLParser.c
    mysql-parser-common.cpp
    mysql-parser.cpp
    mysql-scanner.cpp
    mysql-syntax-check.cpp
)

add_dependencies(mysqlparser
    antlr-wb
    generate_parser
)

if (SKIP_BUNDLED_ANTLR)
else ()
    link_directories(${PROJECT_BINARY_DIR}/antlr-wb)
endif ()

target_link_libraries(mysqlparser ${GTK2_LIBRARIES} ${ANTLR3C_LIBRARIES})

set_target_properties(mysqlparser
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS mysqlparser DESTINATION ${WB_INSTALL_LIB_DIR})

install(DIRECTORY ${PROJECT_BINARY_DIR}/antlr-wb/ DESTINATION ${WB_INSTALL_LIB_DIR} FILES_MATCHING
    PATTERN "*.so"
    PATTERN "*.so.*"
)

set(DATA_FILES 
    MySQL.tokens
)

install(FILES ${DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/data)
