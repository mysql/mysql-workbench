
include_directories(${PROJECT_SOURCE_DIR}/library/base
                    SYSTEM ${Boost_INCLUDE_DIRS}
                   )

add_library(mtemplate
            template.cpp
            dictionary.cpp
            types.cpp
            modifier.cpp
            output.cpp
           )

target_compile_options(mtemplate PUBLIC ${WB_CXXFLAGS})

if(BUILD_FOR_TESTS)
  target_link_libraries(mtemplate gcov)
endif()

set_target_properties(mtemplate
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})
           
install(TARGETS mtemplate DESTINATION ${WB_INSTALL_LIB_DIR})

