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

using namespace System;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "WinCrypt.h"
#include <vcclr.h>
#include <msclr\lock.h>

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
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>

#include <gl/gl.h>
#include <float.h>

#include <glib/glib.h>

#ifndef _WIN32
#include <sys/time.h>
#include <time.h>
#endif

#include <boost/signals2.hpp>

#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.db.query.h"

#include "grts/structs.model.h"
#include "grts/structs.db.query.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.workbench.logical.h"
#include "grts/structs.db.migration.h"
