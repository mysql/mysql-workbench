/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <list>
#include <vector>
#include <map>
#include <set>
#include <iosfwd>
#include <fstream>

#include <string>
#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include <typeinfo>
#include <time.h>
#include <float.h>
#include <memory>

#define _USE_MATH_DEFINES
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-win32.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>

#include <gl/gl.h>
//#include <gl/glu.h>

#include <boost/optional.hpp>
#include <boost/cstdint.hpp>

#include <glib.h>

namespace mforms {
  class Object;
  class ContextMenu;
  class DockingPointDelegate;
  class AppView;
  class View;
}

#pragma make_public(mforms::Object)
#pragma make_public(mforms::ContextMenu)
#pragma make_public(mforms::DockingPointDelegate)
#pragma make_public(mforms::AppView)
#pragma make_public(mforms::View)
