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
