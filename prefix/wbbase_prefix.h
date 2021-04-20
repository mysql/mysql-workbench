/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates.
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

#include <errno.h>
#include <string>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <vector>
#include <list>
#include <set>
#include <sstream>
#include <typeinfo>
#include <unistd.h>
#include <stdexcept>
#include <functional>
#include <locale>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <sys/time.h>
#include <condition_variable>

#include <glib.h>
#include <glib/gstdio.h>

#include <boost/bind/bind.hpp>
#include <boost/optional.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/locale/encoding_utf.hpp>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/rapidjson.h>

#endif
