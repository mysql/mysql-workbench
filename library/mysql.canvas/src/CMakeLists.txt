add_library(mdcanvas
    mdc_algorithms.cpp
    mdc_area_group.cpp
    mdc_back_layer.cpp
    mdc_bounds_magnet.cpp
    mdc_box.cpp
    mdc_box_handle.cpp
    mdc_button.cpp
    mdc_canvas_item.cpp
    mdc_canvas_view.cpp
    mdc_canvas_view_image.cpp
    mdc_canvas_view_x11.cpp
    mdc_canvas_view_opengl.cpp
    mdc_canvas_view_printing.cpp
    mdc_canvas_view_glx.cpp
    mdc_common.cpp
    mdc_connector.cpp
    mdc_draw_util.cpp
    mdc_figure.cpp
    mdc_group.cpp
    mdc_icon_text.cpp
    mdc_interaction_layer.cpp
    mdc_item_handle.cpp
    mdc_layer.cpp
    mdc_layouter.cpp
    mdc_line.cpp
    mdc_magnet.cpp
    mdc_image.cpp
    mdc_rectangle.cpp
    mdc_selection.cpp
    mdc_text.cpp
    mdc_vertex_handle.cpp
    mdc_image_manager.cpp
    mdc_orthogonal_line_layouter.cpp
    mdc_line_segment_handle.cpp
    mdc_box_side_magnet.cpp
)

target_include_directories(mdcanvas
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_include_directories(mdcanvas
 SYSTEM
  PRIVATE
    ${CAIRO_INCLUDE_DIRS}
)


target_compile_options(mdcanvas PRIVATE ${WB_CXXFLAGS})

target_link_libraries(mdcanvas
  PUBLIC
    ${CAIRO_LIBRARIES}
  PRIVATE 
    wbbase 
    ${OPENGL_LIBRARIES}
)

if(BUILD_FOR_GCOV)
  target_link_libraries(mdcanvas PRIVATE gcov)
endif()

set_target_properties(mdcanvas        
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})
if(COMMAND cotire)
   set_target_properties(mysql-workbench-bin PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(mdcanvas)
endif()

add_library(mdcanvasgtk
    gtk/mdc_gtk_canvas_view.cpp
    gtk/mdc_gtk_canvas_scroller.cpp
)

target_include_directories(mdcanvasgtk
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_include_directories(mdcanvasgtk
 SYSTEM
  PRIVATE
    ${GTKMM_INCLUDE_DIRS}
)


target_compile_options(mdcanvasgtk PRIVATE ${WB_CXXFLAGS})
target_link_libraries(mdcanvasgtk 
  PRIVATE 
    mdcanvas 
    wbbase 
)

if(BUILD_FOR_GCOV)
  target_link_libraries(mdcanvasgtk PRIVATE gcov)
endif()

set_target_properties(mdcanvasgtk
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(mdcanvasgtk PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(mdcanvasgtk)
endif()

install(TARGETS mdcanvas DESTINATION ${WB_INSTALL_LIB_DIR})
install(TARGETS mdcanvasgtk DESTINATION ${WB_INSTALL_LIB_DIR})
