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

#include "common.h"

#include <stdlib.h>
#include <stdio.h>

#ifndef HAVE_PRECOMPILED_HEADERS
  #include <glib.h>
#endif

#ifndef _WIN32
  #include <sys/stat.h>
#endif

// TODO: These function should probably be merged with file_utilities.
BASELIBRARY_PUBLIC_FUNC FILE* base_fopen(const char *filename, const char *mode);
BASELIBRARY_PUBLIC_FUNC int base_open(const std::string &filename, int open_flag, int permissions);
BASELIBRARY_PUBLIC_FUNC int base_remove(const std::string &filename);
BASELIBRARY_PUBLIC_FUNC int base_rename(const char *oldname, const char *newname);
#ifdef _WIN32
  BASELIBRARY_PUBLIC_FUNC int base_stat(const char *filename, struct _stat *stbuf);
#else
  BASELIBRARY_PUBLIC_FUNC int base_stat(const char *filename, struct stat *stbuf);
#endif

BASELIBRARY_PUBLIC_FUNC int base_rmdir_recursively(const char *dirname);
BASELIBRARY_PUBLIC_FUNC long base_get_file_size(const char *filename);
