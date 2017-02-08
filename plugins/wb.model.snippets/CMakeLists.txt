include_directories(.
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/ext
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/modules/db.mysql/src
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/modules/
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/forms/mforms
    ${PROJECT_SOURCE_DIR}/plugins
    ../backend
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(wb.model.snippets.wbp MODULE
    include_model_wizard.cpp
    merge_model.cpp
    register_plugin.cpp
)

target_compile_options(wb.model.snippets.wbp PUBLIC ${WB_CXXFLAGS})

target_link_libraries(wb.model.snippets.wbp wbscintilla)

if(BUILD_FOR_TESTS)
  target_link_libraries(wb.model.snippets.wbp gcov)
endif()

set_target_properties(wb.model.snippets.wbp
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(wb.model.snippets.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(wb.model.snippets.wbp)
endif()
install(TARGETS wb.model.snippets.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
