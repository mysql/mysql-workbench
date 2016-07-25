include_directories(.
    ${Boost_INCLUDE_DIRS}
    ${MYSQLNG_INCLUDE_DIR}
)

add_library(wbbase 
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
    jsonparser.cpp
    drawing_gtk.cpp
    boost_fix.cpp
    )

target_link_libraries(wbbase ${CAIRO_LIBRARIES} ${GLIB_LIBRARIES} ${MySQL_LIBRARIES} ${MYSQLNG_LIBRARY} ${GTHREAD_LIBRARIES} ${PCRE_LIBRARIES} ${PYTHON_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SSL_LIBRARIES} ${GTK3_LIBRARIES})

if(COMMAND cotire)
set_target_properties(wbbase PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")
cotire(wbbase)
endif()

set_target_properties(wbbase
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS wbbase DESTINATION ${WB_INSTALL_LIB_DIR})

