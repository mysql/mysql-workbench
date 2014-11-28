/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#define MIN_SERVER_VERSION 50100
#define MAX_SERVER_VERSION 99999

#ifdef _WIN32
  #pragma warning(disable:4251) // class needs to have dll-interface

  #ifdef BASELIBRARY_EXPORTS
    #define BASELIBRARY_PUBLIC_FUNC __declspec(dllexport)
  #else
    #define BASELIBRARY_PUBLIC_FUNC __declspec(dllimport)
  #endif
#else
  #define BASELIBRARY_PUBLIC_FUNC
#endif

#if defined(_WIN32) || defined(__APPLE)
  #define HAVE_PRECOMPILED_HEADERS
#endif

#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#include <math.h>
#endif

#ifdef _WIN32
  #ifndef strcasecmp
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
  #endif

  #ifndef snprintf
    #define HAVE_SNPRINTF 1 // For python libs.
    #define snprintf _snprintf_s
  #endif

#endif // _WIN32
