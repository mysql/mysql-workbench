/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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


#include "stdafx.h"
#include "stdio.h"

#include "grtpp_module_lua.h"
#include "base/string_utilities.h"

#include "base/file_functions.h" // For reading files with non-ANSI path names.
#include "base/file_utilities.h"

using namespace grt;

#ifdef ENABLE_LUA_MODULES


extern int l_log_error(lua_State *lua);
extern int l_log_warning(lua_State *lua);
extern int l_log_message(lua_State *lua);




LuaModule::LuaModule(LuaModuleLoader *loader)
  : Module(loader)
{
}


LuaModule::~LuaModule()
{
  std::string lua_function_table_name= base::strfmt("__%s_lua", _name.c_str());
  lua_State *l= ((LuaModuleLoader*)_loader)->get_lua_context()->get_lua();

  // delete the module data from the lua context
  lua_pushstring(l, lua_function_table_name.c_str());
  lua_pushnil(l);
  lua_settable(l, LUA_GLOBALSINDEX);
}


//--------------------------------------------------------------------------------

LuaModuleLoader::LuaModuleLoader(GRT *grt)
  : ModuleLoader(grt), _lua(grt)
{
  // register a global __GRT variable containing the grt ptr
  {
    LuaModuleLoader **userdata;
    userdata= (LuaModuleLoader**)lua_newuserdata(_lua, sizeof(LuaModuleLoader*));
    *userdata= this;
    luaL_newmetatable(_lua, "MYX_GRT");
    lua_setmetatable(_lua, -2);

    lua_setglobal(_lua, "__GRT");
  }

  lua_gc(_lua, LUA_GCSTOP, 0);  /* stop collector during initialization */
  luaL_openlibs(_lua);
  lua_gc(_lua, LUA_GCRESTART, 0);

  // register logging functions
  lua_register(_lua, "logerror", l_log_error);
  lua_register(_lua, "logwarning", l_log_warning);
  lua_register(_lua, "logmessage", l_log_message);

  _lua.register_grt_functions();
  
  g_assert(lua_gettop(_lua) == 0);
}


LuaModuleLoader::~LuaModuleLoader()
{
}


static void lua_push_fallback_table(lua_State *l)
{
  lua_newtable(l);
  lua_pushstring(l, "__index");
  lua_getglobal(l, "_G");
  lua_settable(l, -3);
}

//--------------------------------------------------------------------------------------------------

Module *LuaModuleLoader::init_module(const std::string &path)
{
  std::string module_name, extends;
  std::string meta_version, meta_author, meta_description;
  char **implements;
  std::string lua_function_table_name;
  ValueRef module_info;
  int status;

  // create a new table which will be the environment for the
  // loaded module
  lua_pushstring(_lua, "___tmp");
  lua_newtable(_lua);
  lua_settable(_lua, LUA_GLOBALSINDEX);

  // set the global environment as a fallback for the module environment
  lua_getglobal(_lua, "___tmp");
  lua_push_fallback_table(_lua);
  lua_setmetatable(_lua, -2);
  lua_pop(_lua, 1); // pop ___tmp

  // load the module
  status= get_lua_context()->load_file(path);
  if (status != 0)
  {
    _grt->send_error(std::string("Could not load lua module ").append(path).append(": ").append(lua_tostring(_lua, -1)));

    lua_pop(_lua, 1);
    return 0;
  }

  // fetch the new environment table
  lua_getglobal(_lua, "___tmp");

  // sets it as the environment for the loaded module
  lua_setfenv(_lua, -2);

  // execute the module, so that function declarations in it get executed
  status= lua_pcall(_lua, 0, 0, 0);

  if (status != 0)
  {
    _grt->send_warning(std::string("error executing lua module ").append(path).append(": ").append(lua_tostring(_lua, -1)));
    lua_pop(_lua, 1);
    return 0;
  }

  // get module info
  lua_getglobal(_lua, "___tmp");
  lua_pushstring(_lua, "getModuleInfo");
  lua_gettable(_lua, -2);
  status= lua_pcall(_lua, 0, 1, 0);

  if (status != 0)
  {
    _grt->send_warning(std::string("error calling getModuleInfo() in lua module ").append(path).
                       append(": ").append(lua_tostring(_lua, -1)));
    lua_pop(_lua, 2);
    return 0;
  }

  module_info= _lua.pop_value();

  if (!module_info.is_valid() || module_info.type() != DictType)
  {
    _grt->send_warning(std::string("invalid return value calling getModuleInfo() in lua module ").append(path));
    lua_pop(_lua, 1);
    return 0;
  }

  lua_pop(_lua, 1); // pop ___tmp

  DictRef module_info_dict(DictRef::cast_from(module_info));
  
  // get module data
  module_name= module_info_dict.get_string("name");
  lua_function_table_name.append("__").append(module_name).append("_lua");

  extends= module_info_dict.get_string("extends");
  
  implements= g_strsplit(module_info_dict.get_string("implements").c_str(), ",", -1);

  BaseListRef module_functions= BaseListRef::cast_from(module_info_dict.get("functions"));

  if (module_name.empty() || !module_functions.is_valid())
  {
    _grt->send_warning(std::string("bad info returned from getModuleInfo() in lua module ").append(path)
                       .append(lua_tostring(_lua, -1)));
    return 0;
  }

  meta_version= module_info_dict.get_string("version");
  meta_author= module_info_dict.get_string("author");
  meta_description= module_info_dict.get_string("description");

  // rename the ___tmp module table to the definitive name
  lua_pushstring(_lua, lua_function_table_name.c_str());
  lua_getglobal(_lua, "___tmp");
  lua_settable(_lua, LUA_GLOBALSINDEX);

  // !!! temporary workaround
  lua_pushstring(_lua, "___tmp");
  lua_pushnil(_lua);
  lua_settable(_lua, LUA_GLOBALSINDEX);

  // init internal module descriptor
  LuaModule *lmodule= new LuaModule(this);

  lmodule->_name= module_name;
  lmodule->_path= path;
  for (size_t c= module_functions.count(), i= 0; i < c; i++)
  {
    lmodule->add_parse_function_spec(StringRef::cast_from(module_functions[i]),
                                     boost::bind(&LuaModuleLoader::call_function, this, _1, _2, _3));
  }
  lmodule->_extends= extends;
  lmodule->_meta_author= meta_author;
  lmodule->_meta_version= meta_version;
  lmodule->_meta_description= meta_description;
  {
    if (g_str_has_suffix(base::dirname(path).c_str(), ".mwbplugin"))
      lmodule->_is_bundle= true;
  }

  for (char **impl= implements; *impl; ++impl)
    lmodule->_interfaces.push_back(*impl);
  
  g_strfreev(implements);
  
  if (_grt->verbose())
    g_message("Initialized Lua module %s (%s)", path.c_str(), module_name.c_str());

  g_assert(lua_gettop(_lua)==0);

  return lmodule;
}


