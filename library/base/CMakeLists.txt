
add_library(wbbase util_functions.cpp sqlstring.cpp file_functions.cpp file_utilities.cpp threaded_timer.cpp string_utilities.cpp geometry.cpp notifications.cpp ui_form.cpp config_file.cpp drawing.cpp log.cpp threading.cpp profiling.cpp)

target_link_libraries(wbbase ${CAIRO_LIBRARIES} ${GLIB_LIBRARIES} ${GTHREAD_LIBRARIES} ${PCRE_LIBRARIES} ${PYTHON_LIBRARIES})


set_target_properties(wbbase
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

install(TARGETS wbbase DESTINATION ${WB_INSTALL_LIB_DIR})

