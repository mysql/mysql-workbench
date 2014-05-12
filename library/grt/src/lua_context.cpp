/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "lua_context.h"

#include "base/util_functions.h"

#include "grtpp_shell.h"
#include "grtpp_util.h"
#include "base/string_utilities.h"
#include "base/file_functions.h"

#include <lauxlib.h>
#include <lualib.h>

#include <math.h>

using namespace grt;
using namespace base;

static void mlua_remove(lua_State *lua, int index)
{
  if (index == -1)
    lua_pop(lua, 1);
  else if (index < 0)
    throw std::invalid_argument("invalid stack index");
  else
    lua_remove(lua, index);
}


LuaContext::LuaContext(GRT *grt)
  : _grt(grt)
{
  _lua= luaL_newstate();

  _cwd= "/";
  
  {
    LuaContext **userdata;
    userdata= (LuaContext**)lua_newuserdata(_lua, sizeof(LuaContext*));
    *userdata= this;
    luaL_newmetatable(_lua, "LUA_CONTEXT");
    lua_setmetatable(_lua, -2);

    lua_setglobal(_lua, "__LUA_CONTEXT");
  }
}


LuaContext::~LuaContext()
{
  lua_close(_lua);
}


LuaContext *LuaContext::get(lua_State *l)
{
  LuaContext **ctx;
  lua_getglobal(l, "__LUA_CONTEXT");

  ctx= (LuaContext**)luaL_checkudata(l, -1, "LUA_CONTEXT");
  if (ctx)
  {
    lua_pop(l, 1);
    return *ctx;
  }
  return NULL;
}


bool LuaContext::set_cwd(const std::string &path)
{
  std::string new_path= Shell::get_abspath(_cwd, path);

  ValueRef value= _grt->get(new_path);

  if (!value.is_valid())
    return false;
  
  _cwd= new_path;

  // set a variable to hold the current object
  lua_pushstring(_lua, MYX_SHELL_CURNODE);
  push_wrap_value(value);
  lua_settable(_lua, LUA_GLOBALSINDEX);
  
  return true;
}


void LuaContext::call_gc()
{
  lua_gc(_lua, LUA_GCCOLLECT, 0);
}


void LuaContext::print_value(const ValueRef &value)
{
  if (!value.is_valid())
  {
    _grt->send_output("NULL\n");
    return;
  }

  _grt->send_output(value.repr()+"\n");
}


int LuaContext::run_file(const std::string &file_name, bool interactive)
{
  int status= luaL_loadfile(_lua, file_name.c_str());
  int rc;

  if (interactive)
    _grt->send_output(strfmt("Opening script file %s ...\n", file_name.c_str()));

  if (status)
  {
    _grt->send_output(strfmt("Error in file: %s\n", lua_tostring(_lua, -1)));
    lua_pop(_lua, 1);
    return -1;
  }

  if (interactive)
    _grt->send_output(strfmt("Executing script file %s ...\n\n", file_name.c_str()));

  status= lua_pcall(_lua, 0, LUA_MULTRET, 0);
  if (status)
  {
    _grt->send_output(strfmt("error executing script: %s\n", lua_tostring(_lua, -1)));
    lua_pop(_lua, 1);
    while (lua_gettop(_lua) > 0)
    {
      _grt->send_output(strfmt("    %s\n", lua_tostring(_lua, -1)));
      lua_pop(_lua, 1);
    }
    rc= -2;
  }
  else
    rc= 0;

  if ((rc == 0) && (interactive))
    _grt->send_output("\nExecution finished.\n");

  g_assert(lua_gettop(_lua)==0);
  
  return rc;
}


int LuaContext::run_script(const std::string &script, std::string *line_buffer)
{  
  int rc= 0;
  int status= 0;
  
  g_assert(lua_gettop(_lua) == 0);
  
  if (line_buffer)
    line_buffer->append(script);
  
  status= luaL_loadbuffer(_lua, line_buffer ? line_buffer->c_str() : script.c_str(), line_buffer ? line_buffer->length() : script.length(), "=stdin");
  if (line_buffer && status == LUA_ERRSYNTAX && strstr(lua_tostring(_lua, -1), "near `<eof>'"))
  {
    // line is continued
    lua_pop(_lua, 1);
    return 1;
  }
  
  if (status == 0)
    status= lua_pcall(_lua, lua_gettop(_lua)-1, 0, 0);
  else
    rc= -1;
  
  if (line_buffer)
    line_buffer->clear();
  
  if (status!=0)
  {
    rc= -1;
    const char *msg= lua_tostring(_lua, -1);
    _grt->send_output(strfmt("luart: error: %s\n", msg));
    lua_pop(_lua, 1);
  }
  while (lua_gettop(_lua) > 0)
  { // print stack contents
    lua_getglobal(_lua, "print");
    lua_insert(_lua, 1);
    if (lua_pcall(_lua, lua_gettop(_lua)-2, 0, 0) != 0)
    {
      _grt->send_output(strfmt("luart: error calling print (%s)\n",
                   lua_tostring(_lua, -1)));
    }
  }
  
  g_assert(lua_gettop(_lua)==0);
  
  return rc;
}


//================================================================================
// Stack Manipulation Stuff

static const char *mlua_popstring(lua_State *l)
{
  const char *s= luaL_checkstring(l, -1);
  lua_pop(l, 1);
  return s;
}


static void *mlua_checkudata(lua_State *L, int ud, const char *tname) 
{ // copied from lua sources and removed the error throwing
  void *p = lua_touserdata(L, ud);

  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      lua_getfield(L, LUA_REGISTRYINDEX, tname);  /* get correct metatable */
      if (lua_rawequal(L, -1, -2)) {  /* does it have the correct mt? */
        lua_pop(L, 2);  /* remove both metatables */
        return p;
      }
      lua_pop(L, 2);
    }
  }

  return NULL; 
}


static internal::Value *luaL_checkgrtudata(lua_State *l, int index)
{
  internal::Value **value;
  value= (internal::Value**)mlua_checkudata(l, index, "MYX_GRT_VALUE");
  if (value)
    return *value;
  value= (internal::Value**)mlua_checkudata(l, index, "MYX_GRT_LIST");
  if (value)
    return *value;
  value= (internal::Value**)mlua_checkudata(l, index, "MYX_GRT_DICT");
  if (value)
    return *value;
  value= (internal::Value**)mlua_checkudata(l, index, "MYX_GRT_OBJECT");
  if (value)
    return *value;
  return NULL;
}


grt::ValueRef LuaContext::pop_value(int pos)
{
  ValueRef value;
  static int i= 0;

  i++;
  switch (lua_type(_lua, pos))
  {
  case LUA_TBOOLEAN:
    value= IntegerRef(lua_toboolean(_lua, pos));
    mlua_remove(_lua, pos);
    break;
  case LUA_TNUMBER:
    {
      lua_Number n= lua_tonumber(_lua, pos);
      if (n - floor(n) == 0)
        value= IntegerRef((long)n);
      else
        value= DoubleRef(n);
      mlua_remove(_lua, pos);
    }
    break;
  case LUA_TSTRING:
    value= StringRef(lua_tostring(_lua, pos));
    mlua_remove(_lua, pos);
    break;
  case LUA_TTABLE:
    {
      int tbl= pos < 0 ? lua_gettop(_lua) : pos;
      int can_be_list= 1;
      int empty= 1;
      unsigned int nexti;

      DictRef dict(_grt);
      BaseListRef list(_grt);

      lua_pushvalue(_lua, tbl);
      
      nexti= 1;
      // first we create a dict from the table, checking if the indices
      // are numeric and in sequence
      lua_pushnil(_lua);
      while (lua_next(_lua, -2) != 0)
      {
        ValueRef item_value;
        const char *item_key;
        
        // handle value
        item_value= pop_value();

        // handle key
        lua_pushvalue(_lua, -1);
        item_key= lua_tostring(_lua, -1);
        
        dict.set(item_key, item_value);
        list.ginsert(item_value);
        
        lua_pop(_lua, 1);

        if (lua_type(_lua, -1) != LUA_TNUMBER || lua_tonumber(_lua, -1) != nexti)
          can_be_list= 0;
        
        nexti++;
        empty= 0;
        // don't pop key, as it should be kept for lua_next() to fetch the
        // next item
      }
      lua_pop(_lua, 1); // pop lua_pushvalue
      mlua_remove(_lua, pos); // pop argument

      if (empty || can_be_list)
        value= list;
      else
        value= dict;
    }
    break;

  case LUA_TNIL:
    mlua_remove(_lua, pos);
    break;

  case LUA_TUSERDATA:
   value= pop_grt_udata(pos);
   break;

  case LUA_TFUNCTION:
  case LUA_TLIGHTUSERDATA:
  case LUA_TTHREAD:
    g_warning("Invalid data (type=%s) in a Lua result value",
              lua_typename(_lua, lua_type(_lua, pos)));
    mlua_remove(_lua, pos);
    break;
  }
  
  --i;

  return value;  
}



