/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
 *
 */

#pragma once

#include "common.h"
#include "base/utf8string.h"
#include <map>

namespace mtemplate {

  struct ModifierAndArgument {
    base::utf8string _name;
    base::utf8string _arg;
  };

  struct Modifier;
  MTEMPLATELIBRARY_PUBLIC_FUNC extern std::map<base::utf8string, Modifier *> UserModifierMap;

  struct MTEMPLATELIBRARY_PUBLIC_FUNC Modifier {
    virtual ~Modifier();

    virtual base::utf8string modify(const base::utf8string &input, const base::utf8string arg = "") = 0;

    template <typename T>
    static void addModifier(const base::utf8string &name) {
      if (UserModifierMap.find(name) != UserModifierMap.end())
        delete UserModifierMap[name];

      UserModifierMap[name] = new T;
    }
  };

  struct MTEMPLATELIBRARY_PUBLIC_FUNC Modifier_HtmlEscape : public Modifier {
    virtual base::utf8string modify(const base::utf8string &input, const base::utf8string arg = "");
  };

  struct MTEMPLATELIBRARY_PUBLIC_FUNC Modifier_XmlEscape : public Modifier {
    virtual base::utf8string modify(const base::utf8string &input, const base::utf8string arg = "");
  };

  MTEMPLATELIBRARY_PUBLIC_FUNC Modifier *GetModifier(const base::utf8string &name);

} //  namespace mtemplate
