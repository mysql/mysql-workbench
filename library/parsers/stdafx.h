/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

#include <antlr4-runtime.h>

// TODO: the windows header can go once we no longer depend on the base lib. 
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include <deque>
#include <functional>
#include <stdint.h>
#include <stack>
#include <list>
#include <inttypes.h>
#include <sstream>
#include <iterator>
#include <memory>
#include <typeindex>
#include <locale>

#include <glib.h>

#include <boost/optional.hpp>
#include <boost/cstdint.hpp>

