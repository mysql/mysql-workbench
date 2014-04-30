#ifndef _STDAFX_H_
#define _STDAFX_H_

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

#include <list>
#include <vector>

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

#define _USE_MATH_DEFINES
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#elif defined(_WIN32)
#define snprintf _snprintf
#include <gl/gl.h>
#include <float.h>
#define INFINITY FLT_MAX
#else
#include <GL/gl.h>
#endif


#include <gl/gl.h>
#include <gl/glu.h>

#include <glib/gthread.h>

#ifndef _WIN32
#include <sys/time.h>
#include <time.h>
#endif

#endif
#endif // _STDAFX_H_
