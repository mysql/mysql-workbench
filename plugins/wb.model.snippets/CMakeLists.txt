add_library(wb.model.snippets.wbp MODULE
    include_model_wizard.cpp
    merge_model.cpp
    register_plugin.cpp
)

include_directories(wb.model.snippets.wbp
  PRIVATE
    ${PROJECT_SOURCE_DIR}/modules/
    ../backend
)

target_compile_options(wb.model.snippets.wbp PRIVATE ${WB_CXXFLAGS})

target_link_libraries(wb.model.snippets.wbp 
  PRIVATE
    wbbase
    grt
    mforms
    wbpublic
)

if(BUILD_FOR_GCOV)
  target_link_libraries(wb.model.snippets.wbp PRIVATE gcov)
endif()

set_target_properties(wb.model.snippets.wbp
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(wb.model.snippets.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(wb.model.snippets.wbp)
endif()
install(TARGETS wb.model.snippets.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
