/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "testgrt.h"
#include "grt_test_utility.h"
#include "grtpp_module_lua.h"
#include "structs.test.h"


BEGIN_TEST_DATA_CLASS(grtpp_lua_test)
public:
  GRT grt;
  LuaModuleLoader *loader;
END_TEST_DATA_CLASS


TEST_MODULE(grtpp_lua_test, "GRT: lua support");

TEST_FUNCTION(1)
{
  // init lua loader

  grt.add_module_loader(loader= new LuaModuleLoader(&grt));
  ensure("init lua loader", loader != NULL);

  grt.load_metaclasses("data/structs.test.xml");
  grt.end_loading_metaclasses();
  ensure_equals("load structs", grt.get_metaclasses().size(), 6U);
  
  ensure_equals("module count", grt.get_modules().size(), 0U);
  
  grt.load_module("../../library/grt/unit-tests/lua_module.lua", true);
  
  ensure_equals("module count after load", grt.get_modules().size(), 1U);
}


TEST_FUNCTION(2)
{
  test_BookRef object(&grt);
  LuaContext *ctx= loader->get_lua_context();
  
  ctx->push_wrap_value(object);
  
  ValueRef value= ctx->pop_value();
  ensure("value", value.is_valid());
  ensure("is object", test_BookRef::can_wrap(value));
//  ensure_equals("refcount", value.refcount(), 1);

  BaseListRef list(&grt);
  list.ginsert(object);
  
  ctx->push_wrap_value(list);
  value= ctx->pop_value();
  
  ensure("value", value.is_valid());
  ensure("is list", BaseListRef::can_wrap(value));
  ensure("list", value == list);
}


TEST_FUNCTION(3)
{
  test_BookRef object(&grt);
  
  LuaContext *ctx= loader->get_lua_context();
  
  ctx->push_wrap_value(object);
  
  {
    ObjectRef obj;
  
    ctx->pop_args("O", &obj);
    ensure("pop value", obj.is_valid());
    ctx->call_gc();

    ensure_equals("refcount", obj.refcount(), 2);
  }
  ensure_equals("final refcount", object.refcount(), 1);
}


TEST_FUNCTION(12)
{
  Module *test_module= grt.get_module("LuaModuleTest");
  ensure("get module LuaModuleTest", test_module!=NULL);

  BaseListRef args(&grt);
  ValueRef res;

  args.ginsert(IntegerRef(2));
  args.ginsert(IntegerRef(3));
  res= test_module->call_function("add2Numbers", args);
  ensure_equals("add2Numbers", (int)IntegerRef::cast_from(res), 5);

  IntegerListRef nlist(&grt);

  args= BaseListRef(&grt);
  args.ginsert(nlist);
  
  nlist.ginsert(IntegerRef(2));
  nlist.ginsert(IntegerRef(3));
  nlist.ginsert(IntegerRef(4));
  nlist.ginsert(IntegerRef(5));

  res= test_module->call_function("sumList", args);

  ensure_equals("sumList", (int)IntegerRef::cast_from(res), 14);
                             
}

/*
purpose:
  to avoid code duplication for checking of test_module functions like testArgInt, testArgDouble.
usage:
  TEST_FUNCTION(13)
*/
template <class GrtType, class StdType>
void test_num_arg(GRT &grt, Module *test_module, const char *func_name)
{
  BaseListRef args(&grt);

  GrtType v1((StdType)2.1);
  GrtType v2((StdType)3.1);
  args.ginsert(v1);
  args.ginsert(v2);

  check_module_function_return(test_module, func_name, args, GrtType(((StdType)v1)+((StdType)v2)));
}

