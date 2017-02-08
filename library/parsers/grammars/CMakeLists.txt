
# Generate the parser files during configure
message("[mysql] Generating parser files...")
execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/build-parsers" mysql
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RESULT_VARIABLE res_var 
)

if(NOT "${res_var}" STREQUAL "0")
  message(FATAL_ERROR "[mysql] Generating parsers files failed with result='${res_var}'")
endif()

# Update the parser files when needed
add_custom_target(generate_parser_mysql
                  ALL
                  "${CMAKE_CURRENT_SOURCE_DIR}/build-parsers" mysql
                  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                  COMMENT "[mysql] Checking if we should regenerate parser files..."
)

set(DATA_FILES 
    MySQL.g
    # JS.g
    # Python2.g
    # Python3.g
)

install(FILES ${DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/data)
