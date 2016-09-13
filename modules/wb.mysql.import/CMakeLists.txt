include_directories(.
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    SYSTEM ${MySQL_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    SYSTEM ${TINYXML_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/ext
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff
    ${PROJECT_SOURCE_DIR}/library/sql-parser/include
    ${PROJECT_SOURCE_DIR}/library/sql-parser/source
    ${PROJECT_SOURCE_DIR}/library/sql-parser/source/linux
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(wb.mysql.import.grt
    src/wb_mysql_import.cpp
    src/wb_mysql_import_dbd4.cpp
)

target_compile_options(wb.mysql.import.grt PUBLIC ${WB_CXXFLAGS})

target_link_libraries(wb.mysql.import.grt wbscintilla ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${SIGC++_LIBRARIES} ${TINYXML_LIBRARIES} ${MYSQL_LIBRARIES})

set_target_properties(wb.mysql.import.grt
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(wb.mysql.import.grt PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(wb.mysql.import.grt)
endif()

install(TARGETS wb.mysql.import.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
