/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

// Automatically generated GRT module wrapper. Do not edit.

using namespace grt;

class WbValidationInterfaceWrapper : public grt::ModuleWrapper {
protected:
  friend class grt::GRT;
  WbValidationInterfaceWrapper(grt::Module *module) : grt::ModuleWrapper(module) {
  }

public:
  static const char *static_get_name() {
    return "WbValidationInterface";
  }

  ssize_t validate(const std::string & param0, const ObjectRef& param1) {
    grt::BaseListRef args(grt::AnyType);
    args.ginsert(grt::StringRef(param0));
    args.ginsert(param1);
    grt::ValueRef ret = _module->call_function("validate", args);
    return *grt::IntegerRef::cast_from(ret);
  }
  std::string getValidationDescription(const ObjectRef& param0) {
    grt::BaseListRef args(grt::AnyType);
    args.ginsert(param0);
    grt::ValueRef ret = _module->call_function("getValidationDescription", args);
    return *grt::StringRef::cast_from(ret);
  }
};

