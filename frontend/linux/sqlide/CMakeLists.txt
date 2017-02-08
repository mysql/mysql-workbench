include_directories(.
    ${PROJECT_SOURCE_DIR}/frontend/linux
    ${PROJECT_SOURCE_DIR}/frontend/linux/linux_utilities
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/backend/wbprivate
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/mysql.canvas/src
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(sqlide
    grid_view_model.cpp
    grid_view.cpp
    recordset_view.cpp
    ../mforms/record_grid_view.cpp
)

target_compile_options(sqlide PUBLIC ${WB_CXXFLAGS})

target_link_libraries(sqlide ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${CAIRO_LIBRARIES} ${SIGC++_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(sqlide gcov)
endif()

set_target_properties(sqlide
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(sqlide PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(sqlide)
endif()
install(TARGETS sqlide DESTINATION ${WB_INSTALL_LIB_DIR})
