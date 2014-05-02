/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _STDAFX_H_
#define _STDAFX_H_

#pragma warning(disable: 4793)  // 'vararg' causes native code generation

using namespace System;

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include "WinCrypt.h"
#include <vcclr.h>
#include <msclr\lock.h>
#endif

#include <list>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include <typeinfo>
#include <stdint.h> // Must be included before boost to avoid macro redefinition.

#ifdef __APPLE__
#undef nil
#define nil empty
#endif
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>

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

#include <gl/glu.h>

#include <glib/gthread.h>

#ifndef _WIN32
#include <sys/time.h>
#include <time.h>
#endif

#include <boost/foreach.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

#endif // _STDAFX_H_
