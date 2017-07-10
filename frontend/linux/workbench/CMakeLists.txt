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
    workbench.css
)
install(FILES ${WORKBENCH_DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR})

set(libdir          ${LIB_INSTALL_DIR})
set(utilities_cmd   "")
configure_file(mysql-workbench.in mysql-workbench @ONLY)
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
    ${PROJECT_SOURCE_DIR}/library/parsers
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${LIBZIP_INCLUDE_DIRS}
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${GNOME_KEYRING_INCLUDE_DIRS}
    SYSTEM ${VSQLITE_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)


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


target_compile_options(mysql-workbench-bin PUBLIC ${WB_CXXFLAGS})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options(mysql-workbench-bin PUBLIC -fPIE)
else()
  target_compile_options(mysql-workbench-bin PUBLIC -fPIE -pie)
endif()

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
    parsers
    mtemplate
    ${MySQLCppConn_LIBRARIES}
    ${X11_LIBRARIES}
    ${GTK3_LIBRARIES}
    ${SIGC++_LIBRARIES}
    ${GRT_LIBRARIES}
    ${PCRE_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${LIBZIP_LIBRARIES}
    ${MySQL_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${GNOME_KEYRING_LIBRARIES}
)

if(BUILD_FOR_TESTS)
  target_link_libraries(mysql-workbench-bin gcov)
endif()

if(COMMAND cotire)
  set_target_properties(mysql-workbench-bin PROPERTIES
    COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")
  cotire(mysql-workbench-bin)
endif()

install(TARGETS mysql-workbench-bin DESTINATION ${WB_INSTALL_DIR_EXECUTABLE})
