add_library(mtemplate
            template.cpp
            dictionary.cpp
            types.cpp
            modifier.cpp
            output.cpp
           )

target_include_directories(mtemplate
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_options(mtemplate PRIVATE ${WB_CXXFLAGS})

target_link_libraries(mtemplate PRIVATE wbbase ${GLIB_LIBRARIES})
if(BUILD_FOR_GCOV)
  target_link_libraries(mtemplate PRIVATE gcov)
endif()

set_target_properties(mtemplate
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})
           
install(TARGETS mtemplate DESTINATION ${WB_INSTALL_LIB_DIR})

