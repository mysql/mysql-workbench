execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
                OUTPUT_VARIABLE GCC_VERSION)
if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") AND (GCC_VERSION VERSION_GREATER 4.6 OR GCC_VERSION VERSION_EQUAL 4.6))
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

set(SWIG_MODULE_mforms_EXTRA_DEPS
    ${PROJECT_SOURCE_DIR}/library/base/base/drawing.h
    ${PROJECT_SOURCE_DIR}/library/base/base/log.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/view.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/form.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/button.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/checkbox.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/textentry.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/textbox.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/label.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/selector.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/listbox.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/tabview.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/box.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/panel.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/filechooser.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/radiobutton.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/imagebox.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/progressbar.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/table.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/scrollpanel.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/wizard.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/drawbox.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/tabswitcher.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/app.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/appview.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/utilities.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/uistyle.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/appview.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/sectionbox.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/widgets.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/menu.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/splitter.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/webbrowser.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/popup.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/code_editor.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/menubar.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/toolbar.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/task_sidebar.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/hypertext.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/popover.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/fs_object_selector.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/simpleform.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/treeview.h
    ${PROJECT_SOURCE_DIR}/library/forms/mforms/dockingpoint.h
    ${PROJECT_SOURCE_DIR}/library/forms/swig/mforms_grt.h
    ${PROJECT_SOURCE_DIR}/library/forms/swig/mforms_drawbox.h
)

set(CMAKE_SWIG_FLAGS -w312)
set_source_files_properties(cairo.i PROPERTIES CPLUSPLUS ON)
set_property(SOURCE cairo.i PROPERTY SWIG_FLAGS -DCAIRO_HAS_PNG_FUNCTIONS=1 -fcompact -DSWIG_PYTHON_LEGACY_BOOL)
if(CMAKE_VERSION VERSION_LESS 3.8)
  swig_add_module(cairo python cairo.i)
else()
  swig_add_library(cairo LANGUAGE python SOURCES cairo.i)
endif()
swig_link_libraries(cairo ${CAIRO_LIBRARIES})

set(SOURCE_FILE_COMPILE_FLAGS "-Wno-missing-field-initializers -Wno-parentheses")
if(CMAKE_COMPILER_IS_GNUCXX)
  set(SOURCE_FILE_COMPILE_FLAGS "${SOURCE_FILE_COMPILE_FLAGS} -Wno-maybe-uninitialized -Wno-stringop-truncation -Wno-cast-function-type")
endif()

set_source_files_properties(${swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS ${SOURCE_FILE_COMPILE_FLAGS})

set(CMAKE_SWIG_FLAGS -w401,402,509)
set_source_files_properties(mforms.i PROPERTIES CPLUSPLUS ON)
set_property(SOURCE mforms.i PROPERTY SWIG_FLAGS -w512 -fvirtual -DSWIG_PYTHON_LEGACY_BOOL)
if(CMAKE_VERSION VERSION_LESS 3.8)
  swig_add_module(mforms python mforms.i)
else()
  swig_add_library(mforms LANGUAGE python SOURCES mforms.i)
endif()
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
