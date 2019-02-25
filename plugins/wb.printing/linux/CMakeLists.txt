add_library(wb.printing.wbp MODULE
    ../backend/wb_module_printing.cpp
    ../backend/wb_printing.cpp
    wb_printing_linux.cpp
)

target_include_directories(wb.printing.wbp
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend
    ${PROJECT_SOURCE_DIR}/modules
)

target_compile_options(wb.printing.wbp PRIVATE ${WB_CXXFLAGS})

target_link_libraries(wb.printing.wbp 
  PRIVATE 
    linux_utilities
    wbbase
    mdcanvas
    grt
    wbpublic
)

if(BUILD_FOR_GCOV)
  target_link_libraries(wb.printing.wbp PRIVATE gcov)
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