// metamethods for grt values

static int gc_function(lua_State *l)
{
  internal::Value *value= luaL_checkgrtudata(l, 1);
  if (value)
  {
    value->release();
  }
  return 0;
}

static int gc_equals(lua_State *l)
{
  internal::Value *value1= luaL_checkgrtudata(l, 1);
  internal::Value *value2= luaL_checkgrtudata(l, 2);
  lua_pop(l, 2);
  return value1 == value2;
}


static int list_index_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  BaseListRef list;
  int index;

  ctx->pop_args("Li", &list, &index);

  index--;
  if (index >= (long)list.count())
    luaL_error(l, "List index out of bounds");
  if (index < 0)
    luaL_error(l, "List index starts at 1");
  ctx->push_wrap_value(list.get(index));
  
  return 1;
}

static int list_newindex_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  BaseListRef list;
  ValueRef value;
  long index;
  
  ctx->pop_args("LiG", &list, &index, &value);

  index--;
  if (index <= 0 || index == (long)list.count())
    list.ginsert(value);
  else
    list.gset(index, value);

  return 0;
}


static int list_len_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  BaseListRef list;

  // needed because there's something unidentified in the top of the stack
  lua_pop(l, 1);

  ctx->pop_args("L", &list);

  lua_pushinteger(l, list.count());

  return 1;
}



static int dict_index_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  DictRef dict;
  ValueRef value;
  const char *member;

  ctx->pop_args("DS", &dict, &member);
  
  value= dict.get(member);
  
  if (!value.is_valid())
    lua_pushnil(l);
  else
    ctx->push_and_wrap_if_not_simple(value);

  return 1;
}

static int dict_newindex_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  DictRef dict;
  ValueRef value;
  const char *member;

  ctx->pop_args("DSG", &dict, &member, &value);

  dict.set(member, value);
    
  return 0;
}

static int dict_len_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  DictRef dict;

  // needed because there's something unidentified in the top of the stack
  lua_pop(l, 1);

  ctx->pop_args("D", &dict);

  lua_pushinteger(l, dict.count());
  
  return 1;
}



static int call_object_method(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);

  // push the object and the method name in the upvalue stack to the normal stack
  lua_pushvalue(l, lua_upvalueindex(1));
  lua_pushvalue(l, lua_upvalueindex(2));
  MetaClass *meta;
  ValueRef dvalue;

  std::string member= lua_tostring(l, -1);
  lua_pop(l, 1);  
  ObjectRef obj(ObjectRef::cast_from(ctx->pop_value()));

  meta= obj.get_metaclass();

  std::string arg_fmt;

  // pop the rest of the args checking the type and # against the
  // definition in the metaclass, creating the arglist to be passed
  const MetaClass::Method *method= meta->get_method_info(member);
  if (!method)
    throw std::logic_error("internal inconsistency");

  arg_fmt= "O"; // the object is left on the stack when this function is entered
  // add args of the method to the arg format to be popped
  // they'll be put in a list
  ArgSpecList args= method->arg_types;
  if (!args.empty())
  {
    arg_fmt.append("(");
    for (ArgSpecList::const_iterator it= args.begin(); it != args.end(); ++it)
    {
      switch (it->type.base.type)
      {
      case IntegerType:
        arg_fmt.append("i");
        break;
      case DoubleType:
        arg_fmt.append("f");
        break;
      case StringType:
        arg_fmt.append("S");
        break;
      case ListType:
        arg_fmt.append("L");
        break;
      case DictType:
        arg_fmt.append("D");
        break;
      case ObjectType:
        arg_fmt.append("O");
        break;
      default:
        throw std::logic_error("unsupported type in grt object method");
      }
    }
    arg_fmt.append(")");
  }

  BaseListRef arglist(ctx->get_grt());

  //g_message("WILL POP %s", arg_fmt.c_str());
  //ctx->dump_stack();

  // pop list of args using the hand built arg fmt
  ctx->pop_args(arg_fmt.c_str(), &obj, &arglist);

  if (obj.is_valid() && !member.empty())
  {
    grt::ValueRef result;
    try
    {
      // call the object method with the args given in lua code
      result = meta->call_method(&obj.content(), member, arglist);
    }
    catch (std::exception &exc)
    {
      luaL_error(l, strfmt("error calling %s: %s", member.c_str(), exc.what()).c_str());
    }
      
    ctx->push_and_wrap_if_not_simple(result);
      
    return 1;
  }
  else
    luaL_error(l, "Invalid GRT object method call");
  return 0;
}


static int obj_index_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ObjectRef obj;
  ValueRef dvalue;
  const char *member;

  ctx->pop_args("OS", &obj, &member);
  
  if (obj.has_member(member))
  {
    dvalue= obj.get_member(member);
    if (!dvalue.is_valid())
      lua_pushnil(l);
    else
      ctx->push_and_wrap_if_not_simple(dvalue);
  }
  else if (obj.has_method(member))
  {
    // push a C closure to the stack containing the object and method
    // info plus a function that will call it
    ctx->push_wrap_value(obj);
    lua_pushstring(l, member);
    lua_pushcclosure(l, call_object_method, 2);
  }
  else
  {
    luaL_error(l, strfmt("Invalid object member '%s'", member).c_str());
  }
  
  return 1;
}


static int obj_newindex_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ObjectRef obj;
  ValueRef dvalue;
  const char *member;
  
  ctx->pop_args("OSG", &obj, &member, &dvalue);

  try
  {
    obj.set_member(member, dvalue);
  }
  catch (std::exception &exc)
  {
    luaL_error(l, "error setting object member: %s", exc.what());
  }
    
  return 0;
}


int LuaContext::push_wrap_value(const grt::ValueRef &value)
{
  internal::Value **ud_ptr;
  int idx,meta;
  
  if (!value.is_valid())
  {
    lua_pushnil(_lua);
    return 1;
  }
  
  ud_ptr= (internal::Value**)lua_newuserdata(_lua, sizeof(internal::Value*));
  idx= lua_gettop(_lua);
  *ud_ptr= value.valueptr();
  (*ud_ptr)->retain();

  // metatable with GC function and accessors for dicts and lists
  switch (value.type())
  {
    case ListType: luaL_newmetatable(_lua, "MYX_GRT_LIST"); break;
    case DictType: luaL_newmetatable(_lua, "MYX_GRT_DICT"); break;
    case ObjectType: luaL_newmetatable(_lua, "MYX_GRT_OBJECT"); break;
    default: luaL_newmetatable(_lua, "MYX_GRT_VALUE"); break;
  }
  meta= lua_gettop(_lua);
  lua_pushstring(_lua, "__gc");
  lua_pushcfunction(_lua, gc_function);
  lua_rawset(_lua, meta);

  lua_pushstring(_lua, "__eq");
  lua_pushcfunction(_lua, gc_equals);
  lua_rawset(_lua, meta);

  switch (value.type())
  {
  case ListType:
    lua_pushstring(_lua, "__index");
    lua_pushcfunction(_lua, list_index_function);
    lua_rawset(_lua, meta);
    lua_pushstring(_lua, "__newindex");
    lua_pushcfunction(_lua, list_newindex_function);
    lua_rawset(_lua, meta);
    lua_pushstring(_lua, "__len");
    lua_pushcfunction(_lua, list_len_function);
    lua_rawset(_lua, meta);
  break;
  case DictType:
    lua_pushstring(_lua, "__index");
    lua_pushcfunction(_lua, dict_index_function);
    lua_rawset(_lua, meta);
    lua_pushstring(_lua, "__newindex");
    lua_pushcfunction(_lua, dict_newindex_function);
    lua_rawset(_lua, meta);
    lua_pushstring(_lua, "__len");
    lua_pushcfunction(_lua, dict_len_function);
    lua_rawset(_lua, meta);
  break;
  case ObjectType:
    lua_pushstring(_lua, "__index");
    lua_pushcfunction(_lua, obj_index_function);
    lua_rawset(_lua, meta);
    lua_pushstring(_lua, "__newindex");
    lua_pushcfunction(_lua, obj_newindex_function);
    lua_rawset(_lua, meta);
    break;
  default:
    break;
  }
  lua_setmetatable(_lua, idx);

  return 1;
}


