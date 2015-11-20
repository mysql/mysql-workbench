include_directories(.
    ${PROJECT_SOURCE_DIR}/library
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/generated
    ${GRT_INCLUDE_DIRS}
    ${GTK2_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${PCRE_INCLUDE_DIRS}
)

add_executable(genobj
    genobj.cpp
)

SET_TARGET_PROPERTIES(genobj PROPERTIES COMPILE_FLAGS "-fPIE -pie")
target_link_libraries(genobj grt wbbase ${GRT_LIBRARIES} ${PCRE_LIBRARIES})

