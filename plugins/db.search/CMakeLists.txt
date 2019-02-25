add_library(db.search.wbp MODULE
    DbSearchFilterPanel.cpp
    DbSearchPanel.cpp
    register_plugin.cpp
)

target_include_directories(db.search.wbp 
  PRIVATE
    ${PROJECT_SOURCE_DIR}/modules/
)

target_compile_options(db.search.wbp PRIVATE ${WB_CXXFLAGS})

set_target_properties(db.search.wbp PROPERTIES
  PREFIX ""
  VERSION   ${WB_VERSION}
  SOVERSION ${WB_VERSION}
)

target_link_libraries(db.search.wbp
  PRIVATE
    wbbase
    wbpublic
    mforms
    cdbc
    grt
)

if(BUILD_FOR_GCOV)
  target_link_libraries(db.search.wbp PRIVATE gcov)
endif()

if(COMMAND cotire)
    set_target_properties(db.search.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(db.search.wbp)
endif()
install(TARGETS db.search.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
