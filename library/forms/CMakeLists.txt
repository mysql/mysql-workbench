add_subdirectory(swig)

set(DATA_FILES
    gtk/embedded_find.glade
)

install(FILES ${DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR})

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
    gtk/src/mforms_acc.cpp
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
    jsonview.cpp
    widgets.cpp
    wizard.cpp
    jsonview.cpp
    home_screen.cpp
    home_screen_connections.cpp
    home_screen_documents.cpp
    home_screen_helpers.cpp
)

target_include_directories(mforms 
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mforms
)

target_include_directories(mforms 
 SYSTEM
  PRIVATE
    ${LIBSECRET_INCLUDE_DIRS}
)


if (KEYRING_DEFINITION)
  target_compile_definitions(mforms PRIVATE HAVE_LIBSECRET_KEYRING)
endif()

target_compile_definitions(mforms PRIVATE $<$<CXX_COMPILER_ID:GNU>:SCI_NAMESPACE> __STDC_FORMAT_MACROS)
target_compile_options(mforms PRIVATE ${WB_CXXFLAGS})

if (HAVE_PRCTL)
  target_compile_definitions(mforms PRIVATE HAVE_PRCTL_H)
endif()


target_link_libraries(mforms
  PUBLIC
    mforms_Iface
  PRIVATE 
    linux_utilities 
    wbpublic::wbpublic
    mdcanvasgtk 
    mdcanvas
    parsers::parsers
    Rapidjson::Rapidjson
)

add_library(mforms_Iface INTERFACE)
add_library(mforms::mforms ALIAS mforms_Iface)
target_include_directories(mforms_Iface INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<$<PLATFORM_ID:Linux>:${PROJECT_SOURCE_DIR}/library/forms/gtk/src>
)

target_link_libraries(mforms_Iface INTERFACE
    ${LIBSECRET_LIBRARIES}
    wbbase
    wbscintilla
)


set_source_files_properties(gtk/src/mforms_acc.cpp PROPERTIES COMPILE_FLAGS -Wno-pragmas)

if(BUILD_FOR_GCOV)
  target_link_libraries(mforms PRIVATE gcov)
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
