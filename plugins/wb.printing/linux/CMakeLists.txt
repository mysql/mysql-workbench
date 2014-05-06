include_directories(.
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/plugins
    ${PROJECT_SOURCE_DIR}/ext
    ${PROJECT_SOURCE_DIR}/library/mysql.canvas/src
    ${PROJECT_SOURCE_DIR}/plugins/wb.printing/backend
    ${PROJECT_SOURCE_DIR}/frontend/linux
    ${PROJECT_SOURCE_DIR}/frontend/linux/linux_utilities
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}
    ${GTK2_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
)

add_library(wb.printing.wbp MODULE
    ../backend/wb_module_printing.cpp
    ../backend/wb_printing.cpp
    wb_printing_linux.cpp
)

set_target_properties(wb.printing.wbp PROPERTIES
#	LINK_FLAGS "-module"
	PREFIX ""
)

target_link_libraries(wb.printing.wbp linux_utilities)

install(TARGETS wb.printing.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
