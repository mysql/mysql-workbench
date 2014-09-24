
# Generate the parser files during configure
message("Generating mysql.parser files...")
execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/build-parser"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RESULT_VARIABLE res_var 
)

if(NOT "${res_var}" STREQUAL "0")
  message(FATAL_ERROR "Generating mysql.parser files failed with result='${res_var}'")
endif()

# Update the parser files when needed
add_custom_target(generate_parser
                  ALL
                  "${CMAKE_CURRENT_SOURCE_DIR}/build-parser"
                  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                  COMMENT "Checking if we should regenerate mysql.parser files..."
)

set(DATA_FILES 
    MySQL.g
)

install(FILES ${DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/data)
