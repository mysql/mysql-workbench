/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#define NOMINMAX

#ifdef _WIN64
  typedef __int64 ssize_t;
#else
  typedef int ssize_t;
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>

#include <tchar.h>
#include <direct.h>
#include <shlwapi.h>
#include <cctype>
#include <conio.h>

#include <sys\utime.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <io.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "UserEnv.lib")
#pragma comment(lib, "version.lib")

#include <Shellapi.h>
#include <Iphlpapi.h>
#include <UserEnv.h>

#include <iostream>
#include <assert.h>
#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <string.h>
#include <vector>
#include <map>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <codecvt>
#include <set>
#include <sstream>
#include <stdarg.h>
#include <locale>
#include <exception>
#include <fcntl.h>
#include <iterator>
#include <iomanip>
#include <signal.h>
#include <future>
#include <stack>

#include <sys/stat.h>
#include <sys/types.h>

#pragma warning( disable: 4100 4244 )

#ifndef CPPCONN_LIB_BUILD
  #define CPPCONN_LIB_BUILD
#endif

#include <algorithm>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/metadata.h>

#pragma warning( default: 4100 4244 )

#define R_OK    4       /* Test for read permission. */
#define W_OK    2       /* Test for write permission. */
#define X_OK    1       /* execute permission */
#define F_OK    0       /* Test for existence. */
#define S_IFLNK 0xA000  /* Symbolic link */