/** 
 ****************************************************************************
 * @brief push a GRT value into the Lua stack as a Lua value
 *
 *   Pushes a GRT value into stack of the given Lua environment, converting
 * the data to Lua types.
 * 
 * @param L  Lua state
 * @param value  GRT value
 *
 * @return number of objects in the stack
 ****************************************************************************
 */
int LuaContext::push_convert_value(const grt::ValueRef &value)
{
  if (value.is_valid())
  {
    switch (value.type())
    {
    case IntegerType:
      lua_checkstack(_lua, lua_gettop(_lua)+1);
      lua_pushinteger(_lua, *IntegerRef::cast_from(value));
      break;
    case StringType:
      lua_checkstack(_lua, lua_gettop(_lua)+1);
      lua_pushstring(_lua, StringRef::cast_from(value).c_str());
      break;
    case DoubleType:
      lua_checkstack(_lua, lua_gettop(_lua)+1);
      lua_pushnumber(_lua, *DoubleRef::cast_from(value));
      break;
    case ListType:
      {
        BaseListRef list(BaseListRef::cast_from(value));

        lua_checkstack(_lua, lua_gettop(_lua) + (int)list.count()*2+1);
        lua_newtable(_lua);
        for (int i=0, c= (int)list.count(); i < c; i++)
        {
          push_convert_value(list[i]);
          lua_rawseti(_lua, -2, i+1);
        }
      }
      break;
    case DictType:
      {
        DictRef dict(DictRef::cast_from(value));

        lua_checkstack(_lua, lua_gettop(_lua) + (int)dict.count()*2+1);
        lua_newtable(_lua);
        for (DictRef::const_iterator iter= dict.begin(); iter != dict.end(); ++iter)
        {
          lua_pushstring(_lua, iter->first.c_str());
          push_convert_value(iter->second);
          lua_rawset(_lua, -3);
        }
      }
      break;
    case ObjectType:
      push_wrap_value(value);
      break;
    case UnknownType: g_assert(0); break;
    }
  }
  else
    lua_pushnil(_lua);

  return 1;
}


int LuaContext::push_and_wrap_if_not_simple(const grt::ValueRef &value)
{
  if (!value.is_valid())
  {
    lua_pushnil(_lua);
    return 1;
  }
  else if (is_simple_type(value.type()))
    return push_convert_value(value);
  else
    return push_wrap_value(value);
}


int LuaContext::push_list_items(const grt::BaseListRef &list)
{
  int x= 0;

  for (size_t c= list.count(), i= 0; i < c; i++)
  {
    x+= push_and_wrap_if_not_simple(list[i]);
  }

  return x;
}

  
void LuaContext::dump_stack()
{
  int c= lua_gettop(_lua);
  int i;
  
  g_message("stack has %i items:", c);
  for (i= 1; i <= c; i++)
  {
    switch (lua_type(_lua, i))
    {
    case LUA_TSTRING:
      g_message("%i) %s (%s)", i,
                luaL_typename(_lua, i), lua_tostring(_lua, i));
      break;
    case LUA_TNUMBER:
      g_message("%i) %s (%f)", i,
                luaL_typename(_lua, i), lua_tonumber(_lua, i));
      break;
    default:
      g_message("%i) %s", i,
                luaL_typename(_lua, i));
      break;
    }
  }
}


grt::ValueRef LuaContext::pop_grt_udata(int pos)
{
  internal::Value **value;
  ValueRef ret;

  value= (internal::Value**)mlua_checkudata(_lua, pos, "MYX_GRT_VALUE");
  if (value)
  {
    ret= ValueRef(*value);
//    (*value)->release();
    mlua_remove(_lua, pos);
    return ret;
  }
  value= (internal::Value**)mlua_checkudata(_lua, pos, "MYX_GRT_LIST");
  if (value)
  {
    ret= ValueRef(*value);
//    (*value)->release();
    mlua_remove(_lua, pos);
    return ret;
  }
  value= (internal::Value**)mlua_checkudata(_lua, pos, "MYX_GRT_DICT");
  if (value)
  {
    ret= ValueRef(*value);
//    (*value)->release();
    mlua_remove(_lua, pos);
    return ret;
  }
  value= (internal::Value**)mlua_checkudata(_lua, pos, "MYX_GRT_OBJECT");
  if (value)
  {
    ret= ValueRef(*value);
//    (*value)->release();
    mlua_remove(_lua, pos);
    return ret;
  }
  return ValueRef();
}




//TODO XXX
//
//check if values are released ok
//check * parsing
void LuaContext::pop_args(const char *format, ...)
{
  va_list args;
  int i, argn;
  int total_argn= 0, opt_argn= 0;
  int top= lua_gettop(_lua);
  int argc= top;
  bool optional= false;
  grt::BaseListRef *group_list= 0;// args grouped inside <> will be put in a list

  va_start(args, format);

  for (i= 0; format[i]!=0; i++)
  {
    if (format[i] == '(' || format[i] == ')')
      continue;
    if (format[i] == '|')
      optional= true;
    else
    {
      total_argn++;
      if (optional)
        opt_argn++;
    }
  }

  if ((lua_gettop(_lua) < (total_argn - opt_argn)) || lua_gettop(_lua) > total_argn)
  {
    char msg[200];
    
    if (opt_argn == 0)
      g_snprintf(msg, sizeof(msg), "Invalid number of arguments to function, expected %i, got %i\n",
              total_argn, lua_gettop(_lua));
    else
      g_snprintf(msg, sizeof(msg), "Invalid number of arguments to function, expected %i to %i, got %i\n",
              total_argn-opt_argn, total_argn, lua_gettop(_lua));
    
    luaL_error(_lua, msg);
  }

  for (i= 0; i < top; i++) lua_insert(_lua, i);

  // uppercase means mandatory (can't be nil)
  for (i= 0, argn= 0; format[i]!=0 && argn < argc; i++)
  {
    switch (format[i])
    {
    case '(':
      group_list= va_arg(args, BaseListRef*);
      if (!group_list->is_valid())
        throw std::logic_error("group list passed to pop_args is not initialized");
      break;
    case ')':
      group_list= 0;
      break;
    case 'i':
      {
        int value= 0;

        if (lua_isnumber(_lua, -1))
          value= luaL_checkint(_lua, -1);
        else
          luaL_typerror(_lua, argn+1, "int");

        if (group_list)
          group_list->ginsert(IntegerRef(value));
        else
          *va_arg(args, int*)= value;

        lua_pop(_lua, 1);
        ++argn;
        break;
      }
    case 'f':
      {
        int value= 0;

        if (lua_isnumber(_lua, -1))
          value= (int) luaL_checknumber(_lua, -1);
        else
          luaL_typerror(_lua, argn+1, "double");

        if (group_list)
          group_list->ginsert(DoubleRef(value));
        else
          *va_arg(args, double*)= value;

        lua_pop(_lua, 1);
        ++argn;
        break;
      }
    case 's':
    case 'S':
      {
        const char *value= NULL;
        internal::Value **udata;
        ValueRef tmp;
        if (lua_isnil(_lua, -1))
        {
          if (format[i] == 'S')
            luaL_typerror(_lua, argn+1, "string");
        }
        if (lua_isstring(_lua, -1))
          value= luaL_checkstring(_lua, -1);
        else if ((udata= (internal::Value**)mlua_checkudata(_lua, -1, "MYX_GRT_VALUE")))
        {
          tmp= ValueRef(*udata);
          (*udata)->release();
          if (tmp.type() == StringType)
            value= StringRef::cast_from(tmp).c_str();
          else
            luaL_error(_lua, "string argument expected, but got a non-string GRT value");
        }
        else
          luaL_typerror(_lua, argn+1, "string");
        
        if (group_list)
          group_list->ginsert(StringRef(value));
        else
          *va_arg(args, const char**)= value;

        lua_pop(_lua, 1);
        ++argn;
        break;
      }
    case 'l':
    case 'L':
      {
        BaseListRef value;
        
        if (lua_isnil(_lua, -1))
        {
          if (format[i] == 'L')
            luaL_typerror(_lua, argn+1, "list");
          lua_pop(_lua, 1);
        }
        else
        {
          // atempt to pop as a lua value
          ValueRef tmp= pop_value();

          if (!tmp.is_valid() || tmp.type() != ListType)
            luaL_typerror(_lua, argn+1, "list");
          
          value= BaseListRef::cast_from(tmp);
        }

        if (group_list)
          group_list->ginsert(value);
        else
          *va_arg(args, BaseListRef*)= value;
        
        ++argn;
      }
      break;
    case 'd':
    case 'D':
      {
        DictRef value;
        
        if (lua_isnil(_lua, -1))
        {
          if (format[i] == 'D')
            luaL_typerror(_lua, argn+1, "dict");
          lua_pop(_lua, 1);
        }
        else
        {
          // atempt to pop as a lua value
          ValueRef tmp= pop_value();

          if (!tmp.is_valid() || tmp.type() != DictType)
            luaL_typerror(_lua, argn+1, "dict");
          
          value= DictRef::cast_from(tmp);
        }

        if (group_list)
          group_list->ginsert(value);
        else
          *va_arg(args, DictRef*)= value;

        ++argn;
      }
      break;
    case 'o':
    case 'O':
      {
        ObjectRef value;
        internal::Value **udata;

        if (lua_isnil(_lua, -1))
        {
          if (format[i] == 'O')
            luaL_typerror(_lua, argn+1, "object");
          lua_pop(_lua, 1);
        }
        else if ((udata= (internal::Value**)mlua_checkudata(_lua, -1, "MYX_GRT_OBJECT")))
        {
          value= ObjectRef(dynamic_cast<internal::Object*>(*udata));
          lua_pop(_lua, 1);
        }
        else
          luaL_typerror(_lua, argn+1, "object");

        if (group_list)
          group_list->ginsert(value);
        else
          *va_arg(args, ObjectRef*)= value;

        ++argn;
      }
      break;
    case 'g':
    case 'G':
      {
        ValueRef value;

        if (lua_isnil(_lua, -1))
        {
          if (format[i] == 'G')
            luaL_typerror(_lua, argn+1, "grt value");
          lua_pop(_lua, 1);
        }
        else if ((value= pop_value()).is_valid())
        {
        }
        else
          luaL_typerror(_lua, argn+1, "Grt value");
        
        if (group_list)
          group_list->ginsert(value);
        else
          *va_arg(args, ValueRef*)= value;
        
        ++argn;
      }
      break;
    case '*':
      {
        if (group_list)
          group_list->ginsert(pop_value());
        else
          *va_arg(args, ValueRef*)= pop_value();
        ++argn;
      }
      break;
    }
  }

  va_end(args);
  g_assert(lua_gettop(_lua)==0);
}



