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

#include "NoteEditorWrapper.h"

using namespace MySQL::Grt;

//--------------------------------------------------------------------------------------------------

NoteEditorWrapper::NoteEditorWrapper(NoteEditorBE *inn) : BaseEditorWrapper(inn) {
}

//--------------------------------------------------------------------------------------------------

NoteEditorWrapper::NoteEditorWrapper(MySQL::Grt::GrtValue ^ arglist)
  : BaseEditorWrapper(new ::NoteEditorBE(
      workbench_model_NoteFigureRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)))) {
}

//--------------------------------------------------------------------------------------------------

NoteEditorWrapper::~NoteEditorWrapper() {
  delete inner; // We created it.
}

//--------------------------------------------------------------------------------------------------

NoteEditorBE *NoteEditorWrapper::get_unmanaged_object() {
  return static_cast<::NoteEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void NoteEditorWrapper::set_text(String ^ text) {
  get_unmanaged_object()->set_text(NativeToCppString(text));
}

//--------------------------------------------------------------------------------------------------

String ^ NoteEditorWrapper::get_text() {
  return CppStringToNative(get_unmanaged_object()->get_text());
}

//--------------------------------------------------------------------------------------------------

void NoteEditorWrapper::set_name(String ^ name) {
  get_unmanaged_object()->set_name(NativeToCppString(name));
}

//--------------------------------------------------------------------------------------------------

String ^ NoteEditorWrapper::get_name() {
  return CppStringToNative(get_unmanaged_object()->get_name());
}

//--------------------------------------------------------------------------------------------------
