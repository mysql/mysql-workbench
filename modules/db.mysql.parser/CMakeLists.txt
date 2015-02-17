include_directories(${LIBXML2_INCLUDE_DIR}
    .
    ./src
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime/include
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime 
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/mysql.parser   
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/generated
)

add_library(db.mysql.parser.grt
    src/mysql_parser_module.cpp
)

add_dependencies(db.mysql.parser.grt mysqlparser)

set_target_properties(db.mysql.parser.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
  VERSION   ${WB_VERSION}
  SOVERSION ${WB_VERSION}
)



# target_link_libraries(db.mysql.parser.grt ${GRT_LIBRARIES} ${GTK2_LIBRARIES} ${SIGC++_LIBRARIES} ${PCRE_LIBRARIES})

install(TARGETS db.mysql.parser.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
