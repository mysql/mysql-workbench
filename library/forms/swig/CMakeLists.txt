execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
                OUTPUT_VARIABLE GCC_VERSION)
if (GCC_VERSION VERSION_GREATER 4.6 OR GCC_VERSION VERSION_EQUAL 4.6)
  add_definitions(-Wno-unused-but-set-variable)
endif()
if (GCC_VERSION)
  add_definitions(-Wno-error)
endif()

set(MODULE_DATA_FILES
    ${CMAKE_BINARY_DIR}/library/forms/swig/mforms.py
    ${CMAKE_BINARY_DIR}/library/forms/swig/cairo.py
)

install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})

include_directories(.
    SYSTEM ${CAIRO_INCLUDEDIR}
    SYSTEM ${CAIRO_INCLUDE_DIRS}
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${LIBXML2_INCLUDE_DIR}
    SYSTEM ${SIGC++_INCLUDE_DIRS}
    SYSTEM ${PYTHON_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    SYSTEM ${Boost_INCLUDE_DIRS}
)

set(CMAKE_SWIG_FLAGS -w312)
set_source_files_properties(cairo.i PROPERTIES CPLUSPLUS ON)
set_property(SOURCE cairo.i PROPERTY SWIG_FLAGS -DCAIRO_HAS_PNG_FUNCTIONS=1 -fcompact -DSWIG_PYTHON_LEGACY_BOOL)
swig_add_module(cairo python cairo.i)
swig_link_libraries(cairo ${CAIRO_LIBRARIES})

set(SOURCE_FILE_COMPILE_FLAGS "-Wno-missing-field-initializers -Wno-parentheses")
if(CMAKE_COMPILER_IS_GNUCXX)
  set(SOURCE_FILE_COMPILE_FLAGS "${SOURCE_FILE_COMPILE_FLAGS} -Wno-maybe-uninitialized")
endif()

set_source_files_properties(${swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS ${SOURCE_FILE_COMPILE_FLAGS})

set(CMAKE_SWIG_FLAGS -w401,402,509)
set_source_files_properties(mforms.i PROPERTIES CPLUSPLUS ON)
set_property(SOURCE mforms.i PROPERTY SWIG_FLAGS -w512 -fvirtual -DSWIG_PYTHON_LEGACY_BOOL)
swig_add_module(mforms python mforms.i)
swig_link_libraries(mforms mforms wbscintilla ${GTK3_LIBRARIES} ${LIBXML2_LIBRARIES})
set_source_files_properties(${swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS ${SOURCE_FILE_COMPILE_FLAGS})

target_compile_options(_mforms PUBLIC ${WB_CXXFLAGS})

set_target_properties(_mforms
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

target_compile_options(_cairo PUBLIC ${WB_CXXFLAGS})

set_target_properties(_cairo
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(BUILD_FOR_TESTS)
  target_link_libraries(_cairo gcov)
endif()                                 
                                 
install(TARGETS _mforms _cairo DESTINATION ${WB_PYTHON_MODULES_DIR})
