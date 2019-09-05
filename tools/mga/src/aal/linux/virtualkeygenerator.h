/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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
#include <memory>
#include <vector>
#include <X11/Xlib.h>
#include <glib.h>

class VirtualKeyGenerator {
  std::shared_ptr<Display> _dp;
  int keyCodeBegin;
  int keyCodeEnd;
  KeySym *symbols;
  int symbolsPerCode;
  std::vector<KeyCode> modTable;
  int altIdxLeft;
  int altIdxRight;

public:
  enum KeyDirection {
    Press = 1,
    Release = 0,
    NoDir = 2,
  };

  enum KeyModifier { ModShift = (1 << 1), ModControl = (1 << 2), ModAltLeft = (1 << 3), ModAltRight = (1 << 4) };

  struct ModKeysCodes {
    KeyCode Shift;
    KeyCode AltLeft;
    KeyCode AltRight;
    KeyCode Control;
  };

  class KeyEvent {
    friend VirtualKeyGenerator;
    KeyCode _code;
    int _modifiers;
    KeyDirection _currentDir;
    std::shared_ptr<Display> _dp;
    ModKeysCodes _mCodes;
    KeyEvent(std::shared_ptr<Display> dp, ModKeysCodes mCodes, KeyCode code, int modKeys);

  protected:
    void sendEvent();

  public:
    ~KeyEvent();
    bool isValid();
    void press();
    void release();
  };

  VirtualKeyGenerator();
  int checkIfShiftNeeded(KeyCode code, KeySym symbol);
  KeyEvent generateEvent(const gunichar key);
};
