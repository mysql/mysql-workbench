/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define DEFINE_TEST_MODULES_CODE
#include "wb_test_helpers.h"
#include "structs.test.h"
#include "grtpp_module_cpp.h"
#include "test_modules.h"

#include "casmine.h"

class TestModuleImpl : public grt::ModuleImplBase { // this module does not implement everything from the interface
public:
  TestModuleImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE("1.0", "", grt::ModuleImplBase, DECLARE_MODULE_FUNCTION(TestModuleImpl::returnNull), NULL);

  grt::ObjectRef returnNull() {
    return grt::ObjectRef();
  }
};

namespace {

$ModuleEnvironment() {};

$describe("GRT: C++ modules") {

  $beforeAll([]() {
    // We have to excplicitly cleanup grt, as we don't know which spec was before
    // otherwise this one will fail.
    grt::GRT::get()->reinitialiseForTests();
    register_structs_test_xml();
    grt::GRT::get()->load_metaclasses(casmine::CasmineContext::get()->tmpDataDir() + "/structs.test.xml");
    grt::GRT::get()->end_loading_metaclasses();
  });

  $afterAll([]() {
    WorkbenchTester::reinitGRT();
  });

  $it("Load structures", [&]() {
    $expect(grt::GRT::get()->get_metaclasses().size()).toBe(6U);
  });

  $it("Load invalid module", [&]() {

    // this is exactly what should be done
    // by module dll during initialization
    grt::InterfaceImplBase::Register<SampleInterface1Impl>();
    grt::InterfaceImplBase::Register<SampleInterface2Impl>();

    grt::GRT::get()->get_native_module<SampleModule1Impl>();
    grt::GRT::get()->get_native_module<SampleModule2Impl>();
    grt::GRT::get()->get_native_module<SampleModule3Impl>();

    grt::GRT::get()->get_native_module<TestModuleImpl>();

    $expect([&](){ grt::GRT::get()->get_native_module<BadModuleImpl>(); }).toThrow();
  });

  $it("Module Interface registration", [&](){
    $expect(grt::GRT::get()->get_interfaces().size()).toBe(2UL);
    $expect(grt::GRT::get()->get_modules().size()).toBe(4UL);

    // interfaces[0]
    const grt::Interface *iface = grt::GRT::get()->get_interface("SampleInterface1");

    $expect(iface->name()).toBe("SampleInterface1");
    $expect(iface->get_functions().size()).toBe(2U);
    $expect(iface->extends().empty()).toBeTrue();
    $expect(iface->get_interfaces().empty()).toBeTrue();

    // virtual int getNumber()= 0;
    const grt::Module::Function *f = &iface->get_functions()[0];

    $expect(f->arg_types.empty()).toBeTrue();
    $expect(f->name).toBe("getNumber");
    $expect((int)f->ret_type.base.type).toBe(grt::IntegerType);
    $expect(f->ret_type.base.object_class).toBe("");
    $expect((int)f->ret_type.content.type).toBe(grt::UnknownType);
    $expect(f->ret_type.content.object_class).toBe("");

    // virtual int calculate()= 0;
    f = &iface->get_functions()[1];

    $expect(f->arg_types.empty()).toBeTrue();
    $expect(f->name).toBe("calculate");
    $expect((int)f->ret_type.base.type).toBe(grt::IntegerType);
    $expect(f->ret_type.base.object_class).toBe("");

    // interfaces[1]
    iface = grt::GRT::get()->get_interface("SampleInterface2");

    $expect(iface->name()).toBe("SampleInterface2");
    $expect(iface->get_functions().size()).toBe(1U);
    $expect(iface->extends().empty()).toBeTrue();
    $expect(iface->get_interfaces().empty()).toBeTrue();

    // virtual int calcSum(int num1)= 0;
    f = &iface->get_functions()[0];

    $expect(f->arg_types.size()).toBe(1U);
    $expect((int)f->arg_types[0].type.base.type).toBe(grt::IntegerType);
    $expect(f->arg_types[0].type.base.object_class.empty()).toBeTrue();
    $expect(f->name).toBe("calcSum");
    $expect((int)f->ret_type.base.type).toBe(grt::IntegerType);
    $expect(f->ret_type.base.object_class.empty()).toBeTrue();
  });

  $it("Module loading and interaction", [&]() {
    // TODO: this test cannot run alone, as it requires at least one module to be registered.
    const grt::Module::Function *f;

    $expect(grt::GRT::get()->get_modules().empty()).toBeFalse();
    grt::Module *m = grt::GRT::get()->get_modules()[0];

    $expect(m).toBeInstanceOf<SampleModule1Impl>();

    $expect(m->name()).toBe("SampleModule1");
    $expect(m->get_functions().size()).toEqual(2U);
    $expect(m->extends().empty()).toBeTrue();
    $expect(grt::GRT::get()->get_interface(m->get_interfaces()[0])).Not.toBe(nullptr);

    // int getNumber();
    f = &m->get_functions()[0];

    $expect(f->arg_types.size()).toEqual(0U);
    $expect(f->name).toBe("getNumber");
    $expect((int)f->ret_type.base.type).toBe(grt::IntegerType);
    $expect(f->ret_type.base.object_class).toBe("");

    // int calculate();
    f = &m->get_functions()[1];

    $expect(f->arg_types.size()).toEqual(0U);
    $expect(f->name).toBe("calculate");
    $expect((int)f->ret_type.base.type).toBe(grt::IntegerType);
    $expect(f->ret_type.base.object_class).toBe("");

    // modules[1]
    m = grt::GRT::get()->get_modules()[1];

    $expect(m->name()).toBe("SampleModule2");
    $expect(m->get_functions().size()).toEqual(1U);
    $expect(m->extends().empty()).toBeTrue();
    $expect(grt::GRT::get()->get_interface(m->get_interfaces()[0])).Not.toBe(nullptr);

    // virtual int calcSum(int num1)= 0;
    f = &m->get_functions()[0];

    $expect(f->arg_types.size()).toEqual(1U);
    $expect((int)f->arg_types[0].type.base.type).toBe(grt::IntegerType);
    $expect(f->arg_types[0].type.base.object_class).toBe("");
    $expect(f->name).toBe("calcSum");
    $expect((int)f->ret_type.base.type).toBe(grt::IntegerType);
    $expect(f->ret_type.base.object_class).toBe("");

    // modules[2]
    m = grt::GRT::get()->get_modules()[2];

    $expect(m->name()).toBe("SampleModule3");
    $expect(m->get_functions().size()).toEqual(6U);
    $expect(m->extends()).toBe("SampleModule2");

    // doSomethingWithNumbers
    f = &m->get_functions()[0];

    $expect(f->arg_types.size()).toEqual(4U);
    $expect((int)f->arg_types[0].type.base.type).toBe(grt::IntegerType);
    $expect(f->arg_types[0].type.base.object_class).toBe("");
    $expect((int)f->arg_types[1].type.base.type).toBe(grt::DoubleType);
    $expect(f->arg_types[1].type.base.object_class).toBe("");
    $expect((int)f->arg_types[2].type.base.type).toBe(grt::IntegerType);
    $expect(f->arg_types[2].type.base.object_class).toBe("");
    $expect((int)f->arg_types[3].type.base.type).toBe(grt::DoubleType);
    $expect(f->arg_types[3].type.base.object_class).toBe("");
    $expect(f->name).toBe("doSomethingWithNumbers");
    $expect((int)f->ret_type.base.type).toBe(grt::StringType);
    $expect(f->ret_type.base.object_class).toBe("");

    // doSomethingWithObject
    f = &m->get_functions()[1];

    $expect(f->arg_types.size()).toEqual(1U);
    $expect((int)f->arg_types[0].type.base.type).toBe(grt::ObjectType);
    // XXX
    $expect(f->arg_types[0].type.base.object_class).toBe("Object");
    $expect(f->name).toBe("doSomethingWithObject");
    $expect((int)f->ret_type.base.type).toBe(grt::IntegerType);
    $expect(f->ret_type.base.object_class).toBe("");

    // doSomethingWithNumberList
    f = &m->get_functions()[2];

    $expect(f->arg_types.size()).toEqual(1U);
    $expect((int)f->arg_types[0].type.base.type).toBe(grt::ListType);
    $expect(f->arg_types[0].type.base.object_class).toBe("");
    $expect((int)f->arg_types[0].type.content.type).toBe(grt::IntegerType);
    $expect(f->arg_types[0].type.content.object_class).toBe("");
    $expect(f->name).toBe("doSomethingWithNumberList");
    $expect((int)f->ret_type.base.type).toBe(grt::IntegerType);
    $expect(f->ret_type.base.object_class).toBe("");

    // doSomethingWithTypedObject
    f = &m->get_functions()[3];

    $expect(f->arg_types.size()).toEqual(2U);
    $expect((int)f->arg_types[0].type.base.type).toBe(grt::StringType);
    $expect((int)f->arg_types[1].type.base.type).toBe(grt::ObjectType);
    $expect(f->arg_types[1].type.base.object_class).toBe("test.Author"); // unsure
    $expect(f->name).toBe("doSomethingWithTypedObject");
    $expect((int)f->ret_type.base.type).toBe(grt::ListType);
    $expect(f->ret_type.base.object_class).toBe("");
  });

  $it("test module calling", [&]() {
    grt::Module *module = grt::GRT::get()->get_module("SampleModule1");

    $expect(module).Not.toBe(nullptr);

    grt::BaseListRef args(grt::AnyType);
    grt::ValueRef result;

    result = module->call_function("getNumber", args);
    $expect(*grt::IntegerRef::cast_from(result)).toBe(42);
  });

  $it("functions returning NULL value were causing exception", [&]() {
    grt::Module *module = grt::GRT::get()->get_module("TestModule");

    $expect(module).Not.toBe(nullptr);

    grt::BaseListRef args(grt::AnyType);
    grt::ValueRef result;

    result = module->call_function("returnNull", args);
    $expect(result.is_valid()).toBeFalse();
  });

};

}
