/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "grtpp_module_cpp.h"
#include "structs.test.h"

#define DEFINE_TEST_MODULES_CODE
#include "test_modules.h"

BEGIN_TEST_DATA_CLASS(grt_module_native)
protected:
END_TEST_DATA_CLASS

class TestModuleImpl : public ModuleImplBase { // this module does not implement everything from the interface
public:
  TestModuleImpl(CPPModuleLoader *ldr) : ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE("1.0", "", ModuleImplBase, DECLARE_MODULE_FUNCTION(TestModuleImpl::returnNull), NULL);

  ObjectRef returnNull() {
    return ObjectRef();
  }
};

TEST_MODULE(grt_module_native, "GRT: C++ modules");

TEST_FUNCTION(1) {
  grt::GRT::get()->load_metaclasses("data/structs.test.xml");
  grt::GRT::get()->end_loading_metaclasses();

  // this is exactly what should be done
  // by module dll during initialization
  InterfaceImplBase::Register<SampleInterface1Impl>();
  InterfaceImplBase::Register<SampleInterface2Impl>();

  grt::GRT::get()->get_native_module<SampleModule1Impl>();
  grt::GRT::get()->get_native_module<SampleModule2Impl>();
  grt::GRT::get()->get_native_module<SampleModule3Impl>();

  grt::GRT::get()->get_native_module<TestModuleImpl>();

  try {
    grt::GRT::get()->get_native_module<BadModuleImpl>();
    ensure("register uncompliant module", false);
  } catch (...) {
  }
}

// test module/interface registration

TEST_FUNCTION(4) {
  ensure_equals("number of interfaces", grt::GRT::get()->get_interfaces().size(), 2U);
  ensure_equals("number of modules", grt::GRT::get()->get_modules().size(), 4U);

  // interfaces[0]
  const Interface *iface = grt::GRT::get()->get_interface("SampleInterface1");

  ensure_equals("interfaces[0] name", iface->name(), "SampleInterface1");
  ensure("interfaces[0] functions_num", iface->get_functions().size() == 2);
  ensure("interfaces[0] extends", iface->extends().empty());
  ensure("interfaces[0] implements", iface->get_interfaces().empty());

  // virtual int getNumber()= 0;
  const grt::Module::Function *f = &iface->get_functions()[0];

  ensure("interfaces[0] functions[0] params_num", f->arg_types.empty());
  ensure("interfaces[0] functions[0] name", f->name == "getNumber");
  ensure("interfaces[0] functions[0] return_type", f->ret_type.base.type == IntegerType);
  ensure("interfaces[0] functions[0] return_object_class", f->ret_type.base.object_class == "");
  ensure("interfaces[0] functions[0] return_type", f->ret_type.content.type == UnknownType);
  ensure("interfaces[0] functions[0] return_object_class", f->ret_type.content.object_class == "");

  // virtual int calculate()= 0;
  f = &iface->get_functions()[1];

  ensure("interfaces[0] functions[1] params_num", f->arg_types.empty());
  ensure("interfaces[0] functions[1] name", f->name == "calculate");
  ensure("interfaces[0] functions[1] return_type", f->ret_type.base.type == IntegerType);
  ensure("interfaces[0] functions[1] return_object_class", f->ret_type.base.object_class == "");

  // interfaces[1]
  iface = grt::GRT::get()->get_interface("SampleInterface2");

  ensure("interfaces[1] name", iface->name() == "SampleInterface2");
  ensure("interfaces[1] functions_num", iface->get_functions().size() == 1);
  ensure("interfaces[1] extends", iface->extends().empty());
  ensure("interfaces[1] implements", iface->get_interfaces().empty());

  // virtual int calcSum(int num1)= 0;
  f = &iface->get_functions()[0];

  ensure("interfaces[1] functions[0] params_num", f->arg_types.size() == 1);
  ensure("interfaces[1] functions[0] params[0] type", f->arg_types[0].type.base.type == IntegerType);
  ensure("interfaces[1] functions[0] params[0] object_class", f->arg_types[0].type.base.object_class.empty());
  ensure("interfaces[1] functions[0] name", f->name == "calcSum");
  ensure("interfaces[1] functions[0] return_type", f->ret_type.base.type == IntegerType);
  ensure("interfaces[1] functions[0] return_object_class", f->ret_type.base.object_class.empty());
}