//================================================================================
// GRT Stuff


static int call_grt_module_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  lua_Debug dbg;
  const char *name;
  BaseListRef arglist;
  int l_top= lua_gettop(l);
  int res= 0;
  
  // argument 0 is self (i.e.: the module table)
  // argument > 0 are the arguments to be passed to the function

  // pop argument from stack, if its there
  if (l_top == 0)
  {
    luaL_error(l, "function call error, module functions must be called as module:function()");
    return 0;
  }
  else if (l_top == 1)
    arglist= BaseListRef(ctx->get_grt());
  else
  {
    arglist= BaseListRef(ctx->get_grt());

    while (lua_gettop(l) > 1)
    {
      ValueRef value= ctx->pop_value();
      arglist.ginsert(value, 0);
    }
  }

  // get info about the function that's being called
  lua_getstack(l, 0, &dbg);
  lua_getinfo(l, "n", &dbg);

  // get name of the module from the 1st table in stack (which is "self")
  lua_pushstring(l, "_name_");
  lua_gettable(l, -2);

  name= lua_tostring(l, -1);
  lua_pop(l, 1);

  if (name)
    res= ctx->call_grt_function(name, dbg.name, arglist);
  else
    luaL_error(l, "The module name is not set. Please check if you use modulename:function() name instead of modulename.function().");

  return res;
}


int LuaContext::add_module_to_table(Module *module, int tbl)
{
  lua_pushstring(_lua, "_name_");
  lua_pushstring(_lua, module->name().c_str());
  lua_settable(_lua, tbl);

  lua_pushstring(_lua, "_extends_");
  if (!module->extends().empty())
    lua_pushstring(_lua, module->extends().c_str());
  else
    lua_pushnil(_lua);
  lua_settable(_lua, tbl);
  
  lua_pushstring(_lua, "version");
  lua_pushstring(_lua, module->version().c_str());
  lua_settable(_lua, tbl);

  lua_pushstring(_lua, "author");
  lua_pushstring(_lua, module->author().c_str());
  lua_settable(_lua, tbl);

  // add all functions to the table, so that they can be called as table.func()
  for (std::vector<Module::Function>::const_iterator iter= module->get_functions().begin();
       iter != module->get_functions().end(); ++iter)
  {
    lua_pushstring(_lua, iter->name.c_str());
    lua_pushcfunction(_lua, call_grt_module_function);
    lua_settable(_lua, tbl);
  }

  return 1;
}


int LuaContext::refresh()
{
  // Will create tables representing the available modules containing
  // the functions it implements. You can then call the functions with
  // module->function()

  const std::vector<Module*> &modules(_grt->get_modules());

  for (std::vector<Module*>::const_iterator iter= modules.begin();
       iter != modules.end(); ++iter)
  {
    lua_newtable(_lua);
    add_module_to_table(*iter, lua_gettop(_lua));
    lua_setglobal(_lua, (*iter)->name().c_str());
  }

  return 0;
}


int LuaContext::call_grt_function(const std::string &module_name, const std::string &function,
                                  const BaseListRef &args)
{
  Module *module;

  module= _grt->get_module(module_name);
  if (!module)
    return luaL_error(_lua, "the GRT module %s does not exist", module_name.c_str());

  ValueRef retval;
  
  try
  {
    retval= module->call_function(function, args);
  }
  catch (std::exception &exc)
  {
    return luaL_error(_lua, "error calling %s.%s: %s", module_name.c_str(), function.c_str(),
                      exc.what());
  }

  if (retval.is_valid())
    push_and_wrap_if_not_simple(retval);

  return 1;
}

//--------------------------------------------------------------------------------------------------

// Helper struct to allow loading lua files via an own loader (to avoid non-ANSI file name issues).
typedef struct
{
  std::string name;
  FILE* file;
  const char* data;
  size_t size;
} LuaFile;

//--------------------------------------------------------------------------------------------------

/**
 * Used to actually read a file's content. This approach is necessary because Lua itself cannot
 * handle utf-8 encoded file paths (at least not on Windows).
 * The file is read in one step.
 */
