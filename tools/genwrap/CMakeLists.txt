add_executable(genwrap
    genwrap.cpp
)

target_include_directories(genwrap
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/modules
)

target_include_directories(genwrap
 SYSTEM
  PRIVATE
   ${GLIB_INCLUDE_DIRS}
   ${GRT_INCLUDE_DIRS}
   ${PCRE_INCLUDE_DIRS}
)


target_compile_options(genwrap PRIVATE ${WB_CXXFLAGS})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options(genwrap PRIVATE -fPIE)
else()
  target_compile_options(genwrap PRIVATE -fPIE -pie)
endif()

target_link_libraries(genwrap PUBLIC Boost::boost PRIVATE grt wbbase wbpublic mtemplate ${X11_LIBRARIES} ${PCRE_LIBRARIES} ${MySQL_LIBRARIES})

if(BUILD_FOR_GCOV)
  target_link_libraries(genwrap PRIVATE gcov)
endif()
