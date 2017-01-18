/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_native.h"

using namespace System::Windows::Forms;

using namespace MySQL::Forms;

//--------------------------------------------------------------------------------------------------

NativeWrapper::NativeWrapper(mforms::NativeContainer *native) : ViewWrapper(native) {
}

//--------------------------------------------------------------------------------------------------

mforms::NativeContainer *NativeWrapper::from_control(System::Windows::Forms::Control ^ control) {
  mforms::NativeContainer *backend = new mforms::NativeContainer();

  NativeWrapper *wrapper = new NativeWrapper(backend);
  NativeWrapper::ConnectParts(backend, wrapper, control);

  return backend;
}

//--------------------------------------------------------------------------------------------------

mforms::View *MySQL::Forms::Native::wrapper_for_control(System::Windows::Forms::Control ^ control) {
  return NativeWrapper::from_control(control);
}

//--------------------------------------------------------------------------------------------------