/*
purpose:
  to avoid code duplication for checking of test_module functions like testArgIntList, testArgDoubleList.
logic description:
  create arithmethic progression of given length & pass it to mentioned functions.
usage:
  TEST_FUNCTION(13)
*/
template <class GrtType, class StdType>
void test_num_arg_list(GRT &grt, Module *test_module, const char *func_name, size_t count)
{
  BaseListRef args(&grt);

  Ref<GrtType> n= (StdType)1.1;
  Ref<GrtType> sum= (StdType)0;

  ListRef<GrtType> vlist(&grt);
  args.ginsert(vlist);

  for (size_t i= 0; i<count; i++)
    vlist.ginsert(Ref<GrtType>((sum= sum+(n= n+1), n)));

  args.ginsert(Ref<GrtType>((sum= sum+(n= n+1), n)));

  check_module_function_return(test_module, func_name, args, sum);
}

TEST_FUNCTION(13)
{ // test passing of all types
  Module *test_module= grt.get_module("LuaModuleTest");
  ensure("get module", test_module!=NULL);

  test_num_arg<IntegerRef, int>(grt, test_module, "testArgInt");
  test_num_arg<DoubleRef, double>(grt, test_module, "testArgDouble");
//  test_num_arg_list<grt::internal::Integer, int>(grt, test_module, "testArgIntList", 4);
//  test_num_arg_list<grt::internal::Double, double>(grt, test_module, "testArgDoubleList", 4);

  // testArgString
  {
    const std::string S1("unit-");
    const std::string S2("test");

    BaseListRef args(&grt, AnyType);
    args.ginsert(StringRef(S1));
    args.ginsert(StringRef(S2));
    check_module_function_return(test_module, "testArgString", args, StringRef(S1+S2));
  }
}

/*
TEST_FUNCTION(14)
{ // test passing of all types
  Module *test_module= grt.get_module("LuaModuleTest");

  ensure("LuaModuleTest is not null", test_module != 0);  
  // testArgStringList
  {
    const int INDEX(2); // lua uses 1-based index
    const size_t LIST_COUNT(4);
    const std::string s[LIST_COUNT]= {"_0", "_1", "_2", "_3"};

    BaseListRef args(&grt);
    StringListRef list= list_from_array<grt::internal::String>(grt, s, LIST_COUNT);
    args.ginsert(list);
    args.ginsert(IntegerRef(INDEX+1));
    check_module_function_return(test_module, "testArgStringList", args, list.get(INDEX));
  }
}
*/
TEST_FUNCTION(15)
{ // test passing of all types
  Module *test_module= grt.get_module("LuaModuleTest");

  ensure("LuaModuleTest is not null", test_module != 0);  
  // testArgObject
  {
    const char *MEMBER_NAME("name");
    const char *MEMBER_VAL("author");
    const char *S(" wrote some stuff");
    const char *OBJ_PATH("test.Author");

    BaseListRef args(&grt, AnyType);
    ObjectRef obj(grt.create_object<grt::internal::Object>(OBJ_PATH));
    args.ginsert(obj);
    obj.set_member(MEMBER_NAME, StringRef(MEMBER_VAL));
    args.ginsert(StringRef(S));
    check_module_function_return(test_module, "testArgObject", args, StringRef(std::string(MEMBER_VAL)+std::string(S)));
  }
}

TEST_FUNCTION(16)
{ // test passing of all types
  Module *test_module= grt.get_module("LuaModuleTest");

  ensure("LuaModuleTest is not null", test_module != 0);  
  // testArgObjectList
  {
    const char *OBJ_PATH("test.Author");
    const char *MEMBER_NAME("name");
    const size_t VAL_COUNT= 4;
    const size_t VAL_INDEX= 2;
    const std::string VA[VAL_COUNT]= {"author1", "author2", "author3", "author4"};

    BaseListRef args(&grt, AnyType);
    ObjectListRef list(&grt);
    args.ginsert(list);
    for (size_t n= 0; n<VAL_COUNT; n++)
    {
      ObjectRef obj(grt.create_object<grt::internal::Object>(OBJ_PATH));
      obj.set_member(MEMBER_NAME, StringRef(VA[n]));
      list.insert(obj);
    }
    args.ginsert(StringRef(VA[VAL_INDEX]));
    check_module_function_return(test_module, "testArgObjectList", args, StringRef(VA[VAL_INDEX]));
  }
}

