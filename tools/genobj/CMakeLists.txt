include_directories(.
    ${PROJECT_SOURCE_DIR}/library
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/generated
    ${GRT_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_executable(genobj
    genobj.cpp
)

target_compile_options(genobj PUBLIC ${WB_CXXFLAGS})
target_compile_options(genobj PUBLIC "$<$<STREQUAL:${CMAKE_CXX_COMPILER_ID}, CLANG>:-fPIE>")
target_compile_options(genobj PUBLIC $<$<NOT:$<STREQUAL:${CMAKE_CXX_COMPILER_ID}, CLANG>>:-fPIE -pie>)

target_link_libraries(genobj grt wbbase ${GRT_LIBRARIES} ${PCRE_LIBRARIES})