static const char* read_lua_file(lua_State* state, void* data, size_t* size)
{
  LuaFile* file_data= static_cast<LuaFile*>(data);

  if (file_data->file == 0)
  {
    // File not yet read. Do it now.
    file_data->file= base_fopen(file_data->name.c_str(), "rb");
    if (file_data->file == NULL)
      return NULL;

    fseek(file_data->file, 0L, SEEK_END);
    file_data->size= ftell(file_data->file);
    if (file_data->size == 0)
    {
      fclose(file_data->file);
      *size= 0;
      return NULL;
    }
    file_data->data= new char[file_data->size];
    fseek(file_data->file, 0L, SEEK_SET);
    fread((void*) file_data->data, file_data->size, 1, file_data->file);
    *size= file_data->size;
    return file_data->data;
  }
  else
  {
    // Since we load the whole file in one run, if we come here, we are done and can close the file.
    fclose(file_data->file);
    file_data->file= NULL;
    delete []file_data->data;
    file_data->data= 0;
    
    *size= 0;
    return NULL;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads a Lua file using our own mechanism, to avoid non-ANSI file name issues.
 */
int LuaContext::load_file(const std::string& path)
{
  LuaFile file= {path, NULL, NULL, 0};
  std::string message= "Loading Lua file: " + path;
  return lua_load(_lua, read_lua_file, &file, message.c_str());
}

//--------------------------------------------------------------------------------------------------

// GRT Functions Exported to Lua


static int l_list_modules(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);

  ctx->pop_args("");

  const std::vector<Module*> &modules(ctx->get_grt()->get_modules());
  for (std::vector<Module*>::const_iterator iter= modules.begin(); iter != modules.end(); ++iter)
  {
    ctx->get_grt()->send_output((*iter)->name()+"\n");
  }
  ctx->get_grt()->send_output(strfmt("%i modules\n", (int) modules.size()));

  return 0;
}


static int l_show_module(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *name;
  Module *module;
  
  ctx->pop_args("s", &name);
  
  module= ctx->get_grt()->get_module(name);
  if (!module)
  {
    ctx->get_grt()->send_output(strfmt("Module '%s' has not been initialized.\n", name));
    return 0;
  }

  if (!module->extends().empty())
    ctx->get_grt()->send_output(strfmt("Module '%s' (version %s, extends '%s')\n", 
                                   name, module->version().c_str(), module->extends().c_str()));
  else
    ctx->get_grt()->send_output(strfmt("Module '%s' (version %s)\n", 
                                   name, module->version().c_str()));

  const std::vector<Module::Function> &functions(module->get_functions());
  
  for (std::vector<Module::Function>::const_iterator iter= functions.begin();
       iter != functions.end(); ++iter)
  {
    std::string ret= fmt_type_spec(iter->ret_type);
    std::string args= fmt_arg_spec_list(iter->arg_types);

    ctx->get_grt()->send_output(strfmt(" %s %s(%s)\n",
                                   ret.c_str(), iter->name.c_str(), args.c_str()));
  }

  return 0;
}


static int l_get_modules(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  int tbl_stack_pos, i;
  
  ctx->pop_args("");

  lua_newtable(l);
  tbl_stack_pos= lua_gettop(l);

  i= 0;
  const std::vector<Module*> &modules(ctx->get_grt()->get_modules());
  for (std::vector<Module*>::const_iterator iter= modules.begin(); iter != modules.end(); ++iter)
  {
    lua_pushinteger(l, ++i);
    lua_pushstring(l, (*iter)->name().c_str());

    lua_settable(l, tbl_stack_pos);
  }

  return 1;
}


static int l_get_module_functions(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *name;
  Module *module;
  int tbl_stack_pos, i;
  
  ctx->pop_args("s", &name);
  
  module= ctx->get_grt()->get_module(name);
  if (!module)
    return 0;

  lua_newtable(l);
  tbl_stack_pos= lua_gettop(l);

  i= 0;

  const std::vector<Module::Function> &functions(module->get_functions());
  
  for (std::vector<Module::Function>::const_iterator iter= functions.begin();
       iter != functions.end(); ++iter)
  {
    lua_pushinteger(l, ++i);
    lua_pushstring(l, iter->name.c_str());

    lua_settable(l, tbl_stack_pos);
  }

  return 1;
}


static int l_call_function(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  BaseListRef arglist;
  const char *module_name, *function_name;
  int res;

  ctx->pop_args("SS|l", &module_name, &function_name, &arglist);

  if (!arglist.is_valid())
    arglist= BaseListRef();

  res= ctx->call_grt_function(module_name, function_name, arglist);

  return res;
}


static int l_refresh(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);

  return ctx->refresh();
}


static int l_save_value(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *fn;
  const char *type= NULL, *version= NULL;
  ValueRef value;

  ctx->pop_args("GS|ss", &value, &fn, &type, &version);

  try
  {
    ctx->get_grt()->serialize(value, fn, type?type:"", version?version:"");
  }
  catch (std::exception &exc)
  {
    return luaL_error(l, "Could not save data to file %s: %s", fn, exc.what());
  }

  return 0;
}


static int l_load_value(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *fn;
  ValueRef value;

  ctx->pop_args("S", &fn);

  try
  {
    value= ctx->get_grt()->unserialize(fn);
  }
  catch (std::exception &exc)
  {
    return luaL_error(l, "Could not load data from file %s: %s", fn, exc.what());
  }
  ctx->push_wrap_value(value);

  return 1;
}

static int l_grt_value_type(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;

  ctx->pop_args("G", &value);

  if (value.is_valid())
    lua_pushstring(l, type_to_str(value.type()).c_str());
  else
    lua_pushnil(l);
  
  return 1;
}


static int l_grt_value_getn(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;
  
  ctx->pop_args("G", &value);
  
  if (value.type() != ListType && value.type() != DictType)
  {
    luaL_error(l, "Invalid parameter: expected list or dict value");
  }
  
  if (value.type() == ListType)
    lua_pushnumber(l, (lua_Number)BaseListRef::cast_from(value).count());
  else
    lua_pushnumber(l, (lua_Number)DictRef::cast_from(value).count());

  return 1;  
}



static int l_grt_value_from_xml(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *xml;

  ctx->pop_args("S", &xml);

  try
  {
    ValueRef value= ctx->get_grt()->unserialize_xml_data(xml?xml:"");

    ctx->push_wrap_value(value);
  }
  catch (std::exception)
  {
    return luaL_error(l, "Could not unserialize data from string");
  }
  return 1;
}


static int l_grt_value_to_xml(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;
  std::string xml;
  const char *type= NULL, *version= NULL;

  ctx->pop_args("G|ss", &value, &type, &version);

  xml= ctx->get_grt()->serialize_xml_data(value, type?type:"", version?version:"");

  lua_pushstring(l, xml.c_str());

  return 1;
}


static int l_cd(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *path;
  
  ctx->pop_args("S", &path);

  if (!*path)
    return 0;

  if (!ctx->set_cwd(path))
    luaL_error(l, "Invalid path");

  return 0;
}


static bool print_member_name(const grt::MetaClass::Member *member, GRT *grt)
{
  grt->send_output(member->name+"\n");
  return true;
}


static int l_ls(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *path = NULL;
  std::string fpath;
  ValueRef value;

  ctx->pop_args("|s", &path);
  if (!path)
    path = "";

  fpath= Shell::get_abspath(ctx->get_cwd(), path);
  value= ctx->get_grt()->get(fpath);
  if (!value.is_valid())
    luaL_error(l, "Invalid path");

  if (value.type() == DictType)
  {
    DictRef dict(DictRef::cast_from(value));
    
    for (DictRef::const_iterator item= dict.begin(); item != dict.end(); ++item)
    {
      //if (!is_simple_type(item->second.type()))
        ctx->get_grt()->send_output(item->first+"\n");
    }
  }
  else if (value.type() == ObjectType)
  {
    ObjectRef object(ObjectRef::cast_from(value));
    MetaClass *meta= object.get_metaclass();

    meta->foreach_member(boost::bind(&print_member_name, _1, ctx->get_grt()));
  }
  else if (value.type() == ListType)
  {
    BaseListRef list(BaseListRef::cast_from(value));
    int unnamed= 0;
    
    for (size_t c= list.count(), i= 0; i < c; i++)
    {
      ValueRef v(list[i]);
      
      if (v.type() == ObjectType)
      {
        ObjectRef o(ObjectRef::cast_from(v));

        if (o.has_member("name"))
          ctx->get_grt()->send_output(o.get_string_member("name")+"\n");
        else
          unnamed++;
      }
    }
    if (unnamed > 0)
      ctx->get_grt()->send_output(strfmt("Plus %i unnamed objects in the list.\n", unnamed));
  }
  else
    luaL_error(l, "Not in a listable object");

  return 0;
}


static int l_pwd(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);

  ctx->pop_args("");
  
  lua_pushstring(l, ctx->get_cwd().c_str());

  return 1;
}


static int l_get_child(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;
  DictRef dict;
  const char *path;

  ctx->pop_args("S|d", &path, &dict);

  if (dict.is_valid())
  {
    if (*path != '/')
      luaL_error(l, "bad path for child object in dict. Must be an absolute path");
    value= get_value_by_path(dict, path);
  }
  if (value.is_valid())
  {
    ctx->push_wrap_value(value);
  }
  else
  {
    lua_pushnil(l);
    //luaL_error(l, "object '%s' not found", path);
  }

  return 1;
}

static int l_get_global(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;
  DictRef dict;
  const char *path;

  // if the argument is already a MYX_GRT_VALUE, leave it on the stack and exit
  if (luaL_checkgrtudata(l, -1))
    return 0;

  ctx->pop_args("S|d", &path, &dict);

  if (dict.is_valid())
  {
    if (*path != '/')
      return luaL_error(l, "bad path for getobj in dict. Must be an absolute path");

    value= get_value_by_path(dict, path);
  }
  else
  {
    try
    {
      value= ctx->get_grt()->get(Shell::get_abspath(ctx->get_cwd(), path));
    }
    catch (grt::bad_item)
    {
      value= grt::ValueRef();
    }
  }
  if (!value.is_valid())
    luaL_error(l, "object '%s' not found", path);
  
  ctx->push_wrap_value(value);

  return 1;
}


