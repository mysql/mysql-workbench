/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _MSC_VER
  #ifdef ACCESSIBILITY_EXPORTS
    #define ACCESSIBILITY_PUBLIC __declspec(dllexport)
  #else
    #define ACCESSIBILITY_PUBLIC __declspec(dllimport)
  #endif
#else
  #define ACCESSIBILITY_PUBLIC
#endif

#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <map>
#include <sstream>
#include <set>

namespace aal {

  enum class CheckState {
    Unchecked,
    Checked,
    Indeterminate
  };

  enum class MouseButton {
    NoButton = 0,
    Left = 1,
    Right = 2,
    Middle = 3,
  };

  enum class Key {
    NoKey,
    Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9, KeyPlus, KeyMinus,
    KeyA, KeyB, KeyC, KeyD, KeyE, KeyF, KeyG, KeyH, KeyI, KeyJ, KeyK, KeyL, KeyM,
    KeyN, KeyO, KeyP, KeyQ, KeyR, KeyS, KeyT, KeyU, KeyV, KeyW, KeyX, KeyY, KeyZ,
    KeyTab, KeyBackspace, KeyReturn, KeyDot, KeyComma, KeyColon, KeySlash, KeyBackslash, KeyBraceLeft, KeyBraceRight,
    KeyDelete, KeyUp, KeyEscape, KeyDown, KeyLeft, KeyRight, KeyPageUp, KeyPageDown, KeyEnd, KeyHome, KeySpace,
    KeyF1, KeyF2, KeyF3, KeyF4, KeyF5, KeyF6, KeyF7, KeyF8, KeyF9, KeyF10, KeyF11, KeyF12,

    Sentinel // The last entry, for iteration.
  };

  enum class Modifier {
    NoModifier   = 0x00,
    ShiftLeft    = 0x01,
    ShiftRight   = 0x02,
    ControlLeft  = 0x04,
    ControlRight = 0x08,
    AltLeft      = 0x10,
    AltRight     = 0x20,
    MetaLeft     = 0x40,
    MetaRight    = 0x80
  };

  ACCESSIBILITY_PUBLIC std::string keyToString(Key k);
  ACCESSIBILITY_PUBLIC std::string modifierToString(Modifier k);
  ACCESSIBILITY_PUBLIC bool containsModifier(Modifier set, Modifier member);
  ACCESSIBILITY_PUBLIC Modifier modifierFromNumber(size_t i);

  ACCESSIBILITY_PUBLIC void methodNotImplemented(std::string const& method);

#define NOT_IMPLEMENTED aal::methodNotImplemented(__FUNCTION__)

  class Accessible;
  using AccessibleRef = std::unique_ptr<Accessible>;
  using AccessibleList = std::vector<AccessibleRef>;
}
