include_directories(.
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/generated
    ${GRT_INCLUDE_DIRS}
    ${PCRE_INCLUDE_DIRS}
)

add_executable(genwrap
    genwrap.cpp
)

target_link_libraries(genwrap grt wbbase wbpublic ${X11_LIBRARIES} ${PCRE_LIBRARIES} ${MYSQL_LIBRARIES} ${CTEMPLATE_LIBRARIES})

