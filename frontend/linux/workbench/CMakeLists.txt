if (CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-rdynamic)
endif()

set(WORKBENCH_DATA_FILES
    res/wb.glade
    res/model_view.glade
    res/diagram_view.glade
    res/find.glade
    res/diagram_size_form.glade
    res/input_dialog.glade
    workbench.rc
)
install(FILES ${WORKBENCH_DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR})

set(libdir          ${LIB_INSTALL_DIR})
set(utilities_cmd   "")
configure_file(mysql-workbench.in mysql-workbench)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mysql-workbench DESTINATION ${WB_INSTALL_BIN_DIR})

include_directories(.
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbprivate/workbench
    ${PROJECT_SOURCE_DIR}/frontend/linux/linux_utilities
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/backend/wbpublic/grt
    ${PROJECT_SOURCE_DIR}/backend/wbprivate
    ${PROJECT_SOURCE_DIR}/frontend/linux/
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/library/cdbc/src
    ${PROJECT_SOURCE_DIR}/library/forms/gtk
    ${PROJECT_SOURCE_DIR}/library/forms/gtk/src
    ${PROJECT_SOURCE_DIR}/generated/grti
    ${PROJECT_SOURCE_DIR}/modules/wb.model/src
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/ext
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/library/forms/mforms
    ${PROJECT_SOURCE_DIR}/library/mysql.canvas/src
    ${PROJECT_SOURCE_DIR}/library/mysql.parser
    ${GTK2_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${LIBZIP_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${GNOME_KEYRING_INCLUDE_DIRS}
    ${VSQLITE_INCLUDE_DIRS}
)

#link_directories(${PROJECT_BINARY_DIR}/mysqlcppconnector/lib ${PROJECT_BINARY_DIR}/antlr-wb)
SET(CMAKE_INSTALL_RPATH "${WB_INSTALL_LIB_DIR}")
add_executable(mysql-workbench-bin
    main.cpp
    program.cpp
    main_form.cpp
    overview_panel.cpp
    model_panel.cpp
    model_diagram_panel.cpp
    properties_tree.cpp
    navigator_box.cpp
    info_box.cpp
    documentation_box.cpp
    diagram_size_form.cpp
    validation_panel.cpp
    sqlide_form.cpp
    sqlide_output_view.cpp
    sqlide_main.cpp
    ../../common/new_server_instance_wizard.cpp
    ../../common/grt_code_editor.cpp
    ../../common/server_instance_editor.cpp
    ../../common/grt_python_debugger.cpp
    ../../common/advanced_sidebar.cpp
    ../../common/snippet_popover.cpp
    ../../common/grt_plugin_wizard.cpp
)

SET_TARGET_PROPERTIES(mysql-workbench-bin PROPERTIES COMPILE_FLAGS -fPIC)

if (KEYRING_DEFINITION)
	set_target_properties(mysql-workbench-bin PROPERTIES
		COMPILE_DEFINITIONS "${KEYRING_DEFINITION}"
	)
endif()

target_link_libraries(mysql-workbench-bin
    mforms
    linux_utilities
    sqlide
    wbprivate
    wbpublic
    wbbase
    grt
    cdbc
    wbscintilla
    mdcanvasgtk
    mdcanvas
    mysqlparser
    ${MYSQLCPPCONN_LIBRARIES}
    ${CTemplate_LIBRARIES}
    ${X11_LIBRARIES}
    ${GTK2_LIBRARIES}
    ${SIGC++_LIBRARIES}
    ${GRT_LIBRARIES}
    ${PCRE_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${LIBZIP_LIBRARIES}
    ${MYSQL_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${GNOME_KEYRING_LIBRARIES}
)

install(TARGETS mysql-workbench-bin DESTINATION ${WB_INSTALL_DIR_EXECUTABLE})
