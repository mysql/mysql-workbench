/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "jsexport.h"
#include "aal/windows/accessible.wr/registry.wr.h"

namespace mga {
  class ScriptingContext;

  class Registry;
  using RegistryRef = std::unique_ptr<Registry>;
  using RegistryList = std::vector<Registry>;

  class Registry : public JSExport {
    
  public:
    Registry();
    virtual ~Registry();

    RegistryRef classesRoot();
    RegistryRef currentUser();
    RegistryRef localMachine();
    RegistryRef users();
    RegistryRef currentConfig();

    bool createKey(std::string keyName, std::string valueName, std::string value,
      aal::RegistryValueKind type, aal::RegistryView view);
    bool hasKey(std::string keyName, aal::RegistryView view);
    bool hasValue(std::string keyName, std::string valueName, bool checkValueType, aal::RegistryValueKind type,
      aal::RegistryView view);
    void setValue(std::string keyName, std::string valueName, std::string value,
      aal::RegistryValueKind type, aal::RegistryView view);
    std::string getValue(std::string keyName, std::string valueName, aal::RegistryView view);
    bool deleteKey(std::string keyName, bool deleteSubTree, aal::RegistryView view);
    bool deleteValue(std::string keyName, std::string valueName, aal::RegistryView view);

    static Registry* validate(JSExport *element);
    static void activate(ScriptingContext &context, JSObject &exports);
    static bool _registered;
  private:

    aal::Registry::Ref _managedObject;
    
   /* static JSVariant getter(ScriptingContext *context, JSExport *element, std::string const& name);
    static void setter(ScriptingContext *context, JSExport *element, std::string const& name, JSVariant value);

    static void defineRegistry(ScriptingContext &context, JSObject &module);*/
  };
}
