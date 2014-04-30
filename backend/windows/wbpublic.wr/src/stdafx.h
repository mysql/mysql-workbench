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

#pragma once

#pragma warning(disable: 4793)  // 'vararg' causes native code generation

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

//#define snprintf _snprintf

#include  <vcclr.h> // .net interop helpers

#define _USE_MATH_DEFINES
#include <math.h>

#include <glib.h>

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

#include <pcre.h>

#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/ref.hpp>

#include <gl/gl.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>
