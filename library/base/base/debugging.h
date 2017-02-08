/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DEBUGGING_H_
#define _DEBUGGING_H_

#if defined(_WIN32) && defined(_DEBUG)

#include "common.h"

// Helper class to allow setting a data change break point in specific scope (function, block etc.).

#ifdef _WIN64
typedef DWORD64 REGISTER_TYPE;
#else
typedef DWORD REGISTER_TYPE;
#endif

class BASELIBRARY_PUBLIC_FUNC DataBreakpoint {
private:
  // One of the 4 hardware registers that are needed for hw watch points.
  REGISTER_TYPE _register_index;

  void SetBits(REGISTER_TYPE& target, REGISTER_TYPE offset, REGISTER_TYPE bits, REGISTER_TYPE value);

public:
  DataBreakpoint();
  ~DataBreakpoint();

  // Enum values used by the Intel Pentium. Don't change them!
  enum Condition { Write = 1, Read /* or write! */ = 3 };

  void Set(void* address, int size, Condition when);
  void Clear();
};

#endif // _WIN32 && _DEBUG

#endif // _DEBUGGING_H_
