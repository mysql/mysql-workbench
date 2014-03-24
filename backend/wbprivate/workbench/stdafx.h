/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _WIN32

#ifdef _WIN64
  typedef __int64 ssize_t;
#else
  typedef int ssize_t;
#endif

#include <stack>
#include <pcre.h>
#include <ctime>

#define snprintf _snprintf
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#define INFINITY FLT_MAX

#include <list>
#include <vector>

#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include <typeinfo>
#include <fstream>
#include <stdint.h>
#include <hash_set>

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>

#include <gl/gl.h>
#include <gl/glu.h>

#include <glib.h>
#include <glib/gthread.h>

#endif // _WIN32

#endif // _STDAFX_H_