TEST_FUNCTION(17)
{ // test passing of all types
  Module *test_module= grt.get_module("LuaModuleTest");

  ensure("LuaModuleTest is not null", test_module != 0);  
  // testArgDict
  {
    const size_t KEY_COUNT(4);
    const int INDEX(2);
    std::string k[KEY_COUNT]= {"id", "code", "name", "material"};
    std::string v[KEY_COUNT]= {"1", "001", "wrench", "metal"};

    BaseListRef args(&grt, AnyType);
    DictRef dict= dict_from_array<StringRef>(grt, k, v, KEY_COUNT);
    args.ginsert(dict);
    args.ginsert(StringRef(k[INDEX]));
    check_module_function_return(test_module, "testArgDict", args, StringRef(v[INDEX]));
  }

}

TEST_FUNCTION(18)
{ // test passing of all types
  Module *test_module= grt.get_module("LuaModuleTest");

  ensure("LuaModuleTest is not null", test_module != 0);  
  // testArgDictList
  {
    const size_t DICT_COUNT(4);
    const size_t KEY_COUNT(4);
    const size_t SEARCHED_DICT(1);
    const size_t NAME_KEY(2);
    std::string k[KEY_COUNT]= {"id","code","name","material"};
    std::string v[DICT_COUNT][KEY_COUNT]= {
      {"1","001","fork","metal"},
      {"2","002","ball","rubber"},
      {"3","003","table","wood"},
      {"4","004","window","glass"}};

    BaseListRef args(&grt, AnyType);
    DictRef dict_arr[DICT_COUNT];
    BaseListRef list(&grt);
    for (size_t n= 0; n<DICT_COUNT; n++)
    {
      dict_arr[n]= dict_from_array<StringRef>(grt, k, v[n], KEY_COUNT);
      list.ginsert(dict_arr[n]);
    }
    args.ginsert(list);
    args.ginsert(StringRef(v[SEARCHED_DICT][NAME_KEY]));
    check_module_function_return(test_module, "testArgDictList", args, DictRef(dict_arr[SEARCHED_DICT]));
  }
}

