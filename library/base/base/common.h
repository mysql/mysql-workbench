/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#define MIN_SERVER_VERSION 50600
#define MAX_SERVER_VERSION 99999

#if defined(__GNUC__) || defined(__APPLE)
#define WB_UNUSED __attribute__((unused))
#define WB_UNUSED_RETURN_VALUE __attribute__((warn_unused_result))
#else
#define WB_UNUSED
#define WB_UNUSED_RETURN_VALUE
#endif

// Define OS-independent debug flag.
#if defined(_DEBUG) || defined(ENABLE_DEBUG)
  #define WB_DEBUG
#elif defined(__linux__) && !defined(NDEBUG)
  #define WB_DEBUG
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4251) // class needs to have dll-interface

#ifdef BASELIBRARY_EXPORTS
#define BASELIBRARY_PUBLIC_FUNC __declspec(dllexport)
#else
#define BASELIBRARY_PUBLIC_FUNC __declspec(dllimport)
#endif
#else
#define BASELIBRARY_PUBLIC_FUNC
#endif

#include <string>
#include <math.h>

#ifdef _MSC_VER
#ifndef strcasecmp
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#ifndef snprintf
#define HAVE_SNPRINTF 1 // For python libs.
#define snprintf _snprintf_s
#endif

#endif // _MSC_VER

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
    // On OSX we only support the 64bit arch.
    #define DEFINE_SSIZE_T_FUNCTIONS
    #define DEFINE_UINT64_T_FUNCTIONS
    #define DEFINE_INT_FUNCTIONS
  #endif
#endif
