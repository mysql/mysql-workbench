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

#include "ConvUtils.h"
#include "GrtWrapper.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "GrtManager.h"

#include "ActionList.h"
#include "GrtThreadedTaskWrapper.h"
#include "TableEditorWrapper.h"
#include "mforms/view.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

      TableColumnsListWrapper::TableColumnsListWrapper(bec::TableColumnsListBE *inn) : ListModelWrapper(inn) {
      }

      void TableColumnsListWrapper::reorder_many(List<int> ^ rows, int nindex) {
        std::vector<size_t> nlist;

        for (int i = 0; i < rows->Count; i++)
          nlist.push_back(rows[i]);

        get_unmanaged_object()->reorder_many(nlist, nindex);
      }

      IndexColumnsListWrapper::IndexColumnsListWrapper(IndexListWrapper ^ owner)
        : ListModelWrapper(new bec::IndexColumnsListBE(owner->get_unmanaged_object())) {
      }

      IndexColumnsListWrapper::IndexColumnsListWrapper(bec::IndexColumnsListBE *inn) : ListModelWrapper(inn) {
      }

      void IndexColumnsListWrapper::set_column_enabled(NodeIdWrapper ^ node, bool flag) {
        get_unmanaged_object()->set_column_enabled(*node->get_unmanaged_object(), flag);
      }

      bool IndexColumnsListWrapper::get_column_enabled(NodeIdWrapper ^ node) {
        return get_unmanaged_object()->get_column_enabled(*node->get_unmanaged_object());
      }

      int IndexColumnsListWrapper::get_max_order_index() {
        return (int)get_unmanaged_object()->get_max_order_index();
      }

      IndexListWrapper::IndexListWrapper(TableEditorWrapper ^ owner)
        : ListModelWrapper(new bec::IndexListBE(owner->get_unmanaged_object())) {
      }

      IndexListWrapper::IndexListWrapper(bec::IndexListBE *inn) : ListModelWrapper(inn) {
      }

      IndexColumnsListWrapper ^ IndexListWrapper::get_columns() {
        return gcnew IndexColumnsListWrapper(get_unmanaged_object()->get_columns());
      }

      void IndexListWrapper::select_index(NodeIdWrapper ^ node) {
        get_unmanaged_object()->select_index(*node->get_unmanaged_object());
      }

      FKConstraintColumnsListWrapper::FKConstraintColumnsListWrapper(FKConstraintListWrapper ^ owner)
        : ListModelWrapper(new bec::FKConstraintColumnsListBE(owner->get_unmanaged_object())) {
      }

      FKConstraintColumnsListWrapper::FKConstraintColumnsListWrapper(bec::FKConstraintColumnsListBE *inn)
        : ListModelWrapper(inn) {
      }

      List<String ^> ^ FKConstraintColumnsListWrapper::get_ref_columns_list(NodeIdWrapper ^ node, bool filtered) {
        return CppStringListToNative(static_cast<bec::FKConstraintColumnsListBE *>(inner)->get_ref_columns_list(
          *node->get_unmanaged_object(), filtered));
      }

      bool FKConstraintColumnsListWrapper::set_column_is_fk(NodeIdWrapper ^ node, bool flag) {
        return get_unmanaged_object()->set_column_is_fk(*node->get_unmanaged_object(), flag);
      }

      bool FKConstraintColumnsListWrapper::get_column_is_fk(NodeIdWrapper ^ node) {
        return get_unmanaged_object()->get_column_is_fk(*node->get_unmanaged_object());
      }

      FKConstraintListWrapper::FKConstraintListWrapper(TableEditorWrapper ^ owner)
        : ListModelWrapper(new bec::FKConstraintListBE(owner->get_unmanaged_object())) {
      }

      FKConstraintListWrapper::FKConstraintListWrapper(bec::FKConstraintListBE *inn) : ListModelWrapper(inn) {
      }

      /*
      NodeId^ FKConstraintListWrapper::add_column(String^ column_name)
      { return gcnew NodeId(&get_unmanaged_object()->add_column(NativeToCppString(column_name))); }*/

      void FKConstraintListWrapper::select_fk(NodeIdWrapper ^ node) {
        get_unmanaged_object()->select_fk(*node->get_unmanaged_object());
      }

      FKConstraintColumnsListWrapper ^ FKConstraintListWrapper::get_columns() {
        return gcnew FKConstraintColumnsListWrapper(get_unmanaged_object()->get_columns());
      }

      IndexListWrapper ^ TableEditorWrapper::get_indexes() {
        return gcnew IndexListWrapper(get_unmanaged_object()->get_indexes());
      }

      FKConstraintListWrapper ^ TableEditorWrapper::get_fks() {
        return gcnew FKConstraintListWrapper(get_unmanaged_object()->get_fks());
      }

      Control ^ TableEditorWrapper::get_inserts_panel() {
        mforms::View *view = get_unmanaged_object()->get_inserts_panel();

        return dynamic_cast<Control ^>(MySQL::Forms::ObjectMapper::GetManagedComponent(view));
      }

      // table options
      //...

      // column editing
      NodeIdWrapper ^ TableEditorWrapper::add_column(String ^ name) {
        return gcnew NodeIdWrapper(&get_unmanaged_object()->add_column(NativeToCppString(name)));
      }

      void TableEditorWrapper::remove_column(NodeIdWrapper ^ node) {
        get_unmanaged_object()->remove_column(*node->get_unmanaged_object());
      }

      // db_Column get_column_with_name(const std::string &name);

      // fk editing
      NodeIdWrapper ^ TableEditorWrapper::add_fk(String ^ name) {
        return gcnew NodeIdWrapper(&get_unmanaged_object()->add_fk(NativeToCppString(name)));
      }

      void TableEditorWrapper::remove_fk(NodeIdWrapper ^ node) {
        get_unmanaged_object()->remove_fk(*node->get_unmanaged_object());
      }

      NodeIdWrapper ^ TableEditorWrapper::add_fk_with_columns(List<NodeIdWrapper ^> ^ columns) {
        std::vector<bec::NodeId> node_vec = ObjectListToCppVector<NodeIdWrapper, bec::NodeId>(columns);
        return gcnew NodeIdWrapper(&get_unmanaged_object()->add_fk_with_columns(node_vec));
      }

      // index editing
      NodeIdWrapper ^ TableEditorWrapper::add_index(String ^ name) {
        return gcnew NodeIdWrapper(&get_unmanaged_object()->add_index(NativeToCppString(name)));
      }

      void TableEditorWrapper::remove_index(NodeIdWrapper ^ node) {
        get_unmanaged_object()->remove_index(*node->get_unmanaged_object(), false);
      }

      NodeIdWrapper ^ TableEditorWrapper::add_index_with_columns(List<NodeIdWrapper ^> ^ columns) {
        std::vector<bec::NodeId> node_vec = ObjectListToCppVector<NodeIdWrapper, bec::NodeId>(columns);
        return gcnew NodeIdWrapper(&get_unmanaged_object()->add_index_with_columns(node_vec));
      }

      List<String ^> ^ TableEditorWrapper::get_index_types() {
        return CppStringListToNative(static_cast<bec::TableEditorBE *>(inner)->get_index_types());
      }

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL
