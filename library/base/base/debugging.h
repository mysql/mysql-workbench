/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#if defined(_MSC_VER) && defined(_DEBUG)

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

#endif // _MSC_VER && _DEBUG