static int l_set_global(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;
  ValueRef indict;
  const char *path;

 ctx->pop_args("SG|d", &path, &value, &indict);

  if (indict.is_valid())
  {
    if (indict.type() != DictType && indict.type() != ObjectType)
    {
      luaL_error(l, "invalid argument, expected a dict but got something else");
    }
    if (*path != '/')
    {
      luaL_error(l, "bad path for setobj in dict. Must be an absolute path");
    }
    if (!set_value_by_path(indict, path, value))
    {
      luaL_error(l, "invalid path '%s'", path);
    }
  }
  else
  {
    if (path && strcmp(path, "/") == 0)
    {
      ctx->get_grt()->set_root(value);
    }
    else if (path)
    {
      std::string fpath;

      fpath= Shell::get_abspath(ctx->get_cwd(), path);
      try
      {
        ctx->get_grt()->set(fpath, value);
      }
      catch (std::exception &exc)
      {
        luaL_error(l, exc.what());
      }
    }
  }

  return 1;
}


static int l_grt_value_to_lua(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;

  value= ctx->pop_value();
  ctx->push_convert_value(value);

  return 1;
}


static int l_grt_value_new_obj(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *gstruct_name;
  MetaClass *gstruct;
  ObjectRef value;
  DictRef args;

  ctx->pop_args("S|D", &gstruct_name, &args);

  gstruct= ctx->get_grt()->get_metaclass(gstruct_name);
  if (!gstruct)
    return luaL_error(l, "invalid struct name '%s' for new object", gstruct_name);

  value= gstruct->allocate();
  if (args.is_valid())
  {
    for (DictRef::const_iterator iter= args.begin(); iter != args.end(); ++iter)
    {
      try
      {
        // if arg is a dict or a list, we replace the default contents
        if (iter->second.type() == DictType)
        {
          grt::replace_contents(grt::DictRef::cast_from(value.get_member(iter->first)),
                                grt::DictRef::cast_from(iter->second));
        }
        else if (iter->second.type() == ListType)
        {
          grt::replace_contents(grt::BaseListRef::cast_from(value.get_member(iter->first)),
                                grt::BaseListRef::cast_from(iter->second));
        }
        else
          value.set_member(iter->first, iter->second);
      }
      catch (grt::type_error)
      {
        return luaL_error(l, "invalid type for initializer for '%s'", iter->first.c_str());
      }
      catch (std::exception &exc)
      {
        return luaL_error(l, "error setting initial value for new object: %s", exc.what());
      }
    }
  }

  //push the Grt value as Lua userdata
  ctx->push_wrap_value(value);

  return 1;
}


static int l_grt_value_new_list(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *content_type_name= NULL;
  const char *content_struct_name= NULL;
  ValueRef value;
  Type content_type;

  ctx->pop_args("|ss", &content_type_name, &content_struct_name);

  if (content_type_name)
  {
    content_type= str_to_type(content_type_name);

    if (content_type == UnknownType && strcmp(content_type_name, "")!=0 && strcmp(content_type_name, "any")!=0)
      return luaL_error(l, "invalid content_type. Use int, real, string, list, dict or object");

    if (content_struct_name && *content_struct_name && content_type != ObjectType)
      return luaL_error(l, "struct name is only needed for object values");
  }
  else
    content_type= AnyType;

  value= BaseListRef(ctx->get_grt(), content_type, content_struct_name?content_struct_name:"");

  //push the Grt value as Lua userdata
  ctx->push_wrap_value(value);

  return 1;
}

static int l_grt_value_new_dict(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *content_type_name= NULL;
  const char *content_struct_name= NULL;
  ValueRef value;
  Type content_type;

  ctx->pop_args("|ss", &content_type_name, &content_struct_name);

  if (content_type_name)
  {
    content_type= str_to_type(content_type_name);

    if (content_type == UnknownType && strcmp(content_type_name, "")!=0 && strcmp(content_type_name, "any")!=0)
      return luaL_error(l, "invalid content_type. Use int, real, string, list, dict or object");

    if (content_struct_name && *content_struct_name && content_type != ObjectType)
      return luaL_error(l, "struct name is only needed for object values");
  }
  else
    content_type= AnyType;

  value= DictRef(ctx->get_grt(), content_type, content_struct_name?content_struct_name:"");

  //push the Grt value as Lua userdata
  ctx->push_wrap_value(value);
  
  return 1;
}

static int l_grt_value_get_type(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;

  ctx->pop_args("G", &value);

  if (value.is_valid())
  {
    if (value.type() == ListType)
      lua_pushstring(l, type_to_str(BaseListRef::cast_from(value).content_type()).c_str());
    else
      lua_pushstring(l, type_to_str(DictRef::cast_from(value).content_type()).c_str());
  }
  else
    lua_pushnil(l);
  
  return 1;
}


static int l_print(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  int i, n= lua_gettop(l);

  // invert the stack
  for (i= 0; i < n; i++) lua_insert(l, i);
  
  // print the stack
  while ((n= lua_gettop(l)) > 0)
  {
    if (luaL_checkgrtudata(l, -1))
    {
      ValueRef value= ctx->pop_value();
      ctx->print_value(value);
    }
    else
    {
      const char *s;
      lua_getglobal(l, "tostring");
      lua_insert(l, -2);  /* move function to be called before the value */

      lua_call(l, 1, 1);
      s = lua_tostring(l, -1);  /* get result */
      if (s == NULL)
        return luaL_error(l, "`tostring' must return a string to `print'");
      //if (i>1) base_grt_printf(grt, " ");
      ctx->get_grt()->send_output(s);
      lua_pop(l, 2);  /* pop result and function */
    }
  }

  return 0;
}

static void lua_tracer(lua_State *l, lua_Debug *ar)
{
  LuaContext *ctx= LuaContext::get(l);
  lua_Debug info;

  lua_getstack(l, 0, &info);
  lua_getinfo(l, "S", &info);

  ctx->get_grt()->send_output(strfmt("Lua: %s:%i", info.source, ar->currentline));
}

static void lua_tracer2(lua_State *l, lua_Debug *ar)
{
  lua_Debug info;
  
  lua_getstack(l, 0, &info);
  lua_getinfo(l, "S", &info);
  
  g_message("Lua: %s:%i\n", info.source, ar->currentline);
}


static int l_trace_enable(lua_State *l)
{
  if (luaL_checkint(l, -1) == 1)
  {
    lua_sethook(l, lua_tracer, LUA_MASKLINE, 0);
  }
  else if (luaL_checkint(l, -1) == 2)
  {
    lua_sethook(l, lua_tracer2, LUA_MASKLINE, 0);
  }  
  else
    lua_sethook(l, NULL, LUA_MASKLINE, 0);
  
  lua_pop(l, 1);
  
  return 0;
}


static int l_backtrace(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  lua_Debug info;
  int i;
  
  ctx->get_grt()->send_output("Current Lua Stacktrace:");
  for (i= 1; ; i++)
  {
    if (lua_getstack(l, i, &info) == 0)
      break;
    lua_getinfo(l, "Snl", &info);
    
    ctx->get_grt()->send_output(strfmt("#%i  %s %s at %s %s:%i", i,
                                   info.namewhat, info.name,
                                   info.what, info.source, info.currentline));
  }
  return 0;
}


static int l_grt_value_refcount(lua_State *l)
{ 
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;

  ctx->pop_args("G", &value);
  
  if (value.is_valid())
    lua_pushinteger(l, value.valueptr()->refcount());
  else
    lua_pushnil(l);

  return 1;
}

static int l_list_structs(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  
  for (std::list<MetaClass*>::const_iterator iter= ctx->get_grt()->get_metaclasses().begin();
       iter != ctx->get_grt()->get_metaclasses().end(); ++iter)
  {
    ctx->get_grt()->send_output((*iter)->name().c_str());
  }
  return 0;
}


static bool print_fmt_member(const MetaClass::Member *member, GRT *grt)
{
  grt->send_output(strfmt(" %s: %s\n", member->name.c_str(), fmt_type_spec(member->type).c_str()));
  return true;
}


static bool print_fmt_method(const MetaClass::Method *method, GRT *grt)
{
  std::string args;
  
  for (ArgSpecList::const_iterator arg= method->arg_types.begin();
       arg != method->arg_types.end(); ++arg)
  {
    if (!args.empty())
      args.append(", ");
    args.append(arg->name);
    args.append(" ");
    args.append(fmt_type_spec(arg->type));
  }
  
  grt->send_output(strfmt(" %s %s(%s)\n",
                          fmt_type_spec(method->ret_type).c_str(),
                          method->name.c_str(), args.c_str()));
  return true;
}


