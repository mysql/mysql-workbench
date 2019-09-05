/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

// In Win32 ssize_t and int are the same, so we get a compiler error if we compile functions/c-tors with
// those types (redefinition error). Hence we need a check when to exclude them.
// A similar problem exists for uint64_t and size_t in Win64.
#ifdef _MSC_VER
  #define DEFINE_INT_FUNCTIONS

  #ifdef _WIN64
    #define DEFINE_SSIZE_T_FUNCTIONS
  #else
    #define DEFINE_UINT64_T_FUNCTIONS
  #endif
#else
  #define DEFINE_SSIZE_T_FUNCTIONS

  #ifdef __LP64__
    #define DEFINE_INT_FUNCTIONS
  #endif

  #ifdef __APPLE__
    // On macOS we only support the 64bit arch.
    #define DEFINE_SSIZE_T_FUNCTIONS
    #define DEFINE_UINT64_T_FUNCTIONS
    #define DEFINE_INT_FUNCTIONS
  #endif
#endif

#define DUMP_DUK_CONTEXT(ctx) \
  duk_push_context_dump(ctx); \
  printf("%s\n", duk_to_string(ctx, -1)); \
  duk_pop(ctx);

#ifdef __OBJC__
  #import <Cocoa/Cocoa.h>
#endif

#ifndef _MSC_VER // Standard headers on Windows are included via stdafx.h
  #include <iostream>
  #include <assert.h>
  #include <memory>
  #include <thread>
  #include <chrono>
  #include <string>
  #include <string.h>
  #include <vector>
  #include <map>
  #include <algorithm>
  #include <fstream>
  #include <stdio.h>
  #include <stdlib.h>
  #include <functional>
  #include <codecvt>
  #include <set>
  #include <sstream>
  #include <stdarg.h>
  #include <locale>
  #include <exception>
  #include <fcntl.h>
  #include <iterator>
  #include <iomanip>
  #include <future>
  #include <stack>

  #include <sys/stat.h>
  #include <sys/types.h>
  #include <sys/utsname.h>
  #include <sys/sysctl.h>
  #include <sys/utsname.h>
  #include <sys/socket.h>

  #include <utime.h>
  #include <unistd.h>
  #include <dirent.h>
  #include <fnmatch.h>
  #include <poll.h>

  #include <pwd.h>
  #include <netinet/in.h>
  #include <ifaddrs.h>
  #include <net/if.h>
  #include <netdb.h>

  #ifndef __linux__
    #include <net/if_dl.h>
  #endif

  #include <cppconn/driver.h>
  #include <cppconn/connection.h>
  #include <cppconn/statement.h>
  #include <cppconn/metadata.h>
#endif

#include "duktape.h"

#define DUK_READ_ONLY (DUK_DEFPROP_CLEAR_CONFIGURABLE | DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_SET_ENUMERABLE)

namespace mga {

  class UIElement;
  using UIElementRef = std::unique_ptr<UIElement>;
  using UIElementList = std::vector<UIElementRef>;


  enum ShowState {
    Hidden = 0,
    Normal = 1,
    Maximized = 2,
    HideOthers = 3
  };
}
