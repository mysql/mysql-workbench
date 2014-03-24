/* 
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
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


#ifndef _GRT_LUA_CONTEXT_H_
#define _GRT_LUA_CONTEXT_H_

#include "grtpp.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
};

namespace grt {
  const std::string LanguageLua= "lua";

class MYSQLGRT_PUBLIC LuaContext
{
public:
  LuaContext(GRT *grt);
  ~LuaContext();

  ValueRef pop_value(int pos= -1);

  int push_wrap_value(const ValueRef &value);
  int push_convert_value(const ValueRef &value);
  int push_list_items(const BaseListRef &list);
  int push_and_wrap_if_not_simple(const ValueRef &value);
  void pop_args(const char *format, ...);

  int run_file(const std::string &file, bool interactive);
  int run_script(const std::string &script, std::string *line_buffer=0);
  
  int call_grt_function(const std::string &module, const std::string &function,
                        const BaseListRef &args);
  
  
  lua_State *get_lua() const { return _lua; }
  operator lua_State *() const { return _lua; }

  static LuaContext *get(lua_State *l);

  int refresh();
  void register_grt_functions();
  
  void dump_stack();
  
  void print_value(const ValueRef &value);
  
  void call_gc();

  GRT *get_grt() const { return _grt; }

  bool set_cwd(const std::string &path);
  std::string get_cwd() const { return _cwd; }
  int load_file(const std::string& path);

protected:
  GRT *_grt;
  lua_State *_lua;
  
  std::string _cwd;

  ValueRef pop_grt_udata(int pos= -1);

  int add_module_to_table(Module *module, int tbl);
};

};

#endif
