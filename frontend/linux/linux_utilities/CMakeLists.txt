if (CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-rdynamic)
endif()

include_directories(.
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/frontend/linux/workbench
    ${PROJECT_SOURCE_DIR}/frontend/linux/sqlide
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/ext
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime/include
    ${PROJECT_SOURCE_DIR}/ext/antlr-runtime
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/forms/gtk
    ${PROJECT_SOURCE_DIR}/library/forms/gtk/src
    ${PROJECT_SOURCE_DIR}/generated/grti
    ${PROJECT_SOURCE_DIR}/library/forms/mforms
    ${PROJECT_SOURCE_DIR}/library/mysql.parser
    ${GTK3_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
)

add_library(linux_utilities
    listmodel_wrapper.cpp
    form_view_base.cpp
    treemodel_wrapper.cpp
    plugin_editor_base.cpp
    image_cache.cpp
    gtk_helpers.cpp
    multi_view.cpp
    editable_iconview.cpp
    auto_completable.cpp
    notebook_dockingpoint.cpp
    widgets_auto_cleaner.cpp
    widget_saver.cpp
)

target_link_libraries(linux_utilities ${GTK3_LIBRARIES} ${CAIRO_LIBRARIES} ${GRT_LIBRARIES} ${SIGC++_LIBRARIES})

set_target_properties(linux_utilities
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS linux_utilities DESTINATION ${WB_INSTALL_LIB_DIR})
