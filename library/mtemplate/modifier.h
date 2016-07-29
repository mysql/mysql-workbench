/*
 * Copyright (c) 2007, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include <string>
#include <map>

namespace mtemplate {

struct ModifierAndArgument
{
  std::string _name;
  std::string _arg;
};

struct Modifier;
extern std::map<std::string, Modifier *> UserModifierMap;

struct Modifier
{
  virtual ~Modifier();
  
  virtual std::string modify(const std::string &input, const std::string arg = "") = 0;

  template <typename T>
  static void add_modifier(const std::string &name)
  {
    if (UserModifierMap.find(name) != UserModifierMap.end())
      delete UserModifierMap[name];

    UserModifierMap[name] = new T;
  }
};

struct Modifier_HtmlEscape : public Modifier
{
  virtual std::string modify(const std::string &input, const std::string arg = "");
};

struct Modifier_XmlEscape : public Modifier
{
  virtual std::string modify(const std::string &input, const std::string arg = "");
};

Modifier *GetModifier(const std::string &name);


}   //  namespace mtemplate

