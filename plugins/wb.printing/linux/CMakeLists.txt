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
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(wb.printing.wbp MODULE
    ../backend/wb_module_printing.cpp
    ../backend/wb_printing.cpp
    wb_printing_linux.cpp
)

target_compile_options(wb.printing.wbp PUBLIC ${WB_CXXFLAGS})

target_link_libraries(wb.printing.wbp linux_utilities ${GTK3_LIBRARIES} ${SIGC++_LIBRARIES} ${GRT_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(wb.printing.wbp gcov)
endif()

set_target_properties(wb.printing.wbp
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(wb.printing.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(wb.printing.wbp)
endif()

install(TARGETS wb.printing.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
