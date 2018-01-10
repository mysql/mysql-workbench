/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __WB_UTILS_H__
#define __WB_UTILS_H__

//#define DEBUG 1
#undef DEBUG
#ifdef DEBUG

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

inline void wb_ptrace(void) {
  enum { SIZE = 100 };
  void *buffer[100];

  int nptrs = backtrace(buffer, SIZE);

  char **strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL)
    perror("backtrace_symbols");

  for (int j = 0; j < nptrs; j++)
    printf("%s\n", strings[j]);

  free(strings);
}
#endif

#ifdef DEBUG
#define ptrace() wb_ptrace()
#define dprint(...) fprintf(stderr, __VA_ARGS__)
#else
#define ptrace()
#define dprint(...)
#endif

#endif
