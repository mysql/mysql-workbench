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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <fstream>
#include <stack>
#include <string>
#include <cctype>

#include <glib.h>

#include "grts/structs.model.h"
#include "grts/structs.db.query.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.workbench.logical.h"
#include "grts/structs.db.migration.h"
