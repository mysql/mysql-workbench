/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "wf_mforms.h"
#include "wf_base.h"

#include "ConvUtils.h"

using namespace System::ComponentModel;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;

void free_wrapper(void *payload) {
  ObjectWrapper *wrapper = reinterpret_cast<ObjectWrapper *>(payload);
  delete wrapper;
}

//----------------- ObjectWrapper ------------------------------------------------------------------

ObjectWrapper::ObjectWrapper(mforms::Object *object) {
  object->set_data(this, free_wrapper);
}

//--------------------------------------------------------------------------------------------------

ObjectWrapper::~ObjectWrapper() {
  IntPtr ^ reference = GetBackendReference();
  if (reference != nullptr) {
    mforms::Object *backend = reinterpret_cast<mforms::Object *>(reference->ToPointer());
    if (backend != NULL)
      backend->set_data(NULL);
  }

  delete reference;

  // Not really needed with managed objects, but this way we can free any used resource
  // quicker than waiting for the garbage collection to kick in (especially important for used
  // unmanaged resources).
  Component ^ value = component; // Assign from gcroot to real var.
  delete value;
}

//--------------------------------------------------------------------------------------------------

/**
 * Internal helper for getting the tag of the native platform control.
 */
IntPtr ^ ObjectWrapper::GetBackendReference() {
  Object ^ tag = nullptr;
  System::Object ^ object = component;
  if (is<Control>(object))
    tag = ((Control ^)object)->Tag;
  else if (is<ToolStripItem>(object))
    tag = ((ToolStripItem ^)object)->Tag;
  else if (is<CommonDialog>(object))
    tag = ((CommonDialog ^)object)->Tag;

  return dynamic_cast<IntPtr ^>(tag);
}

//----------------- ObjectMapper -------------------------------------------------------------------

Component ^ ObjectMapper::GetManagedComponent(mforms::Object *backend) {
  return ObjectWrapper::GetManagedObject<Component>(backend);
}

//--------------------------------------------------------------------------------------------------

mforms::Object *ObjectMapper::GetUnmanagedControl(System::Windows::Forms::Control ^ control) {
  return ObjectWrapper::GetBackend<mforms::Object>(control);
}

//--------------------------------------------------------------------------------------------------

void *ObjectMapper::ManagedToNativeDragData(System::Windows::Forms::IDataObject ^ dataObject, String ^ format) {
  DataWrapper ^ wrapper = dynamic_cast<DataWrapper ^>(dataObject->GetData(format, false));
  if (wrapper == nullptr)
    return NULL;

  return wrapper->GetData();
}

//--------------------------------------------------------------------------------------------------
