include_directories(.
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${GDAL_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/mysql.canvas/src
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(utilities.grt
    src/utilities.cpp
)

target_compile_options(utilities.grt PUBLIC ${WB_CXXFLAGS})

set_target_properties(utilities.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(utilities.grt ${GRT_LIBRARIES} ${GDAL_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(utilities.grt gcov)
endif()

set_target_properties(utilities.grt
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS utilities.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
