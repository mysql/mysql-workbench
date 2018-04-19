/*
 * Copyright (c) 2005, 2018, Oracle and/or its affiliates. All rights reserved.
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

#if defined(_MSC_VER)
#define __LCC__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Definition of the character property for a typical MySQL identifier. Since PCRE does not support every
// possible character property there might still be some failures.
// However most cases should be caught be the definition below (any letter [case insensitiv], any number, underscore,
// white spaces [which should only appear in quoted form], connector and dash punctuation [also only meaningfull in
// quoted ids], marks [any form], math/currency/modifier symbols). Anything not being a letter or number should only
// appear in quoted form.
#define UNICODE_CHAR_PCRE "\\pL|\\pN|\\pM|\\p{Pc}|\\p{Pd}|\\pS}|_"
#define UNICODE_ID_PCRE "(?:\\pL(?:" UNICODE_CHAR_PCRE ")*)"
#define UNICODE_ID_PCRE_AND_WHITESPACE "(?:\\pL(?:\\p{Zs}|" UNICODE_CHAR_PCRE ")*)"
#define IDENTIFIER_PCRE "`" UNICODE_ID_PCRE_AND_WHITESPACE "`|\"" UNICODE_ID_PCRE_AND_WHITESPACE "\"|" UNICODE_ID_PCRE
#define IDENTIFIER_IGNORE_PCRE "(?:" IDENTIFIER_PCRE ")"
#define QUALIFIED_IDENTIFIER_PCRE "(?:(" IDENTIFIER_PCRE ")\\.)?(" IDENTIFIER_PCRE ")"
#define QUALIFIED_IDENTIFIER_IGNORE_PCRE "(?:(?:" IDENTIFIER_PCRE ")\\.)?(?:" IDENTIFIER_PCRE ")"

#ifdef _MSC_VER

#include <winsock2.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <iconv.h>
#include <tree.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Windows includes
#if defined(_MSC_VER)
#include <direct.h>
#else
// unix/linux includes

#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/utsname.h> // uname()
#include <fcntl.h>

#define SIZE_T size_t

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// MacOS X
#if defined(__APPLE__) && defined(__MACH__)
#include <sys/sysctl.h>
#include <mach/machine.h>
#endif

#endif
