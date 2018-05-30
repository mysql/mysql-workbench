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

#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdint.h>
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
#include <memory>
#include <iterator>
#include <stack>
#include <set>
#include <iosfwd>
#include <fstream>
#include <iomanip>
#include <mutex>

#include <Python.h>

#include <glib.h>

#include <boost/signals2.hpp>
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>

#include <gl/gl.h>

#include "grts/structs.h"
#include "grts/structs.app.h"

#include "grts/structs.db.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.model.h"
#include "grts/structs.ui.h"

#include "grts/structs.db.query.h"
#include "grts/structs.db.mysql.h"

#include "grts/structs.meta.h"
#include "grts/structs.db.migration.h"
#include "grts/structs.eer.h"

#include "grts/structs.workbench.logical.h"
#include "grts/structs.workbench.model.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.workbench.h"

#include "grts/structs.wrapper.h"
