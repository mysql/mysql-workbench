/* 
 * Copyright (c) 2004, 2011, Oracle and/or its affiliates. All rights reserved.
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

/**
 * @file  myx_grt_lua_shell.c
 * @brief Lua Shell Frontend
 * 
 * See also: <a href="../grt.html#LuaShell">GRT Lua Shell</a>
 */

#include "stdafx.h"

#ifdef ENABLE_LUA_MODULES

#include "base/string_utilities.h"
#include "grtpp_shell_lua.h"

#include <math.h>

#ifdef _WIN32
  #include <Windows.h>
#endif


using namespace grt;
using namespace base;

LuaShell::LuaShell(GRT *grt)
  : Shell(grt), _loader(0)
{
}


lua_State *LuaShell::get_lua()
{
  return _loader->get_lua_context()->get_lua();
}


void LuaShell::init()
{
  _loader= dynamic_cast<LuaModuleLoader*>(_grt->get_module_loader(LanguageLua));
  if (!_loader)
    throw std::runtime_error("Lua module loader not initialized");

  lua_State *lua= get_lua();

  if (lua_gettop(lua) != 0)
    throw std::logic_error("Internal error in Lua context. Unexpected stack state");

  //already done in the loader initializer
  //myx_lua_register_functions(grt, lua);
  
  _loader->get_lua_context()->refresh();

  lua_pushstring(lua, MYX_SHELL_CURNODE);
  _loader->get_lua_context()->push_wrap_value(_grt->root());
  lua_settable(lua, LUA_GLOBALSINDEX);

  if (lua_gettop(lua) != 0)
    throw std::logic_error("Internal error in Lua context. Unexpected stack state");
}


void LuaShell::print_welcome()
{
  print(strfmt("MySQL Generic Runtime Environment %s\n", GRT_VERSION));

  if (_disable_quit)
    print("\nType 'help' or '?' for help.\n");
  else
    print("Type 'help' or '?' for help. Type 'quit' to exit the shell.\n");
  print("Welcome to the Lua Shell. (Use Preferences -> General to set language)\n");
}


std::string LuaShell::get_prompt()
{
  std::string cwd= _loader->get_lua_context()->get_cwd();

  if (!_current_line.empty())
    return cwd+">>";
  else
    return cwd+" >";
}



void LuaShell::report_lua_error(int status)
{
  if (status != 0)
  {
    const char *msg= lua_tostring(get_lua(), -1);
    print(strfmt("luart: error: %s\n", msg));
    lua_pop(get_lua(), 1);
  }
}


int LuaShell::execute_line(const std::string &linebuf)
{
  return _loader->get_lua_context()->run_script(linebuf, &_current_line);
}



int LuaShell::run_file(const std::string &file_name, bool interactive)
{
  return _loader->get_lua_context()->run_file(file_name, interactive);
}



std::vector<std::string> LuaShell::complete_line(const std::string &line, std::string &completed)
{
  lua_State *lua= get_lua();
  std::vector<std::string> tokens;
  std::string prefix= line;
  int top;

  lua_getglobal(lua, "_G");
  top= lua_gettop(lua);

  lua_pushnil(lua);
  while (lua_next(lua, top) != 0)
  {
    if (lua_isstring(lua, -2))
    {
      const char *s= lua_tostring(lua, -2);
      if (s[0] != '_')
      {
        if (g_str_has_prefix(s, prefix.c_str()))
          tokens.push_back(s);

        if (lua_istable(lua, -1))
        {
          lua_pushnil(lua);
          while (lua_next(lua, -2) != 0)
          {
            if (lua_isstring(lua, -2))
            {
              const char *m= lua_tostring(lua, -2);
              std::string token= strfmt("%s.%s", s, m);
              if (m[0] != '_' && g_str_has_prefix(token.c_str(), prefix.c_str()))
                tokens.push_back(token);
            }
            lua_pop(lua, 1);
          }
        }
      }
    }
    lua_pop(lua, 1);
  }
  lua_pop(lua, 1);
  
  g_assert(lua_gettop(lua)==0);

  if (tokens.size() == 1)
  {
    completed= tokens[0];
    tokens.clear();
  }
  
  return tokens;
}



ValueRef LuaShell::get_global_var(const std::string &var_name)
{
  ValueRef value;

  lua_getglobal(get_lua(), var_name.c_str());
  if (!lua_isnil(get_lua(), -1))
    value= _loader->get_lua_context()->pop_value();
  else
    lua_pop(get_lua(), 1);

  return value;
}


int LuaShell::set_global_var(const std::string &var_name, const ValueRef &value)
{
  _loader->get_lua_context()->push_wrap_value(value);
  lua_setglobal(get_lua(), var_name.c_str());
  
  return 1;
}


extern void myx_grt_shell_show_help(grt::GRT *grt, const char *command);

void LuaShell::show_help(const std::string &keyword)
{
  ::myx_grt_shell_show_help(_grt, keyword.c_str());
}


#endif
