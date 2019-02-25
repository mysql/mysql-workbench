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

target_include_directories(linux_utilities
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
)



target_compile_options(linux_utilities PRIVATE $<$<CXX_COMPILER_ID:GNU>:-rdynamic>)
target_compile_options(linux_utilities PRIVATE ${WB_CXXFLAGS})

target_link_libraries(linux_utilities 
  PUBLIC 
    linux_utilities_Iface
  PRIVATE
    wbbase::wbbase
    wbpublic::wbpublic
    mforms::mforms
    parsers::parsers
    wbscintilla::wbscintilla
)

add_library(linux_utilities_Iface INTERFACE)
add_library(linux_utilities::linux_utilities ALIAS linux_utilities_Iface)

target_include_directories(linux_utilities_Iface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(linux_utilities_Iface
 SYSTEM INTERFACE
    ${GTKMM_INCLUDE_DIRS}
)

target_link_libraries(linux_utilities_Iface INTERFACE grt)



if(BUILD_FOR_GCOV)
  target_link_libraries(linux_utilities PRIVATE gcov)
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
