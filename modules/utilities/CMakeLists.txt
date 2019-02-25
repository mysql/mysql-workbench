add_library(utilities.grt
    src/utilities.cpp
)

target_include_directories(utilities.grt
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
)

target_include_directories(utilities.grt
 SYSTEM
  PRIVATE
   ${GDAL_INCLUDE_DIRS}
)

target_compile_options(utilities.grt PRIVATE ${WB_CXXFLAGS})

set_target_properties(utilities.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(utilities.grt PRIVATE grt wbpublic mdcanvas ${GDAL_LIBRARIES})

if(BUILD_FOR_GCOV)
  target_link_libraries(utilities.grt PRIVATE gcov)
endif()

set_target_properties(utilities.grt
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS utilities.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
