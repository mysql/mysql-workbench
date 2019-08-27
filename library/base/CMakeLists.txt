add_library(wbbase 
    accessibility.cpp
    util_functions.cpp 
    data_types.cpp
    sqlstring.cpp 
    file_functions.cpp 
    file_utilities.cpp 
    threaded_timer.cpp 
    string_utilities.cpp 
    geometry.cpp 
    notifications.cpp 
    ui_form.cpp 
    config_file.cpp 
    drawing.cpp 
    log.cpp 
    threading.cpp 
    profiling.cpp
    drawing_gtk.cpp
    boost_fix.cpp
    utf8string.cpp
    xml_functions.cpp
    symbol-info.cpp
)

target_include_directories(wbbase 
 SYSTEM
  PRIVATE
    ${GTKMM_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/res

)

target_compile_options(wbbase PRIVATE ${WB_CXXFLAGS})
target_compile_definitions(wbbase PUBLIC __STDC_FORMAT_MACROS)

target_link_libraries(wbbase
  PUBLIC
    wbbase::wbbase
    Rapidjson::Rapidjson
  PRIVATE
    wbprivate::wbprivate
    ${GLIB_LIBRARIES}
    ${GTK3_LIBRARIES}
    ${GTKMM_LIBRARIES}
) 

add_library(wbbase_Iface INTERFACE)
add_library(wbbase::wbbase ALIAS wbbase_Iface)
target_include_directories(wbbase_Iface INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/library/base>
)

target_include_directories(wbbase_Iface 
 SYSTEM INTERFACE
    ${PYTHON_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
)
target_link_libraries(wbbase_Iface
  INTERFACE
    Boost::boost
) 


set_source_files_properties(drawing_gtk.cpp PROPERTIES COMPILE_FLAGS -Wno-pragmas)

if(BUILD_FOR_GCOV)
  target_link_libraries(wbbase PRIVATE gcov)
endif()

if(COMMAND cotire)
set_target_properties(wbbase PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")
cotire(wbbase)
endif()

set_target_properties(wbbase
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS wbbase DESTINATION ${WB_INSTALL_LIB_DIR})

