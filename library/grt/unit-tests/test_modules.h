/*
* Copyright (c) 2005, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "grt.h"
#include "grtpp_module_cpp.h"
#include "structs.test.h"

using namespace grt;

struct SampleInterface1Impl : protected InterfaceImplBase {
public:
  DECLARE_REGISTER_INTERFACE(SampleInterface1Impl, DECLARE_INTERFACE_FUNCTION(SampleInterface1Impl::getNumber),
                             DECLARE_INTERFACE_FUNCTION(SampleInterface1Impl::calculate));

  virtual ~SampleInterface1Impl() {
  }

  virtual int getNumber() = 0;
  virtual int calculate() = 0;
};

struct SampleInterface2Impl : protected InterfaceImplBase {
public:
  DECLARE_REGISTER_INTERFACE(SampleInterface2Impl, DECLARE_INTERFACE_FUNCTION(SampleInterface2Impl::calcSum));

  virtual ~SampleInterface2Impl() {
  }

  virtual int calcSum(int num1) = 0;
};

class SampleModule1Impl : public ModuleImplBase, public SampleInterface1Impl {
public:
  SampleModule1Impl(CPPModuleLoader *ldr) : ModuleImplBase(ldr) {
  }
  virtual ~SampleModule1Impl() {
  }

  DEFINE_INIT_MODULE("1.0", "", ModuleImplBase, DECLARE_MODULE_FUNCTION(SampleModule1Impl::getNumber),
                     DECLARE_MODULE_FUNCTION(SampleModule1Impl::calculate));

  int getNumber() {
    return 42;
  }

  int calculate();
};

class SampleModule2Impl : public ModuleImplBase, public SampleInterface2Impl {
public:
  SampleModule2Impl(CPPModuleLoader *ldr) : ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE("1.0", "", ModuleImplBase, DECLARE_MODULE_FUNCTION(SampleModule2Impl::calcSum), NULL);

  int calcSum(int num1) {
    SampleModule1Impl *s1 = grt::GRT::get()->get_native_module<SampleModule1Impl>();
    int num2 = s1->getNumber();
    return num1 + num2;
  }
};

#ifdef DEFINE_TEST_MODULES_CODE
int SampleModule1Impl::calculate() {
  SampleModule2Impl *s2 = grt::GRT::get()->get_native_module<SampleModule2Impl>();
  return s2->calcSum(1);
}
#endif

class SampleModule3Impl : public SampleModule2Impl {
public:
  SampleModule3Impl(CPPModuleLoader *ldr) : SampleModule2Impl(ldr) {
  }

  DEFINE_INIT_MODULE("1.0", "Oracle and/or its affiliates", SampleModule2Impl,
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithNumbers),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithObject),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithNumberList),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithTypedObject),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithDict),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithAuthorList));

  int doSomethingWithObject(ObjectRef object) {
    return 0;
  }
  StringRef doSomethingWithNumbers(IntegerRef a, DoubleRef b, int c, double d) {
    return StringRef("");
  }
  int doSomethingWithNumberList(IntegerListRef ilist) {
    return 0;
  }
  StringListRef doSomethingWithTypedObject(std::string s, test_AuthorRef object) {
    return StringListRef();
  }
  int doSomethingWithDict(DictRef d) {
    return 0;
  }
  ListRef<test_Author> doSomethingWithAuthorList(ListRef<test_Author> authors) {
    return ListRef<test_Author>();
  }
};

class BadModuleImpl : public SampleModule2Impl { // this module does not implement everything from the interface
public:
  BadModuleImpl(CPPModuleLoader *ldr) : SampleModule2Impl(ldr) {
  }

  DEFINE_INIT_MODULE("1.0", "", SampleModule2Impl, DECLARE_MODULE_FUNCTION(BadModuleImpl::calcSum), NULL);

  int calcSum() {
    return 0;
  }
};
