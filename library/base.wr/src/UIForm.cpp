/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/ui_form.h"

#include "ConvUtils.h"
#include "UIForm.h"

using namespace System;
using namespace System::Collections::Generic;

using namespace MySQL::Base;

//----------------- MenuItem -----------------------------------------------------------------------

MenuItem::MenuItem(const ::bec::MenuItem& item)
  : caption(CppStringToNative(item.caption)),
    shortcut(CppStringToNative(item.shortcut)),
    internalName(CppStringToNative(item.internalName)),
    type((MenuItemType)item.type),
    enabled(item.enabled),
    checked(item.checked) {
  subitems = gcnew List<MenuItem ^>();
  for (bec::MenuItemList::const_iterator iterator = item.subitems.begin(); iterator != item.subitems.end(); ++iterator)
    subitems->Add(gcnew MenuItem(*iterator));
}

//--------------------------------------------------------------------------------------------------

String ^ MenuItem::get_caption() {
  return caption;
}

//--------------------------------------------------------------------------------------------------

String ^ MenuItem::get_shortcut() {
  return shortcut;
}

//--------------------------------------------------------------------------------------------------

String ^ MenuItem::getInternalName() {
  return internalName;
}

//--------------------------------------------------------------------------------------------------

MenuItemType MenuItem::get_type() {
  return type;
}

//--------------------------------------------------------------------------------------------------

bool MenuItem::get_checked() {
  return checked;
}

//--------------------------------------------------------------------------------------------------

void MenuItem::set_checked(bool value) {
  checked = value;
}

//--------------------------------------------------------------------------------------------------

bool MenuItem::get_enabled() {
  return enabled;
}

//--------------------------------------------------------------------------------------------------

void MenuItem::set_enabled(bool value) {
  enabled = value;
}

//--------------------------------------------------------------------------------------------------

List<MenuItem ^> ^ MenuItem::get_subitems() {
  return subitems;
}

//----------------- UIForm -------------------------------------------------------------------------

UIForm::UIForm(bec::UIForm* inn) {
  init(inn);
}

//--------------------------------------------------------------------------------------------------

UIForm::UIForm() : inner(NULL) {
}

//--------------------------------------------------------------------------------------------------

UIForm::~UIForm() {
  ReleaseHandle();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a fixed pointer to this object that will not be modified by the GC
 */
System::IntPtr UIForm::GetFixedId() {
  if (!m_gch.IsAllocated)
    m_gch = System::Runtime::InteropServices::GCHandle::Alloc(this);
  return System::Runtime::InteropServices::GCHandle::ToIntPtr(m_gch);
}

//--------------------------------------------------------------------------------------------------

void UIForm::ReleaseHandle() {
  if (m_gch.IsAllocated)
    m_gch.Free();
}

//--------------------------------------------------------------------------------------------------

void UIForm::init(bec::UIForm* inn) {
  if (inner != NULL) {
    // Don't touch inner here. It's already gone at this point.
    ReleaseHandle();
  }

  // Just replace the inner pointer. We are not managing the inner object.
  inner = inn;

  if (inner != NULL) {
    // get a fixed pointer to this object
    System::IntPtr ip = this->GetFixedId();

    // set it as the user data
    inner->set_frontend_data((void*)(intptr_t)ip);
  }
}

//--------------------------------------------------------------------------------------------------

bec::UIForm* UIForm::get_unmanaged_object() {
  return inner;
}

//--------------------------------------------------------------------------------------------------

// Returns the object based on the fixed pointer retrieved by GetFixedId()
UIForm ^ UIForm::GetFromFixedId(System::IntPtr ip) {
  System::Runtime::InteropServices::GCHandle gcHandle = System::Runtime::InteropServices::GCHandle::FromIntPtr(ip);
  return (UIForm ^)gcHandle.Target;
}

//--------------------------------------------------------------------------------------------------

bool UIForm::can_close() {
  return get_unmanaged_object()->can_close();
}

//--------------------------------------------------------------------------------------------------

void UIForm::close() {
  get_unmanaged_object()->close();
}

//--------------------------------------------------------------------------------------------------

System::String ^ UIForm::get_title() {
  return CppStringToNativeRaw(get_unmanaged_object()->get_title());
}

//--------------------------------------------------------------------------------------------------

System::String ^ UIForm::form_id() {
  return CppStringToNativeRaw(get_unmanaged_object()->form_id());
}

//--------------------------------------------------------------------------------------------------
