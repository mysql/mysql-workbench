include_directories(.
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/grt/src
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/generated
    SYSTEM ${GRT_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIRS}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_executable(genwrap
    genwrap.cpp
)

target_compile_options(genwrap PUBLIC ${WB_CXXFLAGS})
target_compile_options(genwrap PUBLIC $<$<STREQUAL:${CMAKE_CXX_COMPILER_ID}, CLANG>:-fPIE>)
target_compile_options(genwrap PUBLIC $<$<NOT:$<STREQUAL:${CMAKE_CXX_COMPILER_ID}, CLANG>>:-fPIE -pie>)

target_link_libraries(genwrap grt wbbase wbpublic ${X11_LIBRARIES} ${PCRE_LIBRARIES} ${MySQL_LIBRARIES} ${CTemplate_LIBRARIES})
