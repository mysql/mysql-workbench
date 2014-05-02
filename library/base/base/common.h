/* 
 * Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef _WIN32
  #if defined(_MSC_VER)
#pragma warning(disable:4251)
#endif//#if defined(_WIN32)

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

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN 
  #include <windows.h>

  #ifndef strcasecmp
    #define strcasecmp _stricmp
    #define strncasecmp strnicmp
  #endif

  #ifndef snprintf
    #define HAVE_SNPRINTF // For python libs.
    #define snprintf _snprintf_s
  #endif

#endif // _WIN32

#endif // _COMMON_H_
