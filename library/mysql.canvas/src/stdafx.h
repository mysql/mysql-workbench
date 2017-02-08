/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
