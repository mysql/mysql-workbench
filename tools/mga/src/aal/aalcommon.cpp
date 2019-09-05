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

#include "aalcommon.h"
#include "accessible.h"
#include "role.h"

//----------------------------------------------------------------------------------------------------------------------

void aal::methodNotImplemented(std::string const& method) {
  std::cout << method << " -> method not implemented..." << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------

// Implementation of certain shared code for all platforms.

using namespace aal;

std::string aal::keyToString(Key k) {
  std::map<Key, std::string> keyMap = {
    { Key::NoKey, "None" },
    { Key::Key1, "Key1" },
    { Key::Key2, "Key2" },
    { Key::Key3, "Key3" },
    { Key::Key4, "Key4" },
    { Key::Key5, "Key5" },
    { Key::Key6, "Key6" },
    { Key::Key7, "Key7" },
    { Key::Key8, "Key8" },
    { Key::Key9, "Key9" },
    { Key::Key0, "Key0" },
    { Key::KeyPlus, "KeyPlus" },
    { Key::KeyMinus, "KeyMinus" },
    { Key::KeyA, "KeyA" },
    { Key::KeyB, "KeyB" },
    { Key::KeyC, "KeyC" },
    { Key::KeyD, "KeyD" },
    { Key::KeyE, "KeyE" },
    { Key::KeyF, "KeyF" },
    { Key::KeyG, "KeyG" },
    { Key::KeyH, "KeyH" },
    { Key::KeyI, "KeyI" },
    { Key::KeyJ, "KeyJ" },
    { Key::KeyK, "KeyK" },
    { Key::KeyL, "KeyL" },
    { Key::KeyM, "KeyM" },
    { Key::KeyN, "KeyN" },
    { Key::KeyO, "KeyO" },
    { Key::KeyP, "KeyP" },
    { Key::KeyQ, "KeyQ" },
    { Key::KeyR, "KeyR" },
    { Key::KeyS, "KeyS" },
    { Key::KeyT, "KeyT" },
    { Key::KeyU, "KeyU" },
    { Key::KeyV, "KeyV" },
    { Key::KeyW, "KeyW" },
    { Key::KeyX, "KeyX" },
    { Key::KeyY, "KeyY" },
    { Key::KeyZ, "KeyZ" },

    { Key::KeyTab, "KeyTab" },
    { Key::KeyBackspace, "KeyBackspace" },
    { Key::KeyReturn, "KeyReturn" },
    { Key::KeyDot, "KeyDot" },
    { Key::KeyComma, "KeyComma" },
    { Key::KeyColon, "KeyColon" },
    { Key::KeySlash, "KeySlash" },
    { Key::KeyBackslash, "KeyBackslash" },
    { Key::KeyBraceLeft, "KeyBraceLeft" },
    { Key::KeyBraceRight, "KeyBraceRight" },
    { Key::KeyDelete, "KeyDelete" },
    { Key::KeyUp, "KeyUp" },
    { Key::KeyEscape, "KeyEscape" },
    { Key::KeyDown, "KeyDown" },
    { Key::KeyLeft, "KeyLeft" },
    { Key::KeyRight, "KeyRight" },
    { Key::KeyPageUp, "KeyPageUp" },
    { Key::KeyPageDown, "KeyPageDown" },
    { Key::KeyEnd, "KeyEnd" },
    { Key::KeyHome, "KeyHome" },
    { Key::KeySpace, "KeySpace" },

    { Key::KeyF1, "KeyF1" },
    { Key::KeyF2, "KeyF2" },
    { Key::KeyF3, "KeyF3" },
    { Key::KeyF4, "KeyF4" },
    { Key::KeyF5, "KeyF5" },
    { Key::KeyF6, "KeyF6" },
    { Key::KeyF7, "KeyF7" },
    { Key::KeyF8, "KeyF8" },
    { Key::KeyF9, "KeyF9" },
    { Key::KeyF10, "Key10" },
    { Key::KeyF11, "KeyF11" },
    { Key::KeyF12, "KeyF12" },
  };

  auto it = keyMap.find(k);
  if (it == keyMap.end())
    return "Unhandled key";
  return it->second;
}

//----------------------------------------------------------------------------------------------------------------------

std::string aal::modifierToString(Modifier k) {
  std::map<Modifier, std::string> modifierMap = {
    { Modifier::NoModifier, "NoModifier" },
    { Modifier::ShiftLeft, "ShiftLeft" },
    { Modifier::ShiftRight, "ShiftRight" },
    { Modifier::ControlLeft, "ControlLeft" },
    { Modifier::ControlRight, "ControlRight" },
    { Modifier::AltLeft, "AltLeft" },
    { Modifier::AltRight, "AltRight" },
    { Modifier::MetaLeft, "MetaLeft" },
    { Modifier::MetaRight, "MetaRight" },
  };

  auto it = modifierMap.find(k);
  if (it == modifierMap.end())
    return "Unknown modifier key";
  return it->second;
}

//----------------------------------------------------------------------------------------------------------------------

bool aal::containsModifier(Modifier set, Modifier member) {
  return ((static_cast<int>(set) & static_cast<int>(member)) != 0);
}

//----------------------------------------------------------------------------------------------------------------------

Modifier aal::modifierFromNumber(size_t i) {
  if (i > 0xFF)
    throw std::runtime_error("Invalid key modifier specified");

  return static_cast<Modifier>(i);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::dump(bool recursive, std::string const& indentation) const {
  if (!isValid())
    return indentation + "<element not valid>";

  std::stringstream ss;
  ss << indentation << "Element: ";

  std::string description;
  try {
    description = getDescription();
  } catch (std::runtime_error &) {
  }

  std::string identifier;
  try {
    identifier = getID();
  } catch (std::runtime_error &) {
  }

  if (identifier.empty())
    identifier = "<no id>";
  ss << "id: '" + identifier + "'";

  auto name = getName();
  if (name.empty()) {
    try {
      name = getText(); // For certain elements in lists, e.g. in an icon view on macOS.
    } catch (...) {
    }
  }

  if (name.empty())
    name = "<no name>";

  ss << ", name: '" << name << "', type: " << roleToFriendlyString(getRole());

  bool focused = false;
  try {
    focused = isFocused();
    if (focused)
      ss << ", focused";
  } catch (...) {
  }


  geometry::Rectangle bounds;
  try {
    bounds = getBounds(true);
    ss << ", bounds: " << getBounds(true);
  } catch (...) {
  }

  if (!description.empty())
    ss << ", description: '" + description + "'";

  if (recursive) {
    ss << std::endl;
    for (AccessibleRef &child : children()) {
      ss << child->dump(true, indentation + "\t");
    }
  }

  return ss.str();
}

//----------------------------------------------------------------------------------------------------------------------

