include_directories(.
    ${GTK2_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${VSQLITE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
)

add_library(dbutils.grt
    src/dbutils.cpp
)

set_target_properties(dbutils.grt PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(dbutils.grt ${GRT_LIBRARIES} ${GTK2_LIBRARIES} ${SIGC++_LIBRARIES})

set_target_properties(dbutils.grt
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS dbutils.grt DESTINATION ${WB_PYTHON_MODULES_DIR})
