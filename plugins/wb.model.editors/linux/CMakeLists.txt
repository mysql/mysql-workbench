set(MODULE_DATA_FILES
    res/editor_note.glade
    res/editor_storednote.glade
    res/editor_layer.glade
    res/editor_image.glade
)

install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data)

include_directories(.
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/mysql.parser
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/plugins
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime/include
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime
    ${PROJECT_SOURCE_DIR}/frontend/linux
    ${PROJECT_SOURCE_DIR}/frontend/linux/linux_utilities
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}
    ${GTK3_INCLUDE_DIRS}
#     ${CAIRO_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
)

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

add_dependencies(wb.model.editors.wbp mysqlparser)

target_link_libraries(wb.model.editors.wbp linux_utilities ${SIGC++_LIBRARIES} ${GRT_LIBRARIES})

set_target_properties(wb.model.editors.wbp
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS wb.model.editors.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
