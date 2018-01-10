/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MDC_EVENTS_H_
#define _MDC_EVENTS_H_

//#include "mdc_common.h"
#include <string>

namespace mdc {

  enum EventState {
    SNone = 0,
    SLeftButtonMask = (1 << 0),
    SMiddleButtonMask = (1 << 1),
    SRightButtonMask = (1 << 2),

    SButtonMask = 0x0f,

    SShiftMask = (1 << 8),
    SControlMask = (1 << 9),
    SAltMask = (1 << 10),
    SOptionMask = SAltMask,
    SCommandMask = (1 << 11),

    SModifierMask = (SShiftMask | SControlMask | SAltMask | SOptionMask | SCommandMask),

    SLeaveMask = (1 << 16),
    SEnterMask = (1 << 17)
  };

  enum KeyCode {
    KNone = 0,

    KEscape,
    KTab = '\t',
    KEnter = '\n',
    KSpace = ' ',
    KPeriod = '.',
    KComma = ',',
    KSemicolon = ';',
    KPlus = '+',
    KMinus = '-',

    KShift = 0xff00,
    KAlt,
    KControl,
    KOption,
    KCommand,

    KF1,
    KF2,
    KF3,
    KF4,
    KF5,
    KF6,
    KF7,
    KF8,
    KF9,
    KF10,
    KF11,
    KF12,

    KLeft,
    KRight,
    KUp,
    KDown,
    KHome,
    KEnd,
    KPageUp,
    KPageDown,

    KInsert,
    KDelete,
    KBackspace

  };

  struct KeyInfo {
    KeyCode keycode;
    std::string string;

    bool operator==(const KeyInfo& other) const {
      if (keycode != 0 && keycode == other.keycode)
        return true;
      return string == other.string;
    }
  };

  inline EventState operator&(EventState s1, EventState s2) {
    return (EventState)((int)s1 & (int)s2);
  }

  inline EventState operator|(EventState s1, EventState s2) {
    return (EventState)((int)s1 | (int)s2);
  }

  enum MouseButton { ButtonLeft = 0, ButtonMiddle = 1, ButtonRight = 2 };

  inline EventState operator-(EventState s, MouseButton b) {
    return (EventState)((int)s & ~(1 << (int)b));
  }

  inline EventState operator+(EventState s, MouseButton b) {
    return (EventState)((int)s | (1 << (int)b));
  }
};

#endif
