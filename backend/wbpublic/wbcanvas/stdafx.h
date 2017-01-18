#ifndef _STDAFX_H_
#define _STDAFX_H_

#ifdef _WIN32

#pragma once

#include <list>
#include <vector>
#include <sigc++/sigc++.h>

#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include <typeinfo>

#ifdef __APPLE__
#undef nil
#define nil empty
#endif
#include <sigc++/sigc++.h>
#include <stdio.h>

#include <glib.h>

#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#elif defined(_WIN32)
#define snprintf _snprintf
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include <float.h>
#define INFINITY FLT_MAX
#else
#include <GL/gl.h>
#endif

#include <gl/gl.h>
#include <gl/glu.h>

#include <wgl/glitz-wgl.h>

extern "C" {
#include <cairo/cairo-glitz.h>
};

#include <glib/gthread.h>

#endif

#endif // _STDAFX_H_
