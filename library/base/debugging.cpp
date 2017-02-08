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

#include "base/debugging.h"

#if defined(_WIN32) && defined(_DEBUG)

#include <windows.h>
#include <stdexcept>

//--------------------------------------------------------------------------------------------------

DataBreakpoint::DataBreakpoint() {
  _register_index = -1;
}

//--------------------------------------------------------------------------------------------------

DataBreakpoint::~DataBreakpoint() {
  Clear();
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to set the given value into target.
 *
 * @param target The target value to modify.
 * @param offset Bit offset in the target, counted from LSB.
 * @param bits Number of bits to set with value.
 * @value The new value to set.
 */
void DataBreakpoint::SetBits(REGISTER_TYPE& target, REGISTER_TYPE offset, REGISTER_TYPE bits, REGISTER_TYPE value) {
  REGISTER_TYPE mask = (1 << bits) - 1;
  target = (target & ~(mask << offset)) | (value << offset);
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets a data watch for the given address.
 *
 * @address The location to watch.
 * @size The size of the memory block address points to. Can be 1, 2 or 4 bytes.
 * @when Determines when to trigger the break point (write or read/write).
 */
void DataBreakpoint::Set(void* address, int size, Condition when) {
  if (_register_index != -1)
    throw std::runtime_error("Watch point already set. Use clear() before setting a new one.");

  switch (size) {
    case 1:
      size = 0;
      break;
    case 2:
      size = 1;
      break;
    case 4:
      size = 3;
      break;
    default:
      throw std::runtime_error("Invalid data length for watch point specified.");
  }

  HANDLE currentThread = GetCurrentThread();

  // Set up the thread context for this watch point.
  CONTEXT context;
  context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

  // Read the register values
  if (!GetThreadContext(currentThread, &context))
    throw std::runtime_error("Failed getting the current thread context.");

  // Find an available hardware register.
  for (_register_index = 0; _register_index < 4; ++_register_index) {
    if ((context.Dr7 & (REGISTER_TYPE)(1ULL << (2 * _register_index))) == 0)
      break;
  }
  if (_register_index >= 4)
    throw std::runtime_error("Could not find a free hardware register.");

  switch (_register_index) {
    case 0:
      context.Dr0 = (REGISTER_TYPE)address;
      break;
    case 1:
      context.Dr1 = (REGISTER_TYPE)address;
      break;
    case 2:
      context.Dr2 = (REGISTER_TYPE)address;
      break;
    case 3:
      context.Dr3 = (REGISTER_TYPE)address;
      break;
  }

  SetBits(context.Dr7, 16 + (4 * _register_index), 2, when);
  SetBits(context.Dr7, 18 + (4 * _register_index), 2, size);
  SetBits(context.Dr7, 2 * _register_index, 1, 1);

  if (!SetThreadContext(currentThread, &context))
    throw std::runtime_error("Could not set modified thread context for the watch point.");
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes a currently set watch point if one is set.
 */
void DataBreakpoint::Clear() {
  if (_register_index != -1) {
    CONTEXT context;
    HANDLE currentThread = GetCurrentThread();

    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(currentThread, &context))
      throw std::runtime_error("Failed getting the current thread context.");

    // Reset debug register settings for this watch point.
    if (_register_index < 0 && _register_index >= 4)
      throw std::runtime_error("Internal error: debug register index is wrong.");

    SetBits(context.Dr7, 2 * _register_index, 1, 0);

    if (!SetThreadContext(currentThread, &context))
      throw std::runtime_error("Could not set modified thread context for the watch point.");

    _register_index = -1;
  }
}

//--------------------------------------------------------------------------------------------------

#endif // _DEBUG
