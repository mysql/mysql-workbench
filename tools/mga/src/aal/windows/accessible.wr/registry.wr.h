/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "aalcommon.h"

#pragma once 

namespace aal {
  enum class RegistryHive { 
    RootBase = 0,
    ClassesRoot,
    CurrentUser,
    LocalMachine,
    Users,
    CurrentConfig
  };
  enum class RegistryView { 
    Default = 0, 
    View32,
    View64 
  };
  enum class RegistryValueKind {
    None = 0,
    Unknown,
    String,
    ExpandString,
    Binary,
    DWord,
    MultiString,
    QWord
  };

  class ACCESSIBILITY_PUBLIC Registry {
  public:
    typedef std::unique_ptr<Registry> Ref;

    Registry();
    virtual ~Registry();

    Ref getClassesRoot();
    Ref getCurrentUser();
    Ref getLocalMachine();
    Ref getUsers();
    Ref getCurrentConfig();

    void setValue(std::string keyName, std::string valueName, std::string value,
      RegistryValueKind type, RegistryView view = RegistryView::Default);
    std::string getValue(std::string keyName, std::string valueName, RegistryView view = RegistryView::Default);

    bool createKey(std::string keyName, std::string valueName, std::string value,
      RegistryValueKind type, RegistryView view = RegistryView::Default);
    bool deleteKey(std::string keyName, bool deleteSubTree, RegistryView view);
    bool hasKey(std::string keyName, RegistryView view);
    bool hasValue(std::string keyName, std::string valueName, bool checkValueType, RegistryValueKind type,
      RegistryView view = RegistryView::Default);
    bool deleteValue(std::string keyName, std::string valueName, RegistryView view = RegistryView::Default);

  private:
    RegistryHive _registryHive;
  }; 
}
