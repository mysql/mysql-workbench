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
#include "modifier.h"
#include "base/string_utilities.h"

#include <map>

namespace mtemplate{

std::map<std::string, Modifier *> StockModifierMap;
std::map<std::string, Modifier *> UserModifierMap;


Modifier *GetModifier(const std::string &name)
{
  if (base::hasPrefix(name, "x-"))
  {
    std::string user_name = name.substr(2);
    if (UserModifierMap.find(user_name) != UserModifierMap.end())
      return UserModifierMap[user_name];
  }
  else
  {
    if (StockModifierMap.find(name) != StockModifierMap.end())
      return StockModifierMap[name];
  }
  
  return NULL;
}

Modifier::~Modifier()
{
}

std::string Modifier_HtmlEscape::modify(const std::string& input, const std::string arg) 
{
  std::string result;
  for (std::string::const_iterator iter = input.begin(); iter != input.end(); ++iter)
  {
    switch (*iter)
    {
      case '&' : result += "&amp;"; break;
      case '\"': result += "&quot;"; break;
      case '\'': result += "&#39;"; break;
      case '<' : result += "&lt;"; break;
      case '>' : result += "&gt;"; break;
        
      case '\r': 
      case '\n': 
      case '\v': 
      case '\f': 
      case '\t': 
        result += " ";
    }
  }
  
  return result;
}


std::string Modifier_XmlEscape::modify(const std::string& input, const std::string arg) 
{
  std::string result;
  for (std::string::const_iterator iter = input.begin(); iter != input.end(); ++iter)
  {
    switch (*iter)
    {
      case '&' : result += "&amp;"; break;
      case '\"': result += "&quot;"; break;
      case '\'': result += "&#39;"; break;
      case '<' : result += "&lt;"; break;
      case '>' : result += "&gt;"; break;
        
      case '\r': 
      case '\n': 
      case '\v': 
      case '\f': 
      case '\t': 
        result += " ";
        break;
      default:
        result += *iter;
        break;
    }
  }
  
  return result;
}


}   //  namespace mtemplate