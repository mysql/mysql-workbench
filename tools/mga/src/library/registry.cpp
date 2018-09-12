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

#include "registry.h"
#include "scripting-context.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

Registry::Registry() : _managedObject(new aal::Registry()) {
}

//----------------------------------------------------------------------------------------------------------------------

Registry::~Registry() {
}

//----------------------------------------------------------------------------------------------------------------------

RegistryRef Registry::localMachine() {
  auto registry = RegistryRef(new Registry());
  registry->_managedObject = _managedObject->getLocalMachine();
  return RegistryRef(std::move(registry));
}

//----------------------------------------------------------------------------------------------------------------------

RegistryRef Registry::currentUser() {
  auto registry = RegistryRef(new Registry());
  registry->_managedObject = _managedObject->getCurrentUser();
  return RegistryRef(std::move(registry));
}

//----------------------------------------------------------------------------------------------------------------------

RegistryRef Registry::currentConfig() {
  auto registry = RegistryRef(new Registry());
  registry->_managedObject = _managedObject->getCurrentConfig();
  return RegistryRef(std::move(registry));
}

//----------------------------------------------------------------------------------------------------------------------

RegistryRef Registry::users() {
  auto registry = RegistryRef(new Registry());
  registry->_managedObject = _managedObject->getUsers();
  return RegistryRef(std::move(registry));
}
 
//----------------------------------------------------------------------------------------------------------------------

