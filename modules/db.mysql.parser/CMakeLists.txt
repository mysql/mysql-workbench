include_directories(${LIBXML2_INCLUDE_DIR}
    .
    ./src
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime/include
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime 
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/parsers   
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/generated
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(db.mysql.parser.grt
    src/mysql_parser_module.cpp
)

target_compile_options(db.mysql.parser.grt PUBLIC ${WB_CXXFLAGS})

add_dependencies(db.mysql.parser.grt parsers)

set_target_properties(db.mysql.parser.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
  VERSION   ${WB_VERSION}
  SOVERSION ${WB_VERSION}
)



# target_link_libraries(db.mysql.parser.grt ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${SIGC++_LIBRARIES} ${PCRE_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(db.mysql.parser.grt gcov)
endif()

if(COMMAND cotire)
    set_target_properties(db.mysql.parser.grt PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.mysql.parser.grt)
endif()

install(TARGETS db.mysql.parser.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
