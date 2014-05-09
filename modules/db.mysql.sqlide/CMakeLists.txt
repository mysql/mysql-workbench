set(MODULE_DATA_FILES
    res/com.mysql.rdbms.mysql.sql.editor.xml
)
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})

include_directories(.
    ${GTK2_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/sql-parser/include
    ${PROJECT_SOURCE_DIR}/library/sql-parser/source
    ${PROJECT_SOURCE_DIR}/library/sql-parser/source/linux
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/backend/grtdb
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/images/ui
)

add_library(db.mysql.sqlide.grt
    src/mysql_sql_editor_be.cpp
    src/db.mysql.sqlide.cpp
)

set_target_properties(db.mysql.sqlide.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(db.mysql.sqlide.grt ${GRT_LIBRARIES} ${GTK2_LIBRARIES} ${SIGC++_LIBRARIES} ${MYSQL_LIBRARIES})

install(TARGETS db.mysql.sqlide.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
