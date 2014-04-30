include_directories(.
    ${GTK2_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/dbc/src
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/modules/db.mysql/src
)

add_library(db.mysql.query.grt
    src/dbquery.cpp
)

set_target_properties(db.mysql.query.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(db.mysql.query.grt ${GRT_LIBRARIES} ${GTK2_LIBRARIES} ${SIGC++_LIBRARIES})

install(TARGETS db.mysql.query.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
