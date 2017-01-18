/*
 * Copyright (c) 2005, 2017, Oracle and/or its affiliates. All rights reserved.
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

#define DATETIME_FMT "%Y-%m-%d %H:%M"

#include "common.h"

#ifndef HAVE_PRECOMPILED_HEADERS
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "glib.h"
#endif

#ifdef _WIN32
#define _br "\r\n"
#define BASE_PATH_SEPARATOR '\\'
#define BASE_PATH_SEPARATOR_STR "\\"

#else
#define _br "\n"
#define BASE_PATH_SEPARATOR '/'
#define BASE_PATH_SEPARATOR_STR "/"
#endif

#define BASE_ORDPTR(value) ((void *)(unsigned long)(value))

// TODO: move Windows specific stuff to base.windows library.

/*
 * Functions
 */
BASELIBRARY_PUBLIC_FUNC char *str_g_replace(char *str, const char *search, const char *replace);
BASELIBRARY_PUBLIC_FUNC char *str_g_subst(const char *str, const char *search, const char *replace);
BASELIBRARY_PUBLIC_FUNC char *auto_line_break(const char *txt, unsigned int width, char sep);

BASELIBRARY_PUBLIC_FUNC char *str_toupper(char *str);
BASELIBRARY_PUBLIC_FUNC int str_is_numeric(const char *str);

#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)
BASELIBRARY_PUBLIC_FUNC int get_value_from_registry(HKEY root_key, const char *sub_key, const char *key,
                                                    const char *def, char *value, int target_size);
BASELIBRARY_PUBLIC_FUNC int set_value_to_registry(HKEY root_key, const char *sub_key, const char *key,
                                                  const char *value);
#endif

BASELIBRARY_PUBLIC_FUNC void set_os_specific_password_functions(
  char *(*store_func)(const char *host, const char *username, const char *password),
  char *(*retrieve_func)(const char *host, const char *username, const char *password_data));

BASELIBRARY_PUBLIC_FUNC std::string get_local_os_name(void);
BASELIBRARY_PUBLIC_FUNC std::string get_local_hardware_info(void);

BASELIBRARY_PUBLIC_FUNC std::int64_t get_physical_memory_size(void);

BASELIBRARY_PUBLIC_FUNC std::int64_t get_file_size(const char *filename);

BASELIBRARY_PUBLIC_FUNC char *strcasestr_len(const char *haystack, int haystack_len, const char *needle);

BASELIBRARY_PUBLIC_FUNC const char *strfindword(const char *str, const char *word);

BASELIBRARY_PUBLIC_FUNC int base_mkdir(const char *filename, int mode, int *error_no);
BASELIBRARY_PUBLIC_FUNC int base_chdir(const char *path);

BASELIBRARY_PUBLIC_FUNC int copy_file(const char *source, const char *target);
BASELIBRARY_PUBLIC_FUNC int copy_folder(const char *source_folder, const char *target_folder);

#include <vector>
#include <algorithm>

namespace base {
  BASELIBRARY_PUBLIC_FUNC double timestamp();

  BASELIBRARY_PUBLIC_FUNC std::string fmttime(time_t t = 0, const char *fmt = "%b %d, %Y");

  //-----------------------------------------------------------------------------
  // Return value is a reference to vector which was passed as the first argument
  template <typename T>
  inline std::vector<T> &vector_remove(std::vector<T> &v, const T &k) {
    const typename std::vector<T>::iterator it = std::remove(v.begin(), v.end(), k);
    v.erase(it, v.end());
    return v;
  }

} // namespace base
