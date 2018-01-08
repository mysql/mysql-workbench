/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/geometry.h"

namespace base {

  struct BASELIBRARY_PUBLIC_FUNC Accessible {
  public:
    enum Role {
      RoleNone,
      Window,
      Pane,
      Link,
      List,
      ListItem,
      PushButton,
      StaticText,
      Text,
      Outline,
      OutlineButton,
      OutlineItem
    };

    virtual ~Accessible();

    // Name + role are mandatory.
    virtual std::string getAccessibilityName() = 0;
    virtual Role getAccessibilityRole() = 0;

    // The rest of the accessible methods are optional, but it is strongly recommended to implement them
    // in all descendants for property accessibility + testing support.
    virtual std::string getAccessibilityTitle();
    virtual std::string getAccessibilityDescription();
    virtual std::string getAccessibilityValue();
    virtual size_t getAccessibilityChildCount();
    virtual Accessible* getAccessibilityChild(size_t index);
    virtual base::Rect getAccessibilityBounds();
    virtual Accessible* accessibilityHitTest(ssize_t x, ssize_t y);
    virtual std::string getAccessibilityDefaultAction();
    virtual void accessibilityDoDefaultAction();
    virtual void accessibilityShowMenu();
  };

}