static int l_show_struct(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  MetaClass *gstruct;
  char *name;

  ctx->pop_args("s", &name);

  gstruct= ctx->get_grt()->get_metaclass(name);
  if (!gstruct)
    return luaL_error(l, "Invalid name %s", name);

  if (gstruct->parent())
    ctx->get_grt()->send_output(strfmt("Struct '%s' (parent %s)\n",
                                   gstruct->name().c_str(), gstruct->parent()->name().c_str()));
  else
    ctx->get_grt()->send_output(strfmt("Struct '%s'\n",
                                   gstruct->name().c_str()));

  gstruct->foreach_member(boost::bind(print_fmt_member, _1, ctx->get_grt()));

  gstruct->foreach_method(boost::bind(&print_fmt_method, _1, ctx->get_grt()));

  return 0;
}


static int l_get_struct(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ObjectRef value;

  ctx->pop_args("O", &value);

  if (!value.is_valid())
    lua_pushnil(l);
  else
    lua_pushstring(l, value.get_metaclass()->name().c_str());

  return 1;
}


static int l_get_contentstruct(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value;

  ctx->pop_args("G", &value);

  if (value.type() == DictType)
    lua_pushstring(l, DictRef::cast_from(value).content_class_name().c_str());
  else if (value.type() == ListType)
    lua_pushstring(l, BaseListRef::cast_from(value).content_class_name().c_str());
  else
    return luaL_error(l, "argument must be a list or dict");

  return 1;
}


static bool push_members(const MetaClass::Member *mem, lua_State *l, size_t *i, size_t tbl_stack_pos)
{
  (*i)++;

  lua_pushinteger(l, *i);
  lua_pushstring(l, mem->name.c_str());

  lua_settable(l, (int)tbl_stack_pos);
  
  return true;
}


static int l_get_struct_members(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *sname;
  MetaClass *gstruct;
  size_t i, tbl_stack_pos;

  ctx->pop_args("S", &sname);
  if (!(gstruct= ctx->get_grt()->get_metaclass(sname)))
    luaL_error(l, "unknown struct name '%s'", sname);

  lua_newtable(l);
  tbl_stack_pos= lua_gettop(l);

  i= 0;
  
  gstruct->foreach_member(boost::bind(&push_members, _1, l, &i, tbl_stack_pos));

  return 1;
}
  
static int l_get_struct_member_type(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *sname, *mname;
  MetaClass *gstruct;

  ctx->pop_args("SS", &sname, &mname);

  if (!(gstruct= ctx->get_grt()->get_metaclass(sname)))
    luaL_error(l, "unknown struct name '%s'", sname);

  const MetaClass::Member* mem= gstruct->get_member_info(mname);
  if (!mem)
    luaL_error(l, "unknown member name '%s.%s'", sname, mname);

  lua_pushstring(l, type_to_str(mem->type.base.type).c_str());

  return 1;
}

static int l_get_struct_member_content_type(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *sname, *mname;
  MetaClass *gstruct;

  ctx->pop_args("SS", &sname, &mname);

  if (!(gstruct= ctx->get_grt()->get_metaclass(sname)))
    luaL_error(l, "unknown struct name '%s'", sname);

  const MetaClass::Member* mem= gstruct->get_member_info(mname);
  if (!mem)
    luaL_error(l, "unknown member name '%s.%s'", sname, mname);

  lua_pushstring(l, type_to_str(mem->type.content.type).c_str());

  return 1;
}

static int l_get_struct_member_content_struct(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *sname, *mname;
  MetaClass *gstruct;

  ctx->pop_args("SS", &sname, &mname);

  if (!(gstruct= ctx->get_grt()->get_metaclass(sname)))
    luaL_error(l, "unknown struct name '%s'", sname);

  const MetaClass::Member *mem= gstruct->get_member_info(mname);
  if (!mem)
    luaL_error(l, "unknown member name '%s.%s'", sname, mname);

  lua_pushstring(l, mem->type.content.object_class.c_str());

  return 1;
}

static int l_struct_is_or_inherits_from(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *sname, *parent_name;
  MetaClass *a, *b;

  ctx->pop_args("SS", &sname, &parent_name);

  a= ctx->get_grt()->get_metaclass(sname);
  b= ctx->get_grt()->get_metaclass(parent_name);
  
  if (!a)
    luaL_error(l, "%s is not a struct", sname);
  if (!b)
    luaL_error(l, "%s is not a struct", parent_name);

  if (a->is_a(b))
    lua_pushboolean(l, 1);
  else
    lua_pushboolean(l, 0);
  
  return 1;
}

static int l_get_struct_attribute(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *sname, *attr;
  MetaClass *gstruct;
  int inherited_caption= 1; // TODO: this var does nothing, can it be removed?
  
  ctx->pop_args("SS", &sname, &attr);

  gstruct= ctx->get_grt()->get_metaclass(sname);
  if (!gstruct)
    luaL_error(l, "unknown struct name '%s'", sname);

  std::string v;
  
  v= gstruct->get_attribute(attr);
  if (inherited_caption && v.empty())
  {
    gstruct= gstruct->parent();
    while (v.empty() && gstruct)
    {
      v= gstruct->get_attribute(attr);

      gstruct= gstruct->parent();
    }
  }
  
  lua_pushstring(l, v.c_str());

  return 1;
}

static int l_struct_exists(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *s;

  ctx->pop_args("S", &s);
  
  if (ctx->get_grt()->get_metaclass(s))
    lua_pushboolean(l, 1);
  else
    lua_pushboolean(l, 0);

  return 1;
}

static int l_run(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *path;

  ctx->pop_args("S", &path);
  
  try
  {
    ctx->run_file(path, true);
  }
  catch (std::exception &exc)
  {
    return luaL_error(l, "error executing script: %s", exc.what());
  }

  return 0;
}

static int l_sleep(lua_State *l)
{
  int ms= luaL_checkint(l, -1);
  lua_pop(l, 1);

  g_usleep(ms*1000);

  return 0;
}

static int l_exit(lua_State *l)
{
  int st= luaL_checkint(l, -1);

  //QQQ
  exit(st);

  return 0;
}


static int l_regex_val(lua_State *l)
{
  const char *txt;
  const char *regex;
  int substr_nr;
  char *val;
  
  if (lua_isnumber(l, -1))
  {
    substr_nr= luaL_checkint(l, -1);
    lua_pop(l, 1);
  }
  else
    substr_nr= 1;

  if (!lua_isstring(l, -1))
    luaL_error(l, "missing regex");
  regex= mlua_popstring(l);

  if (!lua_isstring(l, -1))
    luaL_error(l, "missing text");
  txt= mlua_popstring(l);

  val= get_value_from_text_ex(txt, (int)strlen(txt), regex, substr_nr);

  if (val)
  {
    lua_pushstring(l, val);
    g_free(val);
  }
  else
    lua_pushstring(l, "");

  return 1;
}

static int l_str_replace(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  const char *text;
  const char *from;
  const char *to;
  char *result;
 
  ctx->pop_args("SSS", &to, &from, &text);
  
  result= str_g_subst(text, from, to);

  lua_pushstring(l, result);
  g_free(result);

  return 1;
}



int l_log_error(lua_State *lua)
{
  LuaContext *ctx= LuaContext::get(lua);
  const char *message;
  const char *detail= NULL;
  
  ctx->pop_args("s|s", &message, &detail);
  ctx->get_grt()->send_error(message, detail?detail:"");

  return 0;
}


int l_log_warning(lua_State *lua)
{
  LuaContext *ctx= LuaContext::get(lua);
  const char *message;
  const char *detail= NULL;

  ctx->pop_args("s|s", &message, &detail);
  ctx->get_grt()->send_warning(message, detail?detail:"");

  return 0;
}


int l_log_message(lua_State *lua)
{
  LuaContext *ctx= LuaContext::get(lua);
  const char *message;
  const char *detail= NULL;
  
  ctx->pop_args("s|s", &message, &detail);
  ctx->get_grt()->send_info(message, detail?detail:"");

  return 0;
}


static int l_grt_value_insert(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  BaseListRef value;
  ValueRef rvalue;
  int index= -1;

  ctx->pop_args("L*|i", &value, &rvalue, &index);
  
  if (!rvalue.is_valid())
  {
    luaL_error(l, "Invalid object to be inserted to list");
  }
  if (index == 0)
  {
    luaL_error(l, "List index starts at 1");
  }
  if (index > 0)
    index--;
  if (index < 0 || (size_t) index >= value.count())
    value.ginsert(rvalue);
  else
    value.ginsert(rvalue, index);

  return 0;
}

