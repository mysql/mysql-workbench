%module cairo

#pragma SWIG nowarn=312

%{
#include <string>
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>


%}

%ignore _cairo;
%ignore _cairo_surface;
%ignore _cairo_device;
%ignore _cairo_matrix;
%ignore _cairo_pattern;
%ignore _cairo_user_data_key;
%ignore _cairo_rectangle;
%ignore _cairo_rectangle_list;
%ignore _cairo_scaled_font;
%ignore _cairo_font_face;
%ignore _cairo_font_options;
%ignore _cairo_region;
%ignore _cairo_rectangle_int;
%ignore cairo_path_data_t;
%ignore cairo_path_t;
%ignore cairo_path;

%ignore cairo_rectangle_list_t;
%ignore cairo_rectangle_list_destroy;
%ignore cairo_destroy_func_t;
%ignore cairo_user_data_key_t;

// may need wrapping
%ignore cairo_copy_clip_rectangle_list;

%ignore cairo_user_scaled_font_init_func_t;
%ignore cairo_user_scaled_font_render_glyph_func_t;
%ignore cairo_user_scaled_font_text_to_glyphs_func_t;
%ignore cairo_user_scaled_font_unicode_to_glyph_func_t;
%ignore cairo_user_font_face_set_init_func;
%ignore cairo_user_font_face_set_render_glyph_func;
%ignore cairo_user_font_face_set_text_to_glyphs_func;
%ignore cairo_user_font_face_set_unicode_to_glyph_func;
%ignore cairo_user_font_face_get_init_func;
%ignore cairo_user_font_face_get_render_glyph_func;
%ignore cairo_user_font_face_get_text_to_glyphs_func;
%ignore cairo_user_font_face_get_unicode_to_glyph_func;
%ignore cairo_user_font_face_create;

%ignore cairo_device_t;
%ignore cairo_device_reference;
%ignore cairo_device_type_t;
%ignore cairo_device_get_type;
%ignore cairo_device_status;
%ignore cairo_device_acquire;
%ignore cairo_device_release;
%ignore cairo_device_flush;
%ignore cairo_device_finish;
%ignore cairo_device_destroy;
%ignore cairo_device_get_reference_count;
%ignore cairo_device_get_user_data;
%ignore cairo_device_set_user_data;

%ignore cairo_image_surface_get_data;
%ignore cairo_image_surface_create_for_data;

// create separate wrapper for these if needed
%ignore cairo_set_user_data;
%ignore cairo_get_user_data;
%ignore cairo_font_face_set_user_data;
%ignore cairo_font_face_get_user_data;
%ignore cairo_scaled_font_get_user_data;
%ignore cairo_scaled_font_set_user_data;
%ignore cairo_surface_get_user_data;
%ignore cairo_surface_set_user_data;
%ignore cairo_pattern_get_user_data;
%ignore cairo_pattern_set_user_data;

%ignore cairo_surface_get_mime_data;

%ignore cairo_read_func_t;
%ignore cairo_write_func_t;

//%ignore cairo_set_dash;

// needs separate wrapper
%ignore cairo_image_surface_create_from_png_stream;
%ignore cairo_surface_write_to_png_stream;

// Not supported in cairo 1.8.8 (from OEL6)
%ignore cairo_surface_set_mime_data;
%ignore cairo_region_contains_point;
%ignore cairo_region_contains_rectangle;
%ignore cairo_region_copy;
%ignore cairo_region_create;
%ignore cairo_region_create_rectangle;
%ignore cairo_region_destroy;
%ignore cairo_region_get_extents;
%ignore cairo_region_get_rectangle;
%ignore cairo_region_intersect;
%ignore cairo_region_intersect_rectangle;
%ignore cairo_region_is_empty;
%ignore cairo_region_num_rectangles;
%ignore cairo_region_status;
%ignore cairo_region_subtract;
%ignore cairo_region_subtract_rectangle;
%ignore cairo_region_translate;
%ignore cairo_region_union;
%ignore cairo_region_union_rectangle;
%ignore CAIRO_STATUS_INVALID_SIZE;
%ignore CAIRO_STATUS_LAST_STATUS;
%ignore CAIRO_SURFACE_TYPE_SCRIPT;
%ignore CAIRO_REGION_OVERLAP_IN;
%ignore CAIRO_REGION_OVERLAP_OUT;
%ignore CAIRO_REGION_OVERLAP_PART;



%typemap(in, numinputs=0) cairo_text_extents_t *extents (cairo_text_extents_t extout) {
      $1 = &extout;
}

%typemap(argout) cairo_text_extents_t *extents {
      PyObject *o= SWIG_NewPointerObj(new cairo_text_extents_t(*$1), SWIGTYPE_p_cairo_text_extents_t, 0 |  0 );
      $result= SWIG_Python_AppendOutput($result, o);
}

%typemap(in) const char* (std::string s) {
  if (PyUnicode_Check($input))
  {
    PyObject *tmp = PyUnicode_AsUTF8String($input);
    s = PyString_AsString(tmp);
    $1 = (char*)s.c_str();
    Py_DECREF(tmp); 
  } 
  else if (PyString_Check($input)) 
  {
    s = PyString_AsString($input); 
    $1 = (char*)s.c_str();
  }
  else 
  { 
    PyErr_SetString(PyExc_TypeError, "not a string"); 
    SWIG_fail; 
  } 
} 

%typemap(freearg) const char* {
}

%typemap(in) (const double *dashes, int num_dashes) {
    if (PyList_Check($input)) {
        $1 = new double[PyList_Size($input)];
        $2 = PyList_Size($input);
        for (int c= PyList_Size($input), i= 0; i < c; i++)
        {
            PyObject *item = PyList_GetItem($input, i);
            if (PyFloat_Check(item)) 
                $1[i] = PyFloat_AsDouble(item);
            else
            {
                delete[] $1;
                $1 = 0;
                SWIG_exception_fail(SWIG_TypeError, "expected list of floats");
            }
        }
    }
    else
        SWIG_exception_fail(SWIG_TypeError, "expected list of floats");
}

%typemap(freearg) (const double *dashes, int num_dashes) {
    delete[] $1;
}


%include <cairo/cairo.h>
cairo_surface_t *cairo_image_surface_create_from_png_stream(PyObject *reader)
{
  return cairo_image_surface_create_from_png_stream(py_read_func, reader);
}

