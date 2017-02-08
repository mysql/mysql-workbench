if (GRT_DEFINITIONS)
	add_definitions(${GRT_DEFINITIONS})
endif()
include_directories(. 
    .. 
    ${PROJECT_SOURCE_DIR}/generated 
    SYSTEM ${GRT_INCLUDE_DIRS} 
    SYSTEM ${MySQLCppConn_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS})

#TODO: Set compiler flags 
#set(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} "-Wall -Wsign-compare ${CAIRO_CFLAGS}")

add_library(grt
    grt.cpp
    grtpp_helper.cpp
    grtpp_metaclass.cpp
    grtpp_util.cpp
    grtpp_value.cpp
    grtpp_shell.cpp
    grtpp_module.cpp
    grtpp_module_cpp.cpp
    grtpp_notifications.cpp
    serializer.cpp
    unserializer.cpp
    grtpp_undo_manager.cpp
    diff/changefactory.cpp
    diff/changelistobjects.cpp
    diff/diffchange.cpp
    diff/grtdiff.cpp
    diff/grtlistdiff.cpp
    grtpp_module_python.cpp
    grtpp_shell_python.cpp
    grtpp_shell_python_help.cpp
    python_context.cpp
    python_grtdict.cpp
    python_grtlist.cpp
    python_grtobject.cpp
    python_module.cpp
)

target_compile_options(grt PUBLIC ${WB_CXXFLAGS})

target_link_libraries(grt wbbase ${GLIB_LIBRARIES} ${GMODULE_LIBRARIES} ${GRT_LIBRARIES} ${UUID_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(grt gcov)
endif()

set_target_properties(grt
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(grt PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(grt)
endif()

install(TARGETS grt DESTINATION ${WB_INSTALL_LIB_DIR})
