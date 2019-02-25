set(MODULE_DATA_FILES
    res/editor_note.glade
    res/editor_storednote.glade
    res/editor_layer.glade
    res/editor_image.glade
)

install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data)

add_library(wb.model.editors.wbp MODULE
    ../backend/register_plugin.cpp
    ../backend/wb_editor_image.cpp
    ../backend/wb_editor_note.cpp
    ../backend/wb_editor_layer.cpp
    ../backend/wb_editor_storednote.cpp
    editor_note_fe.cpp
    editor_image_fe.cpp
    editor_layer_fe.cpp
    editor_stored_note_fe.cpp
)

target_include_directories(wb.model.editors.wbp
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend
    ${PROJECT_SOURCE_DIR}/modules
)


target_compile_options(wb.model.editors.wbp PRIVATE ${WB_CXXFLAGS})

add_dependencies(wb.model.editors.wbp parsers)

target_link_libraries(wb.model.editors.wbp 
  PRIVATE
    linux_utilities
    wbbase
    grt
    mforms
    wbpublic
    parsers
  )

if(BUILD_FOR_GCOV)
  target_link_libraries(wb.model.editors.wbp PRIVATE gcov)
endif()

set_target_properties(wb.model.editors.wbp
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(wb.model.editors.wbp PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(wb.model.editors.wbp)
endif()

install(TARGETS wb.model.editors.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
