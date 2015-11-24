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

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set_target_properties(genwrap PROPERTIES COMPILE_FLAGS "-fPIE")
else()
  set_target_properties(genwrap PROPERTIES COMPILE_FLAGS "-fPIE -pie")
endif()

target_link_libraries(genwrap grt wbbase wbpublic ${X11_LIBRARIES} ${PCRE_LIBRARIES} ${MYSQL_LIBRARIES} ${CTemplate_LIBRARIES})
