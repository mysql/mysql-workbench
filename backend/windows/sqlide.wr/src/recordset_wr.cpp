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
#include "ActionList.h"
#include "GrtThreadedTaskWrapper.h"

#include "recordset_wr.h"

#include "mforms/menubar.h"
#include "mforms/toolbar.h"

using namespace MySQL::Forms;
using namespace MySQL::Grt;
using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

RecordsetWrapper::RecordsetWrapper(Ref ref)
  : VarGridModelWrapper(Ref_N2M<::VarGridModel>(&ref)),
    _ref(ref),
    task(gcnew GrtThreadedTaskWrapper(_ref->task.get())),
    action_list(gcnew ActionList(&_ref->action_list())) {
}

//--------------------------------------------------------------------------------------------------

RecordsetWrapper::RecordsetWrapper(IntPtr nref_ptr)
  : _ref(gcnew ManagedRef<::Recordset>(nref_ptr)),
    VarGridModelWrapper(Ref_N2M<::VarGridModel>(&_ref)),
    task(gcnew GrtThreadedTaskWrapper(_ref->task.get())),
    action_list(gcnew ActionList(&_ref->action_list())) {
}

//--------------------------------------------------------------------------------------------------

RecordsetWrapper::~RecordsetWrapper() {
  if (!(void *)~_ref)
    return;
  delete action_list;
  delete task;
  delete _ref;
  delete _flush_ui_changes;
  _flush_ui_changes = nullptr;
}

//--------------------------------------------------------------------------------------------------

void RecordsetWrapper::register_edit_actions() {
  mforms::ToolBarItem *item;
  item = _ref->get_toolbar()->find_item("record_del");
  if (item) {
    item->signal_activated()->connect(boost::bind(&::ActionList::trigger_action, &_ref->action_list(), "record_del"));
    _ref->get_toolbar()
      ->find_item("record_add")
      ->signal_activated()
      ->connect(boost::bind(&::ActionList::trigger_action, &_ref->action_list(), "record_add"));
    _ref->get_toolbar()
      ->find_item("record_edit")
      ->signal_activated()
      ->connect(boost::bind(&::ActionList::trigger_action, &_ref->action_list(), "record_edit"));
  }
  item = _ref->get_toolbar()->find_item("record_wrap_vertical");
  if (item)
    item->signal_activated()->connect(
      boost::bind(&::ActionList::trigger_action, &_ref->action_list(), "record_wrap_vertical"));
}

//--------------------------------------------------------------------------------------------------

void RecordsetWrapper::pending_changes(int % upd_count, int % ins_count, int % del_count) {
  int upd_count_, ins_count_, del_count_;
  _ref->pending_changes(upd_count_, ins_count_, del_count_);
  upd_count = upd_count_;
  ins_count = ins_count_;
  del_count = del_count_;
}

//--------------------------------------------------------------------------------------------------

void RecordsetWrapper::set_flush_ui_changes_cb(DelegateSlot0<void, void>::ManagedDelegate ^ apply) {
  _flush_ui_changes = gcnew DelegateSlot0<void, void>(apply);
  _ref->flush_ui_changes_cb = _flush_ui_changes->get_slot();
}

//--------------------------------------------------------------------------------------------------

void RecordsetWrapper::copy_rows_to_clipboard(List<int> ^ indexes) {
  std::vector<int> row_indexes = IntListToCppVector(indexes);
  _ref->copy_rows_to_clipboard(row_indexes);
}

//--------------------------------------------------------------------------------------------------

ContextMenuStrip ^ RecordsetWrapper::get_context_menu(List<int> ^ indexes, int clicked_column) {
  std::vector<int> row_indexes = IntListToCppVector(indexes);
  _ref->update_selection_for_menu(row_indexes, clicked_column);
  return dynamic_cast<ContextMenuStrip ^>(ObjectMapper::GetManagedComponent(_ref->get_context_menu()));
}

//--------------------------------------------------------------------------------------------------

bool RecordsetWrapper::delete_nodes(List<NodeIdWrapper ^> ^ nodes) {
  std::vector<bec::NodeId> nodes_ = convert_node_list(nodes);
  return _ref->delete_nodes(nodes_);
}

//--------------------------------------------------------------------------------------------------

MySQL::Base::IRecordsetView ^ RecordsetWrapper::wrap_and_create_recordset_view(IntPtr rset) {
  return create_recordset_for_wrapper(
    Ref2Ptr_<::Recordset, RecordsetWrapper>(*(std::shared_ptr<Recordset> *)rset.ToPointer()));
}

void RecordsetWrapper::init_mforms(CreateRecordsetViewForWrapper ^ deleg) {
  create_recordset_for_wrapper = deleg;
  MySQL::Forms::GridViewHelper::init(
    gcnew MySQL::Forms::CreateGridViewDelegate(RecordsetWrapper::wrap_and_create_recordset_view));
}