RegistryRef Registry::classesRoot() {
  auto registry = RegistryRef(new Registry());
  registry->_managedObject = _managedObject->getClassesRoot();
  return RegistryRef(std::move(registry));
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::createKey(std::string keyName, std::string valueName, std::string value,
  aal::RegistryValueKind type, aal::RegistryView view) {
  return _managedObject->createKey(keyName, valueName, value, type, view);
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::hasKey(std::string keyName, aal::RegistryView view) {
  return _managedObject->hasKey(keyName, view);
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::hasValue(std::string keyName, std::string valueName, bool checkValueType, aal::RegistryValueKind type,
  aal::RegistryView view) {
  return _managedObject->hasValue(keyName, valueName, checkValueType, type, view);
}

void Registry::setValue(std::string keyName, std::string valueName, std::string value,
  aal::RegistryValueKind type, aal::RegistryView view) {
  _managedObject->setValue(keyName, valueName, value, type, view);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Registry::getValue(std::string keyName, std::string valueName, aal::RegistryView view) {
  return _managedObject->getValue(keyName, valueName, view);
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::deleteKey(std::string keyName, bool deleteSubTree, aal::RegistryView view) {
  return _managedObject->deleteKey(keyName, deleteSubTree, view);
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::deleteValue(std::string keyName, std::string valueName, aal::RegistryView view) {
  return _managedObject->deleteValue(keyName, valueName, view);
}

//----------------------------------------------------------------------------------------------------------------------

Registry* Registry::validate(JSExport *element) {
  Registry *result = dynamic_cast<Registry *>(element);
  if (result == nullptr)
    throw std::runtime_error("Registry is undefined");

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Registry::activate(ScriptingContext &context, JSObject &exports) {
  JSGlobalObject global(&context);
  JSObject mga = global.get("mga");
  mga.defineEnum("RegistryView", [](JSObject &object) {
    object.defineProperty("Default", static_cast<int>(aal::RegistryView::Default));
    object.defineProperty("View32", static_cast<int>(aal::RegistryView::View32));
    object.defineProperty("View64", static_cast<int>(aal::RegistryView::View64));
  });
  mga.defineEnum("RegistryHive", [](JSObject &object) {
    object.defineProperty("RootBase", static_cast<int>(aal::RegistryHive::RootBase));
    object.defineProperty("ClassesRoot", static_cast<int>(aal::RegistryHive::ClassesRoot));
    object.defineProperty("CurrentUser", static_cast<int>(aal::RegistryHive::CurrentUser));
    object.defineProperty("LocalMachine", static_cast<int>(aal::RegistryHive::LocalMachine));
    object.defineProperty("Users", static_cast<int>(aal::RegistryHive::Users));
    object.defineProperty("CurrentConfig", static_cast<int>(aal::RegistryHive::CurrentConfig));
  });
  mga.defineEnum("RegistryValueKind", [](JSObject &object) {
    object.defineProperty("None", static_cast<int>(aal::RegistryValueKind::None));
    object.defineProperty("Unknown", static_cast<int>(aal::RegistryValueKind::Unknown));
    object.defineProperty("String", static_cast<int>(aal::RegistryValueKind::String));
    object.defineProperty("ExpandString", static_cast<int>(aal::RegistryValueKind::ExpandString));
    object.defineProperty("Binary", static_cast<int>(aal::RegistryValueKind::Binary));
    object.defineProperty("DWord", static_cast<int>(aal::RegistryValueKind::DWord));
    object.defineProperty("QWord", static_cast<int>(aal::RegistryValueKind::QWord));
  });

  std::ignore = context;
  exports.defineFunction({ "registryRoot" }, 0, [](JSExport *registry, JSValues &args) {
    // parameters: name
    std::ignore = registry;
    std::ignore = args;
    JSObject result = args.context()->createJsInstance("Registry", { new Registry() });
    args.pushResult(result);
  });
  exports.defineClass( "Registry", "", 1, [](JSObject *instance, JSValues &args) {
    void *value = args.get(0);
    Registry* backend = reinterpret_cast<Registry *>(value);
    instance->setBacking(backend);
  }, [](JSObject &prototype) {
    prototype.defineFunction({ "currentUser" }, 0, [](JSExport *element, JSValues &args) {
      // parameters: name
      std::ignore = args;
      auto me = validate(element);
      RegistryRef local = me->currentUser();
      if (local) {
        Registry* ptr = local.release();
        JSObject result = args.context()->createJsInstance("Registry", { ptr });
        args.pushResult(result);
      }
    });
    prototype.defineFunction({ "classesRoot" }, 0, [](JSExport *element, JSValues &args) {
      // parameters: name
      std::ignore = args;
      auto me = validate(element);
      RegistryRef local = me->classesRoot();
      if (local) {
        Registry* ptr = local.release();
        JSObject result = args.context()->createJsInstance("Registry", { ptr });
        args.pushResult(result);
      }
    });
    prototype.defineFunction({ "currentConfig" }, 0, [](JSExport *element, JSValues &args) {
      // parameters: name
      std::ignore = args;
      auto me = validate(element);
      RegistryRef local = me->currentConfig();
      if (local) {
        Registry* ptr = local.release();
        JSObject result = args.context()->createJsInstance("Registry", { ptr });
        args.pushResult(result);
      }
    });
    prototype.defineFunction({ "localMachine" }, 0, [](JSExport *element, JSValues &args) {
      // parameters: name
      std::ignore = args;
      auto me = validate(element);
      RegistryRef local = me->localMachine();
      if (local) {
        Registry* ptr = local.release();
        JSObject result = args.context()->createJsInstance("Registry", { ptr });
        args.pushResult(result);
      }
    });
    prototype.defineFunction({ "users" }, 0, [](JSExport *element, JSValues &args) {
      // parameters: name
      std::ignore = args;
      auto me = validate(element);
      RegistryRef local = me->users();
      if (local) {
        Registry* ptr = local.release();
        JSObject result = args.context()->createJsInstance("Registry", { ptr });
        args.pushResult(result);
      }
    });
    prototype.defineFunction({ "createKey" }, 5, [](JSExport *object, JSValues &args) {
      // parameters: name
      std::string keyName;
      if (args.is(ValueType::String, 0)) {
        keyName = args.get(0);
      }
      std::string valueName;
      if (args.is(ValueType::String, 1)) {
        valueName = args.get(1);
      }
      std::string value = args.get(2);
      aal::RegistryValueKind kind = aal::RegistryValueKind::String;
      if (args.size() > 3) { // The registry kind is optional.
        int regKind = args.as(ValueType::Int, 3);
        kind = static_cast<aal::RegistryValueKind>(regKind);
      }
      aal::RegistryView view = aal::RegistryView::Default;
      if (args.size() > 4) { // The registry view is optional.
        int regView = args.as(ValueType::Int, 4);
        view = static_cast<aal::RegistryView>(regView);
      }
      auto me = validate(object);
      bool result = me->createKey(keyName, valueName, value, kind, view);
      args.pushResult(result);
    });
    prototype.defineFunction({ "hasKey" }, 2, [](JSExport *object, JSValues &args) {
      // parameters: name
      std::string keyName;
      if (args.is(ValueType::String, 0)) {
        keyName = args.get(0);
      }
      aal::RegistryView view = aal::RegistryView::Default;
      if (args.size() > 1) { // The registry view is optional.
        int regView = args.as(ValueType::Int, 1);
        view = static_cast<aal::RegistryView>(regView);
      }
      auto me = validate(object);
      bool result = me->hasKey(keyName, view);
      args.pushResult(result);
    });
    prototype.defineFunction({ "hasValue" }, 5, [](JSExport *object, JSValues &args) {
      std::string keyName;
      if (args.is(ValueType::String, 0)) {
        keyName = args.get(0);
      }
      std::string valueName;
      if (args.is(ValueType::String, 1)) {
        valueName = args.get(1);
      }
      bool checkType = false;
      if (args.size() > 2) { // The checkValueType is optional.
        checkType = args.as(ValueType::Boolean, 2);
      }
      aal::RegistryValueKind kind = aal::RegistryValueKind::String;
      if (args.size() > 3) { // The registry kind is optional.
        int regKind = args.as(ValueType::Int, 3);
        kind = static_cast<aal::RegistryValueKind>(regKind);
      }
      aal::RegistryView view = aal::RegistryView::Default;
      if (args.size() > 4) { // The registry view is optional.
        int regView = args.as(ValueType::Int, 4);
        view = static_cast<aal::RegistryView>(regView);
      }
      auto me = validate(object);
      bool result = me->hasValue(keyName, valueName, checkType,  kind, view);
      args.pushResult(result);
    });
    prototype.defineFunction({ "deleteKey" }, 3, [](JSExport *object, JSValues &args) {
      // parameters: name
      std::ignore = args;
      std::string keyName;
      if (args.is(ValueType::String, 0)) {
        keyName = args.get(0);
      }
      bool deleteSubTree = false;
      if (args.size() > 1) { // The deleteSubTree is optional.
        deleteSubTree = args.as(ValueType::Boolean, 1);
      }
      aal::RegistryView view = aal::RegistryView::Default;
      if (args.size() > 2) { // The registry view is optional.
        int regView = args.as(ValueType::Int, 2);
        view = static_cast<aal::RegistryView>(regView);
      }
      auto me = validate(object);
      bool result = me->deleteKey(keyName, deleteSubTree, view);
      args.pushResult(result);
    });
    prototype.defineFunction({ "deleteValue" }, 3, [](JSExport *object, JSValues &args) {
      // parameters: name
      std::ignore = args;
      std::string keyName;
      if (args.is(ValueType::String, 0)) {
        keyName = args.get(0);
      }
      std::string valueName;
      if (args.is(ValueType::String, 1)) {
        valueName = args.get(1);
      }
      aal::RegistryView view = aal::RegistryView::Default;
      if (args.size() > 2) { // The registry view is optional.
        int regView = args.as(ValueType::Int, 2);
        view = static_cast<aal::RegistryView>(regView);
      }
      auto me = validate(object);
      bool result = me->deleteValue(keyName, valueName, view);
      args.pushResult(result);
    });
    prototype.defineFunction({ "setValue" }, 5, [](JSExport *object, JSValues &args) {
      // parameters: name
      std::string keyName;
      if (args.is(ValueType::String, 0)) {
        keyName = args.get(0);
      }
      std::string valueName;
      if (args.is(ValueType::String, 1)) {
        valueName = args.get(1);
      }
      std::string value = args.get(2);
      aal::RegistryValueKind kind = aal::RegistryValueKind::String;
      if (args.is(ValueType::Int, 3)) { // The registry kind is optional.
        int regKind = args.as(ValueType::Int, 3);
        kind = static_cast<aal::RegistryValueKind>(regKind);
      }
      aal::RegistryView view = aal::RegistryView::Default;
      if (args.size() > 4) { // The registry view is optional.
        int regView = args.as(ValueType::Int, 4);
        view = static_cast<aal::RegistryView>(regView);
      }
      auto me = validate(object);
      me->setValue(keyName, valueName, value, kind, view);
    });
    prototype.defineFunction({ "getValue" }, 3, [](JSExport *object, JSValues &args) {
      // parameters: name
      std::string keyName;
      if (args.is(ValueType::String, 0)) {
        keyName = args.get(0);
      }
      std::string valueName;
      if (args.is(ValueType::String, 1)) {
        valueName = args.get(1);
      }
      aal::RegistryView view = aal::RegistryView::Default;
      if (args.size() > 2) { // The registry view is optional.
        int regView = args.as(ValueType::Int, 2);
        view = static_cast<aal::RegistryView>(regView);
      }
      auto me = validate(object);
      auto result = me->getValue(keyName, valueName, view);
      args.pushResult(result);
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::_registered = []() {
  ScriptingContext::registerModule("registry", &Registry::activate);
  return true;
}();

//----------------------------------------------------------------------------------------------------------------------

