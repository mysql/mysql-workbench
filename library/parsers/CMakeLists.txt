add_definitions(-Wno-sign-compare -Wno-type-limits -Wno-unused
-Wno-missing-field-initializers)

include_directories(.
    ./mysql
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${ANTLR4_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/library/base
    SYSTEM ${Boost_INCLUDE_DIRS}
)

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
                  DEPENDS ${GENERATED_FILES})

set_source_files_properties(${GENERATED_FILES} PROPERTIES GENERATED TRUE)

set(PARSERS_SOURCES
    ${GENERATED_SOURCES}
    mysql/MySQLBaseLexer.cpp
    mysql/MySQLBaseRecognizer.cpp
    mysql/MySQLRecognizerCommon.cpp
    parsers-common.cpp
    code-completion/CodeCompletionCore.cpp
    code-completion/mysql-code-completion.cpp
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
    generate_parser_mysql
)

target_link_libraries(parsers ${GTK3_LIBRARIES} ${ANTLR4_LIBRARIES})

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