static int l_grt_value_remove(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  BaseListRef value;
  int index;
  
  ctx->pop_args("Li", &value, &index);

  if (index == 0)
    luaL_error(l, "List index starts at 1");
  index--;
  if (index < 0 || (size_t) index >= value.count())
    luaL_error(l, "Invalid list index");
  value.remove(index);

  return 0;
}


static int l_grt_value_remove_object(lua_State *l)
{  
  LuaContext *ctx= LuaContext::get(l);
  BaseListRef value;
  ValueRef object;
  
  ctx->pop_args("LO", &value, &object);

  value.gremove_value(object);

  return 0;
}


static int l_grt_get_keys(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  DictRef dict;

  ctx->pop_args("D", &dict);
  
  lua_newtable(l);

  int i= 0;
  for (DictRef::const_iterator item= dict.begin(); item != dict.end(); ++item)
  {
    lua_pushstring(l, item->first.c_str());
    lua_rawseti(l, -2, ++i);
  }

  return 1;
}


static int l_grt_get_list_item_by_obj_name(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  BaseListRef list;
  ValueRef value;
  const char *name;

  ctx->pop_args("Ls", &list, &name);

  ObjectRef object= find_named_object_in_list(ObjectListRef::cast_from(list), name);

  if (object.is_valid())
  {
    ctx->push_wrap_value(object);
  }
  else
    lua_pushnil(l);

  return 1;
}

static int l_grt_value_duplicate(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);
  ValueRef value, value_dup;

  ctx->pop_args("G", &value);

  value_dup= copy_value(value, true);

  ctx->push_wrap_value(value_dup);

  return 1;
}



#if 0
static int l_grt_pairs_aux(lua_State *L) 
{
  int i = luaL_checkint(L, 2); // cur index

  i++; // go to next
  lua_pushinteger(L, i); // save new index

  if (lua_istable(L, 1))
  {
    lua_settop(L, 2);
    if (lua_next(L, 1))
      return 2;
    else 
    {
      lua_pushnil(L);
      return 1;
    }
  }
  else
  {
    LuaContext *ctx= LuaContext::get(L);
    internal::Value *value= luaL_checkgrtudata(L, 1);

    if (!value)
      luaL_error(L, "bad argument #1 to ipairs (expected table or grt dict).");
    else if (value->type() == DictType)
    {
      const char *key= lua_tostring(L); // pop key

      ctx->push_and_wrap_if_not_simple(value->get(i));
    }
    else
      luaL_error(L, "bad argument #1 to ipairs (expected table or grt dict).");
  }
}


static int l_grt_pairs(lua_State *l)
{
  LuaContext *ctx= LuaContext::get(l);

  // check if value in stack is a lua table or a grt dict
  if (lua_istable(l, 1))
  {
  }
  else
  {
    if (!mlua_checkudata(l, "MYX_GRT_DICT"))
      luaL_error(l, "bad argument #1 to ipairs (expected table or grt dict)");
  }
  lua_pushvalue(l, lua_upvalueindex(1));  /* return generator, */
  lua_pushvalue(l, 1);  /* state, */
  lua_pushnil(l);
  
  return 3;
}
#endif

static int l_grt_ipairs_aux(lua_State *L) 
{
  int i = luaL_checkint(L, 2); // cur index

  i++; // go to next
  lua_pushinteger(L, i); // save new index

  if (lua_istable(L, 1))
  {
    lua_rawgeti(L, 1, i); // get index and value from table
  }
  else
  {
    LuaContext *ctx= LuaContext::get(L);
    internal::Value *value= luaL_checkgrtudata(L, 1);

    if (!value)
      luaL_error(L, "bad argument #1 to ipairs (expected table or grt list).");
    else if (value->get_type() == ListType)
    {
      internal::List *list= (internal::List*)value;
      
      if (i > (int)list->count())
        lua_pushnil(L);
      else
        ctx->push_and_wrap_if_not_simple(list->get(i-1));
    }
    else
      luaL_error(L, "bad argument #1 to ipairs (expected table or grt list, got %s).", type_to_str(value->get_type()).c_str());
  }

  return (lua_isnil(L, -1)) ? 0 : 2;
}


static int l_grt_ipairs(lua_State *l)
{
  // check if value in stack is a lua table or a grt list
  if (lua_istable(l, 1))
  {
  }
  else
  {
    if (!mlua_checkudata(l, 1, "MYX_GRT_LIST"))
      luaL_error(l, "bad argument #1 to ipairs (expected table or grt list)");
  }
  lua_pushvalue(l, lua_upvalueindex(1));  /* return generator, */
  lua_pushvalue(l, 1);  /* state, */
  lua_pushinteger(l, 0);  /* and initial value */
  
  return 3;
}


static void set_closure(lua_State *l, const char *name, lua_CFunction f, lua_CFunction cf)
{
  lua_pushcfunction(l, cf);
  lua_pushcclosure(l, f, 1);
  lua_setfield(l, LUA_GLOBALSINDEX, name);
}


void LuaContext::register_grt_functions()
{
  static luaL_reg grtValue[]= {
    {"toLua",         l_grt_value_to_lua},
    {"setGlobal",     l_set_global},
    {"getGlobal",     l_get_global},
    {"child",         l_get_child},
    {"newObj",        l_grt_value_new_obj},
    {"newList",       l_grt_value_new_list},
    {"newDict",       l_grt_value_new_dict},
    {"getContentType",l_grt_value_get_type},
    {"insert",        l_grt_value_insert},
    {"remove",        l_grt_value_remove},
    {"removeObj",     l_grt_value_remove_object},
    {"getKeys",       l_grt_get_keys},
    {"getListItemByObjName", l_grt_get_list_item_by_obj_name},
    {"load",          l_load_value},
    {"save",          l_save_value},
    {"fromXml",       l_grt_value_from_xml},
    {"toXml",         l_grt_value_to_xml},
    {"typeOf",        l_grt_value_type},
    {"duplicate",     l_grt_value_duplicate},
    {"refcount",      l_grt_value_refcount},
    
    // redundant, but kept for backward compat
    {"getn",          l_grt_value_getn},
    {NULL,            NULL}
  };
  static luaL_reg grtStruct[]= {
    {"list",                    l_list_structs},
    {"show",                    l_show_struct},
//    {"load",                    l_loadstructs},
    {"exists",                  l_struct_exists},
    {"get",                     l_get_struct},
    {"getContent",              l_get_contentstruct},
    {"getAttrib" ,              l_get_struct_attribute},
    {"getMembers",              l_get_struct_members},
    {"getMemberType",           l_get_struct_member_type},
    {"getMemberContentType",    l_get_struct_member_content_type},
    {"getMemberContentStruct",  l_get_struct_member_content_struct},
    {"isOrInheritsFrom",        l_struct_is_or_inherits_from},
    {NULL,                      NULL}
  };
  static luaL_reg grtModules[]= {
    {"list",          l_list_modules},
    {"show",          l_show_module},
    {"get",           l_get_modules},
    {"getFunctions",  l_get_module_functions},
    {"callFunction",  l_call_function},
    {NULL,         NULL}
  };
  static luaL_reg grtUtil[]= {
    {"regExVal",          l_regex_val},
    {"replace",           l_str_replace},
    {NULL,         NULL}
  };

  // override the built-in print with our own
  lua_register(_lua, "print", l_print);

  // replace ipairs and pairs with versions that support GRT lists/dicts
  set_closure(_lua, "ipairs", l_grt_ipairs, l_grt_ipairs_aux);
//  set_closure(_lua, "pairs", l_grt_pairs, l_grt_pairs_aux);

  

  // register our exported functions

  lua_register(_lua, "trace", l_trace_enable);
  lua_register(_lua, "backtrace", l_backtrace);
  
  lua_register(_lua, "refresh", l_refresh);

  lua_register(_lua, "cd", l_cd);
  lua_register(_lua, "ls", l_ls);
  lua_register(_lua, "dir", l_ls);
  lua_register(_lua, "pwd", l_pwd);

  lua_register(_lua, "run", l_run);
  lua_register(_lua, "exit", l_exit);
  lua_register(_lua, "sleep", l_sleep);

  luaL_openlib(_lua, "grtV", grtValue, 0);
  lua_pop(_lua, 1);
  luaL_openlib(_lua, "grtS", grtStruct, 0);
  lua_pop(_lua, 1);
  luaL_openlib(_lua, "grtM", grtModules, 0);
  lua_pop(_lua, 1);
  luaL_openlib(_lua, "grtU", grtUtil, 0);
  lua_pop(_lua, 1);
}



