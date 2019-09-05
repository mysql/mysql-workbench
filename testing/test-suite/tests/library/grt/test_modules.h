/*
 * Copyright (c) 2005, 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "grt.h"
#include "grtpp_module_cpp.h"
#include "structs.test.h"

struct SampleInterface1Impl : protected grt::InterfaceImplBase {
public:
  DECLARE_REGISTER_INTERFACE(SampleInterface1Impl, DECLARE_INTERFACE_FUNCTION(SampleInterface1Impl::getNumber),
                             DECLARE_INTERFACE_FUNCTION(SampleInterface1Impl::calculate));

  virtual ~SampleInterface1Impl() {
  }

  virtual int getNumber() = 0;
  virtual int calculate() = 0;
};

struct SampleInterface2Impl : protected grt::InterfaceImplBase {
public:
  DECLARE_REGISTER_INTERFACE(SampleInterface2Impl, DECLARE_INTERFACE_FUNCTION(SampleInterface2Impl::calcSum));

  virtual ~SampleInterface2Impl() {
  }

  virtual int calcSum(int num1) = 0;
};

class SampleModule1Impl : public grt::ModuleImplBase, public SampleInterface1Impl {
public:
  SampleModule1Impl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }
  virtual ~SampleModule1Impl() {
  }

  DEFINE_INIT_MODULE("1.0", "", grt::ModuleImplBase, DECLARE_MODULE_FUNCTION(SampleModule1Impl::getNumber),
                     DECLARE_MODULE_FUNCTION(SampleModule1Impl::calculate));

  virtual int getNumber() override {
    return 42;
  }

  virtual int calculate() override;
};

class SampleModule2Impl : public grt::ModuleImplBase, public SampleInterface2Impl {
public:
  SampleModule2Impl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE("1.0", "", grt::ModuleImplBase, DECLARE_MODULE_FUNCTION(SampleModule2Impl::calcSum), NULL);

  virtual int calcSum(int num1) override {
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
  SampleModule3Impl(grt::CPPModuleLoader *ldr) : SampleModule2Impl(ldr) {
  }

  DEFINE_INIT_MODULE("1.0", "Oracle and/or its affiliates", SampleModule2Impl,
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithNumbers),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithObject),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithNumberList),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithTypedObject),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithDict),
                     DECLARE_MODULE_FUNCTION(SampleModule3Impl::doSomethingWithAuthorList));

  int doSomethingWithObject(grt::ObjectRef object) {
    return 0;
  }
  grt::StringRef doSomethingWithNumbers(grt::IntegerRef a, grt::DoubleRef b, int c, double d) {
    return grt::StringRef("");
  }
  int doSomethingWithNumberList(grt::IntegerListRef ilist) {
    return 0;
  }
  grt::StringListRef doSomethingWithTypedObject(std::string s, test_AuthorRef object) {
    return grt::StringListRef();
  }
  int doSomethingWithDict(grt::DictRef d) {
    return 0;
  }
  grt::ListRef<test_Author> doSomethingWithAuthorList(grt::ListRef<test_Author> authors) {
    return grt::ListRef<test_Author>();
  }
};

class BadModuleImpl : public SampleModule2Impl { // this module does not implement everything from the interface
public:
  BadModuleImpl(grt::CPPModuleLoader *ldr) : SampleModule2Impl(ldr) {
  }

  DEFINE_INIT_MODULE("1.0", "", SampleModule2Impl, DECLARE_MODULE_FUNCTION(BadModuleImpl::calcSum), NULL);

  int calcSum() {
    return 0;
  }
};
