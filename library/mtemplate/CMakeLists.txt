
include_directories(${PROJECT_SOURCE_DIR}/library/base/base
                   )

add_library(mtemplate
            template.cpp
            dictionary.cpp
            types.cpp
            modifier.cpp
            output.cpp
           )

           
install(TARGETS mtemplate DESTINATION ${WB_INSTALL_LIB_DIR})

