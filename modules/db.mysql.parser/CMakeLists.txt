include_directories(.
    ./src
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime/include
    ${PROJECT_SOURCE_DIR}/library/mysql.parser   
)

add_library(db.mysql.parser.grt
    src/mysql_parser_module.cpp
)

set_target_properties(db.mysql.parser.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

# target_link_libraries(db.mysql.parser.grt ${GRT_LIBRARIES} ${GTK2_LIBRARIES} ${SIGC++_LIBRARIES} ${PCRE_LIBRARIES})

install(TARGETS db.mysql.parser.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
