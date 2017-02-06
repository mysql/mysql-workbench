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

// Enum class count and index.
template <class enumeration>
std::size_t enumCount() {
  static_assert(std::is_enum<enumeration>::value, "Not an enum");
  return static_cast<std::size_t>(enumeration::Count); // Requires that the enum class has a last member named "count".
}

template <class enumeration>
std::size_t enumIndex(const enumeration value) {
  static_assert(std::is_enum<enumeration>::value, "Not an enum");
  return static_cast<std::size_t>(value);
}
