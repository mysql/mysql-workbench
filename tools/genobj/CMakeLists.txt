include_directories(.
    ${PROJECT_SOURCE_DIR}/library
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/generated
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_executable(genobj
    genobj.cpp
)

target_compile_options(genobj PUBLIC ${WB_CXXFLAGS})
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options(genobj PRIVATE -fPIE)
else()
  target_compile_options(genobj PRIVATE -fPIE -pie)
endif()

target_link_libraries(genobj grt wbbase ${GRT_LIBRARIES} ${PCRE_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(genobj gcov)
endif()
