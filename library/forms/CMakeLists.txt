add_subdirectory(swig)

set(DATA_FILES
    gtk/embedded_find.glade
)

install(FILES ${DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR})

include_directories(.
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${GNOME_KEYRING_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIRS}
    SYSTEM ${PROJECT_SOURCE_DIR}
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
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
    SYSTEM ${Boost_INCLUDE_DIRS}
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
    gtk/src/lf_treeview.cpp
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
    gridview.cpp
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
    treeview.cpp
    utilities.cpp
    view.cpp
    webbrowser.cpp
    jsonview.cpp
    widgets.cpp
    wizard.cpp
    jsonview.cpp
    home_screen.cpp
    home_screen_connections.cpp
    home_screen_documents.cpp
    home_screen_launchers.cpp
    home_screen_helpers.cpp
    ProjectForm.cpp
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

target_compile_options(mforms PUBLIC ${WB_CXXFLAGS})

target_link_libraries(mforms linux_utilities mdcanvasgtk mdcanvas wbscintilla ${CAIRO_LIBRARIES} ${GTK3_LIBRARIES} ${GNOME_KEYRING_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(mforms gcov)
endif()

set_target_properties(mforms
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(mforms PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(mforms)
endif()

install(TARGETS mforms DESTINATION ${WB_INSTALL_LIB_DIR})
