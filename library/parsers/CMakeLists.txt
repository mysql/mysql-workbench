set(GENERATED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLLexer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParserBaseListener.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParserBaseVisitor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParserListener.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParserVisitor.cpp
)

set(GENERATED_OTHERS
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLLexer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLLexer.tokens
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParserBaseListener.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParserBaseVisitor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParser.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParserListener.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParser.tokens
  ${CMAKE_CURRENT_SOURCE_DIR}/mysql/MySQLParserVisitor.h
)

set (GENERATED_FILES ${GENERATED_OTHERS} ${GENERATED_SOURCES})

add_custom_command(OUTPUT ${GENERATED_FILES} 
          DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/grammars/MySQLLexer.g4 ${CMAKE_CURRENT_SOURCE_DIR}/grammars/MySQLParser.g4
          COMMAND ${Java_JAVA_EXECUTABLE} -Xmx1024m -jar ${ANTLR_JAR_PATH} -Dlanguage=Cpp -listener -visitor -o ../mysql -package parsers ${CMAKE_CURRENT_SOURCE_DIR}/grammars/MySQLLexer.g4 ${CMAKE_CURRENT_SOURCE_DIR}/grammars/MySQLParser.g4
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/grammars
          COMMENT "[mysql] Generating parser files..."
          )


add_custom_target(generate_parser_mysql
                  ALL
                  DEPENDS ${GENERATED_FILES}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_source_files_properties(${GENERATED_FILES} PROPERTIES GENERATED TRUE)

set(PARSERS_SOURCES
    ${GENERATED_SOURCES}
    mysql/MySQLBaseLexer.cpp
    mysql/MySQLBaseRecognizer.cpp
    mysql/MySQLRecognizerCommon.cpp
    parsers-common.cpp
    SymbolTable.cpp
    code-completion/CodeCompletionCore.cpp
    code-completion/mysql-code-completion.cpp
)

add_library(parsers
  ${PARSERS_SOURCES}
)



target_compile_options(parsers PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-sign-compare -Wno-type-limits -Wno-unused -Wno-missing-field-initializers -Wno-implicit-fallthrough>)

set(NEW_WB_CFLAGS ${WB_CFLAGS})
list(REMOVE_ITEM NEW_WB_CFLAGS -Werror)
target_compile_options(parsers PRIVATE ${NEW_WB_CFLAGS})

add_dependencies(parsers
    generate_parser_mysql)

target_link_libraries(parsers 
  PUBLIC 
   parsers_Iface
  PRIVATE 
   wbbase::wbbase
   ${GTK3_LIBRARIES})

if(BUILD_FOR_GCOV)
  target_link_libraries(parsers PRIVATE gcov)
endif()

add_library(parsers_Iface INTERFACE)
add_library(parsers::parsers ALIAS parsers_Iface)

target_include_directories(parsers_Iface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(parsers_Iface
 SYSTEM INTERFACE
    ${ANTLR4_INCLUDE_DIRS}
)

target_link_libraries(parsers_Iface INTERFACE
   ${ANTLR4_LIBRARIES}
) 


set_target_properties(parsers
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})
if(COMMAND cotire)
    set_target_properties(parsers PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(parsers)
endif()

install(TARGETS parsers DESTINATION ${WB_INSTALL_LIB_DIR})


