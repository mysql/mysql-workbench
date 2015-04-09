include_directories(.
    ${PROJECT_SOURCE_DIR}/frontend/linux
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/backend/wbprivate
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/mysql.canvas/src
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${VSQLITE_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
)

add_library(sqlide
    grid_view_model.cpp
    grid_view.cpp
    recordset_view.cpp
    ../mforms/record_grid_view.cpp
)

target_link_libraries(sqlide ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${CAIRO_LIBRARIES} ${SIGC++_LIBRARIES})

set_target_properties(sqlide
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS sqlide DESTINATION ${WB_INSTALL_LIB_DIR})
