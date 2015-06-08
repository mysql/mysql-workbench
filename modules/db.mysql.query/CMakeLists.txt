include_directories(.
    ${GTK3_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/modules/db.mysql/src
    ${PROJECT_SOURCE_DIR}/library/cdbc/src
)

add_library(db.mysql.query.grt
    src/dbquery.cpp
)

target_link_libraries(db.mysql.query.grt grt ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${SIGC++_LIBRARIES} ${MYSQLCPPCONN_LIBRARIES})

set_target_properties(db.mysql.query.grt
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS db.mysql.query.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
