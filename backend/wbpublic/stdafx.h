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

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <stdint.h>

#define snprintf _snprintf

#include <string>
#include <vector>
#include <stdexcept>
#include <stdarg.h>
#include <algorithm>
#include <map>
#include <list>
#include <sstream>
#include <cctype>
#include <algorithm>

#include <glib.h>

#include <pcre.h>

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>

#include <gl/gl.h>

#include <iterator>

// Common includes.
#include <stack>

#ifdef _WIN32
#include <antlr3.h>

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/geometry.h"
#include "base/threaded_timer.h"
#endif

#endif // _WIN32

#endif // _STDAFX_H_
