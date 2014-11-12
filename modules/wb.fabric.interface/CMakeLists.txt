include_directories(.
    ${GTK2_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${TINYXML_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/modules
)

add_library(wb.mysqlfabric.interface.grt
    wb_fabric_interface.cpp
)

set_target_properties(wb.mysqlfabric.interface.grt
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS wb.mysqlfabric.interface.grt DESTINATION ${WB_INSTALL_LIB_DIR}/modules)
