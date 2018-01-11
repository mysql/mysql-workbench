/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

using namespace System;
using namespace System::Collections::Generic;

#include "ConvUtils.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "IconManager.h"
#include "GrtWrapper.h"
#include "ModelWrappers.h"
#include "var_grid_model_wr.h"

using namespace MySQL::Grt;

//--------------------------------------------------------------------------------------------------

VarGridModelWrapper::VarGridModelWrapper(Ref ref)
  : GridModelWrapper((&ref).get()), //! temporary solution, update base classes to accept Ref in constructor
    _ref(ref) {
}

//--------------------------------------------------------------------------------------------------

VarGridModelWrapper::VarGridModelWrapper(IntPtr nref_ptr)
  : GridModelWrapper(((::VarGridModel::Ref*)(void*)(nref_ptr))
                       ->get()), //! temporary solution, update base classes to accept Ref in constructor
    _ref(gcnew ManagedRef<::VarGridModel>(nref_ptr)) {
}

//--------------------------------------------------------------------------------------------------

VarGridModelWrapper::~VarGridModelWrapper() {
  if (!(void*)~_ref)
    return;
  delete _refresh_ui_cb;
  delete _ref;
}

//--------------------------------------------------------------------------------------------------

void VarGridModelWrapper::refresh_ui_cb(DelegateSlot0<void, void>::ManagedDelegate ^ cb) {
  if (cb != nullptr) {
    _refresh_ui_cb = gcnew DelegateSlot0<void, void>(cb);
    _refresh_ui_connection =
      new boost::signals2::connection(_ref->refresh_ui_signal.connect(_refresh_ui_cb->get_slot()));
  } else {
    if (_refresh_ui_connection != NULL) {
      _refresh_ui_connection->disconnect();
      delete _refresh_ui_connection;
    }
    if (_refresh_ui_cb != nullptr)
      delete _refresh_ui_cb;
  }
}

//--------------------------------------------------------------------------------------------------

void VarGridModelWrapper::set_update_selection_delegate(DelegateSlot0<void, void>::ManagedDelegate ^ selection) {
  if (selection != nullptr) {
    _update_selection = gcnew DelegateSlot0<void, void>(selection);
    _ref->update_edited_field = _update_selection->get_slot();
  } else {
    _update_selection = nullptr;
    _ref->update_edited_field = std::function<void()>();
  }
}

//--------------------------------------------------------------------------------------------------

void VarGridModelWrapper::set_rows_changed(DelegateSlot0<void, void>::ManagedDelegate ^ update) {
  if (update != nullptr) {
    _rows_changed = gcnew DelegateSlot0<void, void>(update);
    _ref->rows_changed = _rows_changed->get_slot();
  } else {
    _rows_changed = nullptr;
    _ref->rows_changed = std::function<void()>();
  }
}

//--------------------------------------------------------------------------------------------------

int VarGridModelWrapper::edited_field_row() {
  return (int)_ref->edited_field_row();
}

//--------------------------------------------------------------------------------------------------

int VarGridModelWrapper::edited_field_column() {
  return (int)_ref->edited_field_column();
}

//--------------------------------------------------------------------------------------------------
