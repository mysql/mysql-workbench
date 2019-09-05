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

#include "common.h"

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <iosfwd>
#include <fstream>

#ifndef _MSC_VER
#include <sys/stat.h>
#endif

// TODO: These function should probably be merged with file_utilities.
BASELIBRARY_PUBLIC_FUNC FILE *base_fopen(const char *filename, const char *mode);
BASELIBRARY_PUBLIC_FUNC int base_open(const std::string &filename, int open_flag, int permissions);
BASELIBRARY_PUBLIC_FUNC int base_remove(const std::string &filename);
BASELIBRARY_PUBLIC_FUNC int base_rename(const char *oldname, const char *newname);
#ifdef _MSC_VER
BASELIBRARY_PUBLIC_FUNC int base_stat(const char *filename, struct _stat *stbuf);
#else
BASELIBRARY_PUBLIC_FUNC int base_stat(const char *filename, struct stat *stbuf);
#endif

BASELIBRARY_PUBLIC_FUNC int base_rmdir_recursively(const char *dirname);
BASELIBRARY_PUBLIC_FUNC long base_get_file_size(const char *filename);
