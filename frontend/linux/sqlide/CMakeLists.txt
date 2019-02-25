add_library(sqlide
    grid_view_model.cpp
    grid_view.cpp
    recordset_view.cpp
    ../mforms/record_grid_view.cpp
)

target_include_directories(sqlide
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
    ${PROJECT_SOURCE_DIR}/frontend/linux
    ${PROJECT_SOURCE_DIR}/generated
)

target_include_directories(sqlide
 SYSTEM
  PRIVATE
    ${VSQLITE_INCLUDE_DIRS}
    ${GTKMM_INCLUDE_DIRS}
)


target_compile_options(sqlide PRIVATE ${WB_CXXFLAGS})

target_link_libraries(sqlide 
  PRIVATE 
   mforms 
   linux_utilities::linux_utilities
   wbpublic::wbpublic
 ${GTK3_LIBRARIES} ${CAIRO_LIBRARIES} ${SIGC++_LIBRARIES})

if(BUILD_FOR_GCOV)
  target_link_libraries(sqlide PRIVATE gcov)
endif()

set_target_properties(sqlide
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(sqlide PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(sqlide)
endif()
install(TARGETS sqlide DESTINATION ${WB_INSTALL_LIB_DIR})
