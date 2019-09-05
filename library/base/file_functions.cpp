/*
 * Copyright (c) 2005, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/common.h"

#ifndef _MSC_VER
#include <errno.h>
#include <sys/file.h>
#endif

#include "base/file_functions.h"
#include "base/string_utilities.h"

#include <glib/gstdio.h>

using namespace base;

//--------------------------------------------------------------------------------------------------

/**
 * @brief Wrapper around fopen that expects a filename in UTF-8 encoding
 * @param filename name of file to open
 * @param mode second argument of fopen
 * @return If successful, base_fopen returns opened FILE*.
 *           Otherwise, it returns NULL.
 */
FILE *base_fopen(const char *filename, const char *mode) {
#ifdef _MSC_VER
  std::wstring wmode;
  while (*mode != '\0')
    wmode += *mode++;
  if (wmode.find_first_of(L"b") == std::wstring::npos && wmode.find_first_of(L"t") == std::wstring::npos)
    wmode += L"b"; // Always open in binary mode.
  return _wfsopen(string_to_wstring(filename).c_str(), wmode.c_str(), _SH_DENYWR);

#else

  FILE *file;
  char *local_filename;

  if (!(local_filename = g_filename_from_utf8(filename, -1, NULL, NULL, NULL)))
    return NULL;

  file = fopen(local_filename, mode);

  g_free(local_filename);

  return file;
#endif
}

//--------------------------------------------------------------------------------------------------

/**
 *	Similar to base_fopen but returns a file descriptor instead. The file is always opened in binary
 *	mode (only matters on Windows).
 *	Also here, the filename must be UTF-8 encoded.
 */
int base_open(const std::string &filename, int open_flag, int permissions) {
  int fd;

#ifdef _MSC_VER
  int result = _wsopen_s(&fd, string_to_wstring(filename).c_str(), open_flag | O_BINARY, _SH_DENYWR, permissions);
  if (result != 0)
    return -1;
#else
  char *local_filename = g_filename_from_utf8(filename.c_str(), -1, NULL, NULL, NULL);
  if (local_filename == NULL)
    return -1;

  fd = open(local_filename, open_flag, permissions);
  g_free(local_filename);

#endif

  return fd;
}

//--------------------------------------------------------------------------------------------------

int base_remove(const std::string &filename) {
#ifdef _MSC_VER
  return _wremove(string_to_wstring(filename).c_str());
#else
  char *local_filename;
  if (!(local_filename = g_filename_from_utf8(filename.c_str(), -1, NULL, NULL, NULL)))
    return -1;
  int res = remove(local_filename);
  g_free(local_filename);

  return res;
#endif
}

//--------------------------------------------------------------------------------------------------

int base_rename(const char *oldname, const char *newname) {
#ifdef _MSC_VER

  int result;
  int required;
  WCHAR *converted_old;
  WCHAR *converted_new;

  required = MultiByteToWideChar(CP_UTF8, 0, oldname, -1, NULL, 0);
  if (required == 0)
    return -1;

  converted_old = g_new0(WCHAR, required);
  MultiByteToWideChar(CP_UTF8, 0, oldname, -1, converted_old, required);

  required = MultiByteToWideChar(CP_UTF8, 0, newname, -1, NULL, 0);
  if (required == 0) {
    g_free(converted_old);
    return -1;
  }

  converted_new = g_new0(WCHAR, required);
  MultiByteToWideChar(CP_UTF8, 0, newname, -1, converted_new, required);

  result = _wrename(converted_old, converted_new);

  g_free(converted_old);
  g_free(converted_new);

  return result;

#else

  char *local_oldname;
  char *local_newname;

  if (!(local_oldname = g_filename_from_utf8(oldname, -1, NULL, NULL, NULL)) ||
      !(local_newname = g_filename_from_utf8(newname, -1, NULL, NULL, NULL)))
    return EINVAL;

  const int file = rename(local_oldname, local_newname);

  g_free(local_oldname);
  g_free(local_newname);

  return file;
#endif
}

//--------------------------------------------------------------------------------------------------

#ifdef _MSC_VER
int base_stat(const char *filename, struct _stat *stbuf) {
  // Convert filename from UTF-8 to UTF-16.
  int required;
  WCHAR *converted;
  int result;

  required = MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
  if (required == 0)
    return -1;

  // Required contains the length for the result string including the terminating 0.
  converted = g_new0(WCHAR, required);
  MultiByteToWideChar(CP_UTF8, 0, filename, -1, converted, required);

  result = _wstat(converted, stbuf);
  g_free(converted);

  return result;
}
#else
int base_stat(const char *filename, struct stat *stbuf) {
  return g_stat(filename, stbuf);
}
#endif

//--------------------------------------------------------------------------------------------------

int base_rmdir_recursively(const char *path) {
  int res = 0;
  GError *error = NULL;
  GDir *dir;
  const char *dir_entry;
  gchar *entry_path;

  dir = g_dir_open(path, 0, &error);
  if (!dir && error)
    return error->code;

  while ((dir_entry = g_dir_read_name(dir))) {
    entry_path = g_build_filename(path, dir_entry, NULL);
    if (g_file_test(entry_path, G_FILE_TEST_IS_DIR))
      (void)base_rmdir_recursively(entry_path);
    else
      (void)::g_remove(entry_path);
    g_free(entry_path);
  }

  (void)g_rmdir(path);

  g_dir_close(dir);
  return res;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the size of the specified file (if it exists and can be accessed, otherwise 0).
 */
long base_get_file_size(const char *filename) {
  long result = 0;

#ifdef _MSC_VER
  struct _stat file_stat;
  if (base_stat(filename, &file_stat) == 0)
    result = file_stat.st_size;
#else
  struct stat file_stat;
  if (base_stat(filename, &file_stat) == 0)
    result = file_stat.st_size;
#endif

  return result;
}

//--------------------------------------------------------------------------------------------------
