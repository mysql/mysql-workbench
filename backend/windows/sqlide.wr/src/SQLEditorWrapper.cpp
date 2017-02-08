/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "GrtTemplates.h"
#include "SQLEditorWrapper.h"

#include "grt/editor_base.h"
#include "objimpl/wrapper/mforms_ObjectReference_impl.h"

#include "mforms/dockingpoint.h"
#include "mforms/view.h"
#include "mforms/code_editor.h"

using namespace MySQL::GUI::Workbench;
using namespace MySQL::Forms;
using namespace MySQL::Grt;

//--------------------------------------------------------------------------------------------------

SqlEditorWrapper::SqlEditorWrapper(IntPtr nref_ptr) : _ref(gcnew ManagedRef<MySQLEditor>(nref_ptr)) {
  _result_docking_point = NULL;
  _managed_result_dock_delegate = nullptr;
}

//--------------------------------------------------------------------------------------------------

SqlEditorWrapper::~SqlEditorWrapper() {
  delete _ref;
  if (_result_docking_point != NULL)
    _result_docking_point->release();
  delete _managed_result_dock_delegate;
}

//--------------------------------------------------------------------------------------------------

void SqlEditorWrapper::set_result_docking_delegate(ManagedDockDelegate ^ theDelegate) {
  if (_result_docking_point != NULL)
    _result_docking_point->release();

  if (_managed_result_dock_delegate != nullptr)
    delete _managed_result_dock_delegate;
  _managed_result_dock_delegate = theDelegate;

  // We don't let the docking point delete our native delegate because it is managed by the
  // managed delegate.
  _result_docking_point = mforms::manage(new mforms::DockingPoint(theDelegate->get_unmanaged_delegate(), false));
  db_query_QueryEditorRef qeditor(db_query_QueryEditorRef::cast_from((*_ref)->grtobj()));
  qeditor->resultDockingPoint(mforms_to_grt(_result_docking_point, "DockingPoint"));
}

//--------------------------------------------------------------------------------------------------

Control ^ SqlEditorWrapper::get_editor_container() {
  return dynamic_cast<Control ^>(ObjectMapper::GetManagedComponent(_ref->get_container()));
}

//--------------------------------------------------------------------------------------------------

Control ^ SqlEditorWrapper::get_editor_control() {
  return dynamic_cast<Control ^>(ObjectMapper::GetManagedComponent(_ref->get_editor_control()));
}

//--------------------------------------------------------------------------------------------------

void SqlEditorWrapper::append_text(String ^ text) {
  std::string rawText = NativeToCppString(text);
  _ref->get_editor_control()->append_text(rawText.c_str(), rawText.length());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorWrapper::set_text(String ^ text) {
  std::string rawText = NativeToCppString(text);
  _ref->get_editor_control()->set_text(rawText.c_str());
}

//--------------------------------------------------------------------------------------------------

SqlEditorWrapper ^ SqlEditorWrapper::get_sql_editor(MySQL::Grt::BaseEditorWrapper ^ wrapper) {
  return Ref2Ptr_<MySQLEditor, SqlEditorWrapper>(wrapper->get_unmanaged_object()->get_sql_editor());
}

//--------------------------------------------------------------------------------------------------

void SqlEditorWrapper::set_language(String ^ language) {
  if (language == "mysql")
    _ref->get_editor_control()->set_language(mforms::LanguageMySQL);
  else
    _ref->get_editor_control()->set_language(mforms::LanguageNone);
}

//--------------------------------------------------------------------------------------------------

void SqlEditorWrapper::focus() {
  _ref->focus();
}

//--------------------------------------------------------------------------------------------------
