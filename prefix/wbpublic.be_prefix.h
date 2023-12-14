/*
 * Copyright (c) 2014, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifdef __OBJC__
  #import <Cocoa/Cocoa.h>
#endif

#ifdef __cplusplus

#define _USE_MATH_DEFINES
#include <math.h>

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
#include <exception>
#include <stack>
#include <thread>
#include <mutex>

#include <glib.h>

#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/ref.hpp>

#define BOOST_DATE_TIME_NO_LIB
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#undef BOOST_DATE_TIME_NO_LIB

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>

#include "grts/structs.model.h"
#include "grts/structs.db.query.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/rapidjson.h>

#endif
