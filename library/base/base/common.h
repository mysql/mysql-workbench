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

#define MIN_SERVER_VERSION 50100
#define MAX_SERVER_VERSION 99999

#if defined(__GNUC__) || defined(__APPLE)
#define WB_UNUSED __attribute__((unused))
#define WB_UNUSED_RETURN_VALUE __attribute__((warn_unused_result))
#else
#define WB_UNUSED
#define WB_UNUSED_RETURN_VALUE
#endif

// set OS-independent debug flag
#if defined(_WIN32)
#ifdef _DEBUG
#define WB_DEBUG
#endif
#elif defined(__APPLE__)
#ifdef ENABLE_DEBUG
#define WB_DEBUG
#endif
#elif defined(__linux__)
#ifndef NDEBUG
#define WB_DEBUG
#endif
#endif

#ifdef _WIN32
#pragma warning(disable : 4251) // class needs to have dll-interface

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

// In Win32 ssize_t and int are the same, so we get a compiler error if we compile functions/c-tors with
// those types (redefinition error). Hence we need a check when to exclude them.
// A similar problem exists for uint64_t and size_t in Win64.
#ifdef _WIN32
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
