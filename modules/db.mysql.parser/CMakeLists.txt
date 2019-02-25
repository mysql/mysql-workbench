add_library(db.mysql.parser.grt
    src/mysql_parser_module.cpp
    src/ObjectListeners.cpp
)

target_include_directories(db.mysql.parser.grt
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
)

target_include_directories(db.mysql.parser.grt
 SYSTEM
  PRIVATE
    ${LIBXML2_INCLUDE_DIR}
    ${GLIB_INCLUDE_DIRS}
)


set_source_files_properties(src/mysql_parser_module.cpp PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)
set_source_files_properties(src/ObjectListeners.cpp PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)
target_compile_options(db.mysql.parser.grt PRIVATE ${WB_CXXFLAGS})

target_link_libraries(db.mysql.parser.grt 
  PRIVATE 
   parsers
   wbbase::wbbase
   wbpublic::wbpublic
   grt::grt
)
add_dependencies(db.mysql.parser.grt parsers)

set_target_properties(db.mysql.parser.grt PROPERTIES
	PREFIX ""
  VERSION   ${WB_VERSION}
  SOVERSION ${WB_VERSION}
)

if(BUILD_FOR_GCOV)
  target_link_libraries(db.mysql.parser.grt PRIVATE gcov)
endif()

if(COMMAND cotire)
    set_target_properties(db.mysql.parser.grt PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.mysql.parser.grt)
endif()

install(TARGETS db.mysql.parser.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