void LuaModuleLoader::refresh()
{
  _lua.refresh();
}


static int call_traceback(lua_State *lua)
{
  lua_getfield(lua, LUA_GLOBALSINDEX, "debug");
  g_assert(lua_istable(lua, -1));
  lua_getfield(lua, -1, "traceback");
  g_assert(lua_isfunction(lua, -1));
  lua_pushvalue(lua, 1); // pass error msg
  lua_pushinteger(lua, 2); // skip 
  lua_call(lua, 2, 1); // call traceback

  return 1;
}


ValueRef LuaModuleLoader::call_function(const BaseListRef &args,
    Module *module, const Module::Function &function)
{
  std::string lua_function_table_name;
  int rc;
  int error_func;
  int argc;

  // add traceback function
  lua_pushcfunction(_lua, call_traceback);
  error_func= lua_gettop(_lua);

//  if (getenv("GRT_VERBOSE"))
//    g_message("Calling lua function %s.%s", function->module->name, function->name);

  lua_checkstack(_lua, lua_gettop(_lua)+5);

  lua_function_table_name.append("__").append(module->_name).append("_lua");

  lua_getglobal(_lua, lua_function_table_name.c_str());
  if (lua_isnil(_lua, -1))
  {
    g_warning("Error calling %s.%s, Lua module environment not found", module->_name.c_str(), function.name.c_str());
  }
  lua_pushstring(_lua, function.name.c_str());
  lua_gettable(_lua, -2);
  lua_remove(_lua, -2);
  if (args.is_valid())
  {
    argc= _lua.push_list_items(args);
  }
  else
  {
    argc= 0;
  }

  rc = lua_pcall(_lua, argc, 1, error_func);

  // lua functions return the return value directly
  // errors must be raised with the error() lua function
  
  ValueRef retval;
  
  if (rc != 0)
  {
    std::string msg("Error callig lua function ");
    
    if (rc == LUA_ERRMEM)
      msg.append(module->_name).append(".").append(function.name).
        append(": out of memory");
    else
      msg.append(module->_name).append(".").append(function.name).
      append(": '").append(lua_tostring(_lua, -1)).append("\n").append("'");

    lua_pop(_lua, 2); // pop error and error_func

    throw grt::module_error(msg);
  }
  else
  {
    retval= _lua.pop_value();
  }
  
  lua_pop(_lua, 1); // pop the traceback function

  g_assert(lua_gettop(_lua) == error_func-1);

  return retval;
}


bool LuaModuleLoader::load_library(const std::string &file)
{
  return _lua.run_file(file, false) == 0;
}


bool LuaModuleLoader::run_script_file(const std::string &path)
{
  return _lua.run_file(path, false) == 0;
}

bool LuaModuleLoader::run_script(const std::string &script)
{
  return _lua.run_script(script, 0) == 0;
}

bool LuaModuleLoader::check_file_extension(const std::string &path)
{
  return g_str_has_suffix(path.c_str(), ".lua") != 0;
}


#endif // ENABLE_LUA_MODULES
