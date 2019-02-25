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

target_include_directories(grt
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
)


target_include_directories(grt
 SYSTEM
  PRIVATE
    ${MySQLCppConn_INCLUDE_DIRS}
)


target_compile_options(grt PRIVATE ${WB_CXXFLAGS})

target_link_libraries(grt
  PUBLIC 
    wbbase 
    grt_Iface
    ${LIBXML2_LIBRARIES}
    ${UUID_LIBRARIES}
  PRIVATE 
    ${MySQLCppConn_LIBRARIES}
    ${PYTHON_LIBRARIES}
)

add_library(grt_Iface INTERFACE)
add_library(grt::grt ALIAS grt_Iface)

target_include_directories(grt_Iface INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/diff>
)

target_include_directories(grt_Iface
 SYSTEM INTERFACE
    $<BUILD_INTERFACE:${LIBXML2_INCLUDE_DIR}>
)


set_source_files_properties(python_grtlist.cpp PROPERTIES COMPILE_FLAGS -Wno-pragmas)
if(BUILD_FOR_GCOV)
  target_link_libraries(grt PRIVATE gcov)
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
