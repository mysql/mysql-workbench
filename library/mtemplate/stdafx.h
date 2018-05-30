/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <windows.h>
#include <shellapi.h>

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
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <list>
#include <inttypes.h>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <sddl.h>
#include <stdio.h>
#include <winevt.h>
#include <memory>

#include <VersionHelpers.h>

#include <boost/locale/encoding_utf.hpp>
#include <boost/optional.hpp>
#include <boost/cstdint.hpp>

#include <glib.h>
#include <glib/gstdio.h>

#include <iosfwd>
#include <fstream>