TEST_FUNCTION(5) {
  const grt::Module::Function *f;

  ensure("No Modules loaded", !grt::GRT::get()->get_modules().empty());
  // modules[0]
  grt::Module *m = grt::GRT::get()->get_modules()[0];

  ensure("modules[0] name", m->name() == "SampleModule1");
  ensure("modules[0] functions_num", m->get_functions().size() == 2);
  ensure("modules[0] extends", m->extends().empty());
  ensure("modules[0] implements", grt::GRT::get()->get_interface(m->get_interfaces()[0]) != 0);

  // int getNumber();
  f = &m->get_functions()[0];

  ensure("modules[0] functions[0] params_num", f->arg_types.size() == 0);
  ensure("modules[0] functions[0] name", f->name == "getNumber");
  ensure("modules[0] functions[0] return_type", f->ret_type.base.type == IntegerType);
  ensure("modules[0] functions[0] return_object_class", f->ret_type.base.object_class == "");

  // int calculate();
  f = &m->get_functions()[1];

  ensure("modules[0] functions[1] params_num", f->arg_types.size() == 0);
  ensure("modules[0] functions[1] name", f->name == "calculate");
  ensure("modules[0] functions[0] return_type", f->ret_type.base.type == IntegerType);
  ensure("modules[0] functions[0] return_object_class", f->ret_type.base.object_class == "");

  // modules[1]
  m = grt::GRT::get()->get_modules()[1];

  ensure("modules[1] name", m->name() == "SampleModule2");
  ensure("modules[1] functions_num", m->get_functions().size() == 1);
  ensure("modules[1] extends", m->extends().empty());
  ensure("modules[1] implements", grt::GRT::get()->get_interface(m->get_interfaces()[0]) != 0);

  // virtual int calcSum(int num1)= 0;
  f = &m->get_functions()[0];

  ensure("modules[1] functions[0] params_num", f->arg_types.size() == 1);
  ensure("modules[1] functions[0] params[0] type", f->arg_types[0].type.base.type == IntegerType);
  ensure("modules[1] functions[0] params[0] object_class", f->arg_types[0].type.base.object_class == "");
  ensure("modules[1] functions[0] name", f->name == "calcSum");
  ensure("modules[1] functions[0] return type", f->ret_type.base.type == IntegerType);
  ensure("modules[1] functions[0] return object_class", f->ret_type.base.object_class == "");

  // modules[2]
  m = grt::GRT::get()->get_modules()[2];

  ensure("modules[2] name", m->name() == "SampleModule3");
  ensure("modules[2] functions_num", m->get_functions().size() == 6);
  ensure_equals("modules[2] extends", m->extends(), "SampleModule2");
  // ensure("modules[1] implements", m->implements == NULL);

  // doSomethingWithNumbers
  f = &m->get_functions()[0];

  ensure("modules[2] functions[0] params_num", f->arg_types.size() == 4);
  ensure("modules[2] functions[0] params[0] type", f->arg_types[0].type.base.type == IntegerType);
  ensure("modules[2] functions[0] params[0] object_class", f->arg_types[0].type.base.object_class == "");
  ensure("modules[2] functions[0] params[1] type", f->arg_types[1].type.base.type == DoubleType);
  ensure("modules[2] functions[0] params[1] object_class", f->arg_types[1].type.base.object_class == "");
  ensure("modules[2] functions[0] params[2] type", f->arg_types[2].type.base.type == IntegerType);
  ensure("modules[2] functions[0] params[2] object_class", f->arg_types[2].type.base.object_class == "");
  ensure("modules[2] functions[0] params[3] type", f->arg_types[3].type.base.type == DoubleType);
  ensure("modules[2] functions[0] params[3] object_class", f->arg_types[3].type.base.object_class == "");
  ensure("modules[2] functions[0] name", f->name == "doSomethingWithNumbers");
  ensure("modules[2] functions[0] return type", f->ret_type.base.type == StringType);
  ensure("modules[2] functions[0] return object_class", f->ret_type.base.object_class == "");

  // doSomethingWithObject
  f = &m->get_functions()[1];

  ensure("modules[2] functions[1] params_num", f->arg_types.size() == 1);
  ensure("modules[2] functions[1] params[0] type", f->arg_types[0].type.base.type == grt::ObjectType);
  // XXX
  ensure_equals("modules[2] functions[1] params[0] object_class", f->arg_types[0].type.base.object_class, "Object");
  ensure("modules[2] functions[1] name", f->name == "doSomethingWithObject");
  ensure("modules[2] functions[1] return type", f->ret_type.base.type == IntegerType);
  ensure("modules[2] functions[1] return object_class", f->ret_type.base.object_class == "");

  // doSomethingWithNumberList
  f = &m->get_functions()[2];

  ensure("modules[2] functions[2] params_num", f->arg_types.size() == 1);
  ensure("modules[2] functions[2] params[0] type", f->arg_types[0].type.base.type == ListType);
  ensure("modules[2] functions[2] params[0] object_class", f->arg_types[0].type.base.object_class == "");
  ensure("modules[2] functions[2] params[0] content_type", f->arg_types[0].type.content.type == IntegerType);
  ensure("modules[2] functions[2] params[0] content_object_class", f->arg_types[0].type.content.object_class == "");
  ensure("modules[2] functions[2] name", f->name == "doSomethingWithNumberList");
  ensure("modules[2] functions[2] return type", f->ret_type.base.type == IntegerType);
  ensure("modules[2] functions[2] return object_class", f->ret_type.base.object_class == "");

  // doSomethingWithTypedObject
  f = &m->get_functions()[3];

  ensure("modules[2] functions[3] params_num", f->arg_types.size() == 2);
  ensure("modules[2] functions[3] params[0] type", f->arg_types[0].type.base.type == StringType);
  ensure("modules[2] functions[3] params[1] type", f->arg_types[1].type.base.type == grt::ObjectType);
  ensure_equals("modules[2] functions[3] params[1] object_class", f->arg_types[1].type.base.object_class,
                "test.Author"); // unsure
  ensure("modules[2] functions[3] name", f->name == "doSomethingWithTypedObject");
  ensure("modules[2] functions[3] return type", f->ret_type.base.type == ListType);
  ensure("modules[2] functions[3] return object_class", f->ret_type.base.object_class == "");
}

TEST_FUNCTION(6) { // test module calling
  grt::Module *module = grt::GRT::get()->get_module("SampleModule1");

  ensure("get SampleModule1", module != NULL);

  BaseListRef args(AnyType);
  ValueRef result;

  result = module->call_function("getNumber", args);
  ensure_equals("getNumber", *IntegerRef::cast_from(result), 42);
}

TEST_FUNCTION(7) { // functions returning NULL value were causing exception
  grt::Module *module = grt::GRT::get()->get_module("TestModule");

  ensure("get TestModule", module != NULL);

  BaseListRef args(AnyType);
  ValueRef result;

  result = module->call_function("returnNull", args);
  ensure("returnNull", !result.is_valid());
}

END_TESTS
