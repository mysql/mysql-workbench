/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef HAVE_PRECOMPILED_HEADERS
#include <unordered_set>
#endif

namespace base {
#ifndef _WIN32
  struct EnumHash {
    template <typename T>
    inline typename std::enable_if<std::is_enum<T>::value, std::size_t>::type operator()(T const value) const {
      return static_cast<std::size_t>(value);
    }
  };
  template <class T>
  using unordered_enumset = std::unordered_set<T, EnumHash>;
#else
  template <class T>
  using unordered_enumset = std::unordered_set<T>;
#endif
}
