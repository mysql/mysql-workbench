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
    ${PROJECT_SOURCE_DIR}/library/parsers
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${Boost_INCLUDE_DIRS}
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
    main_app.cpp
)

target_compile_options(linux_utilities PUBLIC ${WB_CXXFLAGS})

target_link_libraries(linux_utilities ${GTK3_LIBRARIES} ${CAIRO_LIBRARIES} ${GRT_LIBRARIES} ${SIGC++_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(linux_utilities gcov)
endif()

set_target_properties(linux_utilities
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(linux_utilities PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(linux_utilities)
endif()

install(TARGETS linux_utilities DESTINATION ${WB_INSTALL_LIB_DIR})
