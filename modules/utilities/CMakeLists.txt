include_directories(.
    ${GTK3_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/mysql.canvas/src
)

add_library(utilities.grt
    src/utilities.cpp
)

set_target_properties(utilities.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(utilities.grt ${GRT_LIBRARIES} ${GDAL_LIBRARIES})

set_target_properties(utilities.grt
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS utilities.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
