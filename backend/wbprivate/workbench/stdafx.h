/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#define HAVE_ROUND
#include <Python/Python.h>

#include <stack>
#include <pcre.h>
#include <ctime>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <list>
#include <vector>
#include <stack>

#include <stdio.h>
#include <float.h>
#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include <typeinfo>
#include <fstream>
#include <stdint.h>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>

#include <boost/signals2.hpp>
#include <boost/optional.hpp>
#include <boost/cstdint.hpp>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>

#include <gl/gl.h>
#include <gl/glu.h>

#include <glib.h>

#include <antlr3.h>

#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.model.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.query.h"
