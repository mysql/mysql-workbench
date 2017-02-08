/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifdef __OBJC__
  #import <Cocoa/Cocoa.h>
  #import <WebKit/WebKit.h>
#endif

#ifdef __cplusplus

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <string>
#include <assert.h>
#include <vector>

#include <glib.h>
#include <glib/gstdio.h>

#include <cairo/cairo.h>

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "mforms/mforms.h"

#endif
