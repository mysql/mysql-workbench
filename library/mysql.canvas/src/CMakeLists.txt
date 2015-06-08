include_directories(.
    ${CAIRO_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/backend
    ${PROJECT_SOURCE_DIR}/library/base
)

#if (UNIX)
#	include_directories(./linux)
#endif ()

#TODO: Set other compiler flags
#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas)
#endif ()

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
target_link_libraries(mdcanvas ${CAIRO_LIBRARIES})

set_target_properties(mdcanvas        
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

include_directories(./gtk)

add_library(mdcanvasgtk
    gtk/mdc_gtk_canvas_view.cpp
    gtk/mdc_gtk_canvas_scroller.cpp
)
target_link_libraries(mdcanvasgtk mdcanvas ${CAIRO_LIBRARIES} ${GTK3_LIBRARIES})

set_target_properties(mdcanvasgtk
                      PROPERTIES VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})


install(TARGETS mdcanvas DESTINATION ${WB_INSTALL_LIB_DIR})
install(TARGETS mdcanvasgtk DESTINATION ${WB_INSTALL_LIB_DIR})
