/* 
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved.
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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#include <stdexcept>
#include <functional>
#include <locale>
#include <algorithm>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <wchar.h>
#include <codecvt>
#include <fcntl.h>

#include <list>
#include <string>
#include <map>
#include <sddl.h>
#include <stdio.h>
#include <winevt.h>

#include <VersionHelpers.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <glib/glib.h>
#include <glib/gstdio.h>
