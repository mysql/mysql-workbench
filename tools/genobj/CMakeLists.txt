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
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set_target_properties(genobj PROPERTIES COMPILE_FLAGS "-fPIE")
else()
  set_target_properties(genobj PROPERTIES COMPILE_FLAGS "-fPIE -pie")
endif()
target_link_libraries(genobj grt wbbase ${GRT_LIBRARIES} ${PCRE_LIBRARIES})

