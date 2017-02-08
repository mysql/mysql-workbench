/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "StoredNoteEditorWrapper.h"

using namespace MySQL::Grt;

//--------------------------------------------------------------------------------------------------

StoredNoteEditorWrapper::StoredNoteEditorWrapper(StoredNoteEditorBE *inn) : BaseEditorWrapper(inn) {
}

//--------------------------------------------------------------------------------------------------

StoredNoteEditorWrapper::StoredNoteEditorWrapper(MySQL::Grt::GrtValue ^ arglist)
  : BaseEditorWrapper(new StoredNoteEditorBE(
      GrtStoredNoteRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)))) {
  grt::ValueRef value = grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0);
  _is_script = grt::ObjectRef::cast_from(value).is_instance("db.Script");
}

//--------------------------------------------------------------------------------------------------

StoredNoteEditorWrapper::~StoredNoteEditorWrapper() {
  delete inner; // We created it.
}

//--------------------------------------------------------------------------------------------------

StoredNoteEditorBE *StoredNoteEditorWrapper::get_unmanaged_object() {
  return static_cast<StoredNoteEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void StoredNoteEditorWrapper::commit_changes() {
  get_unmanaged_object()->commit_changes();
}

//--------------------------------------------------------------------------------------------------

void StoredNoteEditorWrapper::load_text() {
  get_unmanaged_object()->load_text();
}

//--------------------------------------------------------------------------------------------------

void StoredNoteEditorWrapper::set_name(String ^ name) {
  get_unmanaged_object()->set_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

String ^ StoredNoteEditorWrapper::get_name() {
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------

bool StoredNoteEditorWrapper::is_sql_script() {
  return _is_script;
}

//--------------------------------------------------------------------------------------------------
