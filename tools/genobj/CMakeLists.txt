add_executable(genobj
    genobj.cpp
)

target_include_directories(genobj
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
)

target_include_directories(genobj
 SYSTEM
  PRIVATE
    ${PROJECT_SOURCE_DIR}/generated
    ${GTK3_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GTKMM_INCLUDE_DIRS}
    ${SIGC++_INCLUDE_DIRS}
    ${PCRE_INCLUDE_DIRS}
)


target_compile_options(genobj PRIVATE ${WB_CXXFLAGS})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options(genobj PRIVATE -fPIE)
else()
  target_compile_options(genobj PRIVATE -fPIE -pie)
endif()

target_link_libraries(genobj PUBLIC Boost::boost PRIVATE grt wbbase ${GTKMM_LIBRARIES} ${GTK3_LIBRARIES} ${GLIB_LIBRARIES} ${PCRE_LIBRARIES})

if(BUILD_FOR_GCOV)
  target_link_libraries(genobj PRIVATE gcov)
endif()
