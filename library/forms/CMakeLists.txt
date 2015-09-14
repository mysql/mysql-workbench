add_subdirectory(swig)

set(DATA_FILES
    gtk/embedded_find.glade
)

install(FILES ${DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR})

include_directories(.
    ${CAIRO_INCLUDE_DIRS}
    ${GTK2_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${GNOME_KEYRING_INCLUDE_DIRS}
    ${PCRE_INCLUDE_DIRS}
    ${TINYXML_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${VSQLITE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/library
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/mysql.canvas/src
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/forms/mforms
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
if (UNIX)
    ${PROJECT_SOURCE_DIR}/frontend/linux/linux_utilities
endif()
)

add_library(mforms
    app.cpp
    appview.cpp
    base.cpp
    box.cpp
    button.cpp
    canvas.cpp
    checkbox.cpp
    code_editor.cpp
    container.cpp
    dockingpoint.cpp
    drawbox.cpp
    filechooser.cpp
    find_panel.cpp
    form.cpp
    fs_object_selector.cpp
    gtk/src/active_label.cpp
    gtk/src/lf_box.cpp
    gtk/src/lf_button.cpp
    gtk/src/lf_code_editor.cpp
    gtk/src/lf_drawbox.cpp
    gtk/src/lf_find_panel.cpp
    gtk/src/lf_form.cpp
    gtk/src/lf_hypertext.cpp
    gtk/src/lf_label.cpp
    gtk/src/lf_listbox.cpp
    gtk/src/lf_menu.cpp
    gtk/src/lf_menubar.cpp
    gtk/src/lf_mforms.cpp
    gtk/src/lf_native.cpp
    gtk/src/lf_panel.cpp
    gtk/src/lf_popover.cpp
    gtk/src/lf_popup.cpp
    gtk/src/lf_radiobutton.cpp
    gtk/src/lf_scrollpanel.cpp
    gtk/src/lf_selector.cpp
    gtk/src/lf_splitter.cpp
    gtk/src/lf_tabview.cpp
    gtk/src/lf_textbox.cpp
    gtk/src/lf_textentry.cpp
    gtk/src/lf_table.cpp
    gtk/src/lf_toolbar.cpp
    gtk/src/lf_treenodeview.cpp
    gtk/src/lf_utilities.cpp
    gtk/src/lf_view.cpp
    gtk/src/lf_wizard.cpp
    hypertext.cpp
    imagebox.cpp
    label.cpp
    listbox.cpp
    menu.cpp
    menubar.cpp
    mforms.cpp
    native.cpp
    panel.cpp
    password_cache.cpp
    popover.cpp
    popup.cpp
    progressbar.cpp
    radiobutton.cpp
    record_grid.cpp
    scrollpanel.cpp
    sectionbox.cpp
    selector.cpp
    simpleform.cpp
    splitter.cpp
    table.cpp
    tabswitcher.cpp
    tabview.cpp
    tabview_dock.cpp
    task_sidebar.cpp
    textbox.cpp
    textentry.cpp
    toolbar.cpp
    treenodeview.cpp
    utilities.cpp
    view.cpp
    webbrowser.cpp
    jsonview.cpp
    widgets.cpp
    wizard.cpp
)

if (KEYRING_DEFINITION)
	set_target_properties(mforms PROPERTIES
		COMPILE_DEFINITIONS "${KEYRING_DEFINITION}"
	)
endif()
if (CMAKE_COMPILER_IS_GNUCXX)
	set_target_properties(mforms PROPERTIES
	  COMPILE_FLAGS "-DSCI_NAMESPACE"
	)
endif()

target_link_libraries(mforms linux_utilities mdcanvasgtk mdcanvas ${CAIRO_LIBRARIES} ${GTK2_LIBRARIES} ${GNOME_KEYRING_LIBRARIES} ${TINYXML_LIBRARY})

set_target_properties(mforms
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS mforms DESTINATION ${WB_INSTALL_LIB_DIR})
