if (GRT_DEFINITIONS)
	add_definitions(${GRT_DEFINITIONS})
endif()
include_directories(. .. ${PROJECT_SOURCE_DIR}/generated ${GRT_INCLUDE_DIRS} ${MYSQLCPPCONN_INCLUDE_DIRS})

#TODO: Set compiler flags 
#set(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} "-Wall -Wsign-compare ${CAIRO_CFLAGS}")

add_library(grt
    grtpp_grt.cpp
    grtpp_helper.cpp
    grtpp_metaclass.cpp
    grtpp_util.cpp
    grtpp_value.cpp
    grtpp_shell.cpp
    grtpp_shell_lua.cpp
    grtpp_shell_lua_help.cpp
    grtpp_module.cpp
    grtpp_module_cpp.cpp
    grtpp_module_lua.cpp
    grtpp_notifications.cpp
    lua_context.cpp
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

target_link_libraries(grt wbbase ${GLIB_LIBRARIES} ${GMODULE_LIBRARIES} ${GRT_LIBRARIES} ${UUID_LIBRARIES})

install(TARGETS grt DESTINATION ${WB_INSTALL_LIB_DIR})
