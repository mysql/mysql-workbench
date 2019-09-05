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
#include "virtualkeygenerator.h"

#include <iostream>
#include <X11/extensions/XTest.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <xkbcommon/xkbcommon.h>
#include <X11/XKBlib.h>
#include <thread>

VirtualKeyGenerator::KeyEvent::KeyEvent(std::shared_ptr<Display> dp, ModKeysCodes mCodes, KeyCode code, int modKeys) {
  _dp = dp;
  _mCodes = mCodes;
  _code = code;
  _modifiers = modKeys;
  _currentDir = KeyDirection::Release;
}

//----------------------------------------------------------------------------------------------------------------------

VirtualKeyGenerator::KeyEvent::~KeyEvent() {
  if (_code != 0 && _currentDir == KeyDirection::Press) {
    std::cerr << "Forcing key release" << std::endl;
    _currentDir = KeyDirection::Release;
    sendEvent();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void VirtualKeyGenerator::KeyEvent::sendEvent() {
  if (_modifiers) {
    if (_modifiers & ModShift) {
      XTestFakeKeyEvent(_dp.get(), _mCodes.Shift, _currentDir == KeyDirection::Press ? 1 : 0, CurrentTime);
    }

    if (_modifiers & ModControl) {
      XTestFakeKeyEvent(_dp.get(), _mCodes.Control, _currentDir == KeyDirection::Press ? 1 : 0, CurrentTime);
    }

    if (_modifiers & ModAltRight) {
      XTestFakeKeyEvent(_dp.get(), _mCodes.AltLeft, _currentDir == KeyDirection::Press ? 1 : 0, CurrentTime);
    }

    if (_modifiers & ModAltLeft) {
      XTestFakeKeyEvent(_dp.get(), _mCodes.AltLeft, _currentDir == KeyDirection::Press ? 1 : 0, CurrentTime);
    }
    XSync(_dp.get(), 0);
  }
  XTestFakeKeyEvent(_dp.get(), _code, _currentDir == KeyDirection::Press ? 1 : 0, CurrentTime);
  XSync(_dp.get(), 0);
}

//----------------------------------------------------------------------------------------------------------------------

bool VirtualKeyGenerator::KeyEvent::isValid() {
  return _code != 0;
}

//----------------------------------------------------------------------------------------------------------------------

void VirtualKeyGenerator::KeyEvent::press() {
  if (_code == 0) {
    std::cerr << "Unknown key code" << std::endl;
    return;
  }

  if (_currentDir == KeyDirection::Press) {
    std::cerr << "Can't press key twice without releasing it." << std::endl;
    return;
  }
  _currentDir = KeyDirection::Press;
  sendEvent();
}

//----------------------------------------------------------------------------------------------------------------------

void VirtualKeyGenerator::KeyEvent::release() {
  if (_code == 0) {
    std::cerr << "Unknown key code" << std::endl;
    return;
  }

  if (_currentDir == KeyDirection::Release) {
    std::cerr << "Can't release key twice without pressing it." << std::endl;
    return;
  }
  _currentDir = KeyDirection::Release;
  sendEvent();
}

//----------------------------------------------------------------------------------------------------------------------

VirtualKeyGenerator::VirtualKeyGenerator() {
  _dp = std::shared_ptr<Display>(XOpenDisplay(nullptr), [](Display* d) { XCloseDisplay(d); });

  XDisplayKeycodes(_dp.get(), &keyCodeBegin, &keyCodeEnd);
  symbols = XGetKeyboardMapping(_dp.get(), keyCodeBegin, keyCodeEnd - keyCodeBegin + 1, &symbolsPerCode);

  XModifierKeymap* mods = XGetModifierMapping(_dp.get());

  modTable.resize(Mod5MapIndex + 1);

  for (int index = 0; index < 8; index++) {
    modTable[index] = 0;

    for (int key = 0; key < mods->max_keypermod; key++) {
      int keycode = mods->modifiermap[index * mods->max_keypermod + key];

      if (keycode != 0) {
        modTable[index] = keycode;
        break;
      }
    }
  }

  for (int index = Mod1MapIndex; index <= Mod5MapIndex; index++) {
    if (modTable[index]) { // use XGetKeyboardMapping instead
      KeySym kSymbol = XkbKeycodeToKeysym(_dp.get(), modTable[index], 0, 0);
      switch (kSymbol) {
        case XK_Alt_R:
          altIdxRight = index;
          break;
        case XK_Alt_L:
          altIdxLeft = index;
          break;
      }
    }
  }

  if (mods)
    XFreeModifiermap(mods);
}

//----------------------------------------------------------------------------------------------------------------------

int VirtualKeyGenerator::checkIfShiftNeeded(KeyCode code, KeySym symbol) {
  if (XkbKeycodeToKeysym(_dp.get(), code, 0, 0) != symbol) {
    if (XkbKeycodeToKeysym(_dp.get(), code, 0, 1) == symbol)
      return 1;
    else
      return -1;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------------------------------------------------

VirtualKeyGenerator::KeyEvent VirtualKeyGenerator::generateEvent(const gunichar key) {
  // Somehow control key need special treatment.
  if (g_unichar_iscntrl(key)) {
    return KeyEvent(_dp, {0, 0, 0, 0}, XKeysymToKeycode(_dp.get(), XK_Return), 0);
  }
  gunichar keyCopy = key;
  if (keyCopy > 0x00ff)
    keyCopy = keyCopy | 0x01000000;

  KeySym kSymbol = (KeySym)keyCopy;
  KeyCode kCode = 0;
  int modifierState = 0;
  if ((kCode = XKeysymToKeycode(_dp.get(), kSymbol)) != 0) {
    switch (checkIfShiftNeeded(kCode, kSymbol)) {
      case 1:
        modifierState |= ModShift;
        break;
      case 0:
        modifierState &= ~ModShift;
        break;
      case -1:
        kCode = 0;
        break;
    }
  }

  if (kCode == 0) {
    // we modify only the last symbol, hopefully it's not used
    symbols[(keyCodeEnd - keyCodeBegin - 1) * symbolsPerCode] = kSymbol;

    XChangeKeyboardMapping(_dp.get(), keyCodeBegin, symbolsPerCode, symbols, (keyCodeEnd - keyCodeBegin));

    XSync(_dp.get(), 0);

    // This is a hack w/o any guarantees.
    // Seems to work, though.
    kCode = keyCodeEnd - 1;

    // we better try to force the mapping notify and refresh the keyboard mapping
    XKeysymToKeycode(_dp.get(), XK_F1);
    XEvent event;
    while (1) {
      XNextEvent(_dp.get(), &event);
      if (event.type == MappingNotify) {
        XMappingEvent* e = (XMappingEvent*)&event;
        XRefreshKeyboardMapping(e);
        break;
      }
    }

    switch (checkIfShiftNeeded(kCode, kSymbol)) {
      case 1:
        modifierState |= ModShift;
        break;
      case 0:
        modifierState &= ~ModShift;
        break;
      case -1:
        kCode = 0;
        break;
    }
  }
  if (kCode != 0) {
    ModKeysCodes m;
    m.AltRight = modTable[altIdxRight];
    m.AltLeft = modTable[altIdxLeft];
    m.Shift = modTable[ShiftMapIndex];
    m.Control = modTable[ControlMapIndex];
    return KeyEvent(_dp, m, kCode, modifierState);
  }

  return KeyEvent(_dp, {0, 0, 0, 0}, 0, 0);
}