/*
usage:
  TEST_FUNCTION(20)
*/
void test_return_of_all_types(GRT& grt, Module* test_module, BaseListRef& args, bool grt_value_return)
{
  while (args.count())
    args.remove(0);
  args.ginsert(IntegerRef(grt_value_return?0:1));

  // testRetAny
  {
    int v[]= {grt_value_return?123456:12345};
    check_module_function_return(test_module, "testRetAny", args, list_from_array<grt::internal::Integer>(grt, v, 1));
  }
  check_module_function_return(test_module, "testRetInt", args, IntegerRef(123));
  check_module_function_return(test_module, "testRetDouble", args, DoubleRef(123.456));
  check_module_function_return(test_module, "testRetString", args, StringRef("hello"));

  check_module_function_return(test_module, "testRetObject", args, grt.create_object<grt::internal::Object>("test.Book"));

  // testRetDict
  {
    const size_t KEY_COUNT(3);
    std::string k1[KEY_COUNT]= {"k1","k2","k3"};
    std::string k0[KEY_COUNT]= {"key1","key2","key3"};
    std::string *k(grt_value_return?k0:k1);
    int v[KEY_COUNT]= {1,2,3};

    DictRef dict= dict_from_array<IntegerRef>(grt, k, v, KEY_COUNT);
    check_module_function_return(test_module, "testRetDict", args, dict);
  }

  // testRetList
  {
    BaseListRef list(&grt, AnyType);
    list.ginsert(IntegerRef(grt_value_return?2:1));
    list.ginsert(DoubleRef(grt_value_return?3.1:2.1));
    list.ginsert(StringRef(grt_value_return?"four":"three"));
    check_module_function_return(test_module, "testRetList", args, list);
  }

  // testRetIntList
  {
    const size_t LIST_COUNT(4);
    int v1[LIST_COUNT]= {1,2,3,4};
    int v0[LIST_COUNT]= {2,3,4,5};
    int *v(grt_value_return?v0:v1);

    IntegerListRef list= list_from_array<grt::internal::Integer>(grt, v, LIST_COUNT);
    check_module_function_return(test_module, "testRetIntList", args, list);
  }

  // testRetDoubleList
  {
    const size_t LIST_COUNT(4);
    double v1[]= {1.1, 2.2, 3.3, 4.4};
    double v0[]= {2.2, 3.3, 4.4, 5.5};
    double *v(grt_value_return?v0:v1);

    DoubleListRef list= list_from_array<grt::internal::Double>(grt, v, LIST_COUNT);
    check_module_function_return(test_module, "testRetDoubleList", args, list);
  }
/*
  // testRetStringList
  {
    const size_t LIST_COUNT(4);
    std::string v1[]= {"one","two","three","four"};
    std::string v0[]= {"two","three","four","five"};
    std::string *v(grt_value_return?v0:v1);

    StringListRef list= list_from_array<grt::internal::String>(grt, v, LIST_COUNT);
    check_module_function_return(test_module, "testRetStringList", args, list);
  }
  */
  // testRetObjectList
  {
    const char *OBJ_PATH("test.Book");
    const char *MEMBER_NAME("title");
    const size_t VAL_COUNT(3);
    const StringRef VA[VAL_COUNT]= {"Book1", "Book2", "Book3"};

    ObjectListRef list(&grt);
    for (size_t n= 0; n<VAL_COUNT; n++)
    {
      list.insert(grt.create_object<grt::internal::Object>(OBJ_PATH));
      list.get(n).set_member(MEMBER_NAME, VA[n]);
    }
    check_module_function_return(test_module, "testRetObjectList", args, list);
  }

  // testRetDictList
  {
    const size_t DICT_COUNT(2);
    const size_t KEY_COUNT(2);
    std::string k1[KEY_COUNT]= {"k1","k2"};
    std::string k0[KEY_COUNT]= {"key1","key2"};
    std::string *k(grt_value_return?k0:k1);
    int v[DICT_COUNT][KEY_COUNT]= {
      {1,2},
      {11,22}};

    DictRef dict_arr[DICT_COUNT];
    BaseListRef list(&grt);
    for (size_t n= 0; n<DICT_COUNT; n++)
    {
      dict_arr[n]= dict_from_array<IntegerRef>(grt, k, v[n], KEY_COUNT);
      list.ginsert(dict_arr[n]);
    }
    check_module_function_return(test_module, "testRetDictList", args, list);
  }
}

TEST_FUNCTION(20)
{ // test return of all types
  Module *test_module= grt.get_module("LuaModuleTest");
  ensure("get module", test_module!=NULL);
  BaseListRef args(&grt, AnyType);

  test_return_of_all_types(grt, test_module, args, false);
  test_return_of_all_types(grt, test_module, args, true);
}


TEST_FUNCTION(21)
{
  // test error catching
  Module *test_module= grt.get_module("LuaModuleTest");
  ensure("get module", test_module!=NULL);

  BaseListRef args(&grt);
  ValueRef res;

  args.ginsert(IntegerRef(2));
  args.ginsert(IntegerRef(3));
  try {
    res= test_module->call_function("errorFunc", args);
    ensure("lua module error handling", false);
  } catch (grt::module_error&) {
    // this is the correct exception to be raised
  } catch (...) {
    ensure("lua module error exception", false);
  }
}


TEST_FUNCTION(22)
{
  // some unit tests written in lua
  Module *test_module= grt.get_module("LuaModuleTest");
  ensure("LuaModuleTest is not null", test_module != 0);
  BaseListRef args(&grt, AnyType);  

  ValueRef res= test_module->call_function("doLuaUnitTests", args);

  ensure_equals("lua unit tests", IntegerRef::cast_from(res), 0);
}


END_TESTS
