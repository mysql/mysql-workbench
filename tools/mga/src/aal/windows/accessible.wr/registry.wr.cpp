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

#include "registry.wr.h"

using namespace aal;

//----------------------------------------------------------------------------------------------------------------------

static Microsoft::Win32::RegistryHive ConverRegHive(RegistryHive hive) {
  switch (hive) {
  case RegistryHive::ClassesRoot:
    return Microsoft::Win32::RegistryHive::ClassesRoot;
  case RegistryHive::CurrentConfig:
    return Microsoft::Win32::RegistryHive::CurrentConfig;
  case RegistryHive::CurrentUser:
    return Microsoft::Win32::RegistryHive::CurrentUser;
  case RegistryHive::LocalMachine:
    return Microsoft::Win32::RegistryHive::LocalMachine;
  case RegistryHive::Users:
    return Microsoft::Win32::RegistryHive::Users;
  default:
    break;
  }
  return Microsoft::Win32::RegistryHive::ClassesRoot;
}

//----------------------------------------------------------------------------------------------------------------------

static Microsoft::Win32::RegistryValueKind ConverValueType(RegistryValueKind kind) {
  switch (kind) {
  case RegistryValueKind::None:
    return Microsoft::Win32::RegistryValueKind::None;
  case RegistryValueKind::Unknown:
    return Microsoft::Win32::RegistryValueKind::Unknown;
  case RegistryValueKind::Binary:
    return Microsoft::Win32::RegistryValueKind::Binary;
  case RegistryValueKind::String:
    return Microsoft::Win32::RegistryValueKind::String;
  case RegistryValueKind::ExpandString:
    return Microsoft::Win32::RegistryValueKind::ExpandString;
  case RegistryValueKind::MultiString:
    return Microsoft::Win32::RegistryValueKind::MultiString;
  case RegistryValueKind::DWord:
    return Microsoft::Win32::RegistryValueKind::DWord;
  case RegistryValueKind::QWord:
    return Microsoft::Win32::RegistryValueKind::QWord;
  default:
    break;
  }
  return Microsoft::Win32::RegistryValueKind::String;
}

//----------------------------------------------------------------------------------------------------------------------

static Microsoft::Win32::RegistryView ConverViewType(RegistryView view) {
  switch (view) {
  case RegistryView::Default:
    return Microsoft::Win32::RegistryView::Default;
  case RegistryView::View32:
    return Microsoft::Win32::RegistryView::Registry32;
  case RegistryView::View64:
    return Microsoft::Win32::RegistryView::Registry64;
  default:
    break;
  }
  return Microsoft::Win32::RegistryView::Default;
}


//----------------------------------------------------------------------------------------------------------------------

Registry::Registry(){
  _registryHive = RegistryHive::RootBase;
}

//----------------------------------------------------------------------------------------------------------------------

Registry::~Registry() {
}

//----------------------------------------------------------------------------------------------------------------------

Registry::Ref Registry::getClassesRoot() {
  auto classesRoot = Ref(new Registry());
  classesRoot->_registryHive = RegistryHive::ClassesRoot;
  return classesRoot;
}

//----------------------------------------------------------------------------------------------------------------------

Registry::Ref Registry::getCurrentUser() {
  auto currentUser = Ref(new Registry());
  currentUser->_registryHive = RegistryHive::CurrentUser;
  return currentUser;
}

//----------------------------------------------------------------------------------------------------------------------

Registry::Ref Registry::getLocalMachine() {
  auto localMachine = Ref(new Registry());
  localMachine->_registryHive = RegistryHive::LocalMachine;
  return localMachine;
}

//----------------------------------------------------------------------------------------------------------------------

Registry::Ref Registry::getUsers() {
  auto users = Ref(new Registry());
  users->_registryHive = RegistryHive::Users;
  return users;
}

//----------------------------------------------------------------------------------------------------------------------

Registry::Ref Registry::getCurrentConfig() {
  auto currentConfig = Ref(new Registry());
  currentConfig->_registryHive = RegistryHive::CurrentConfig;
  return currentConfig;
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::createKey(std::string keyName, std::string valueName, std::string value,
  RegistryValueKind type, RegistryView view) {
  return RegistryNet::Instance->CreateKey(ConverRegHive(_registryHive), gcnew System::String(keyName.c_str()),
    gcnew System::String(valueName.c_str()), gcnew System::String(value.c_str()), ConverValueType(type),
    ConverViewType(view));
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::deleteKey(std::string keyName, bool deleteSubTree, RegistryView view) {
  return RegistryNet::Instance->DeleteKey(ConverRegHive(_registryHive), gcnew System::String(keyName.c_str()),
    deleteSubTree, ConverViewType(view));
}

//----------------------------------------------------------------------------------------------------------------------

void Registry::setValue(std::string keyName, std::string valueName, std::string value,
  RegistryValueKind type, RegistryView view) {
  RegistryNet::Instance->SetValue(ConverRegHive(_registryHive), gcnew System::String(keyName.c_str()),
    gcnew System::String(valueName.c_str()), gcnew System::String(value.c_str()), ConverValueType(type),
    ConverViewType(view));
}

//----------------------------------------------------------------------------------------------------------------------

std::string Registry::getValue(std::string keyName, std::string valueName, RegistryView view) {
  System::Object^ value = RegistryNet::Instance->GetValue(ConverRegHive(_registryHive), 
    gcnew System::String(keyName.c_str()), gcnew System::String(valueName.c_str()), ConverViewType(view));
  array<unsigned char> ^ chars = System::Text::Encoding::UTF8->GetBytes(value->ToString());
  if (chars == nullptr || chars->Length == 0)
    return "";
  pin_ptr<unsigned char> char_ptr = &chars[0];
  std::string result((char *)char_ptr);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::hasKey(std::string keyName, RegistryView view) {
  return RegistryNet::Instance->HasKey(ConverRegHive(_registryHive), gcnew System::String(keyName.c_str()), 
    ConverViewType(view));
}


bool Registry::hasValue(std::string keyName, std::string valueName, bool checkValueType, RegistryValueKind type, 
  RegistryView view) {
  return RegistryNet::Instance->HasValue(ConverRegHive(_registryHive), gcnew System::String(keyName.c_str()),
    gcnew System::String(valueName.c_str()), checkValueType, ConverValueType(type), ConverViewType(view));
}

//----------------------------------------------------------------------------------------------------------------------

bool Registry::deleteValue(std::string keyName, std::string valueName, RegistryView view) {
  return RegistryNet::Instance->DeleteValue(ConverRegHive(_registryHive), gcnew System::String(keyName.c_str()), 
    gcnew System::String(valueName.c_str()), ConverViewType(view));
}

//----------------------------------------------------------------------------------------------------------------------
