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

#pragma once

#include "ConvUtils.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "IconManager.h"

using namespace Runtime::InteropServices; // Needed for the [Out] keyword.

namespace MySQL {
  namespace Grt {

    //----------------------------------------------------------------------------------------------

    /**
     * Wraps a native NodeId to make it available to managed code.
     */
  public
    ref class NodeIdWrapper {
    protected:
      ::bec::NodeId *inner;

    public:
      NodeIdWrapper(const bec::NodeId *inn);
      NodeIdWrapper();
      NodeIdWrapper(int index);
      NodeIdWrapper(String ^ str);
      ~NodeIdWrapper();

      bec::NodeId *get_unmanaged_object();
      bool operator==(NodeIdWrapper ^ node);
      bool equals(NodeIdWrapper ^ node);
      int operator[](int i);
      int get_by_index(int i);
      int depth();
      int end();
      bool previous();
      bool next();
      NodeIdWrapper ^ append(int i);
      bool is_valid();
      String ^ toString();
    };

    //----------------------------------------------------------------------------------------------

    /**
     * Helper methods.
     */
    inline NodeIdWrapper ^ nativeToManaged(const bec::NodeId &input) { return gcnew NodeIdWrapper(&input); }

      inline List<NodeIdWrapper ^> ^
      nativeToManaged(const std::vector<bec::NodeId> &input) {
        typedef const std::vector<bec::NodeId> SourceContainerType;
        typedef List<NodeIdWrapper ^> TargetContainerType;

        TargetContainerType ^ result = gcnew TargetContainerType(static_cast<int>(input.size()));
        SourceContainerType::const_iterator e = input.end();

        for (SourceContainerType::const_iterator i = input.begin(); i != e; i++)
          result->Add(gcnew NodeIdWrapper(&(*i)));

        return result;
      }

      //----------------------------------------------------------------------------------------------

      /**
       * Wraps a native ListModel to make it available to managed code. This wrapper also
       * takes care for callbacks from unmanaged to managed code.
       */
      typedef DelegateSlot2<void, void, bec::NodeId, NodeIdWrapper ^, int, int> TreeRefreshSlot;

  public
    ref class ListModelWrapper {
    protected:
      bec::ListModel *inner;
      List<TreeRefreshSlot ^> tree_refresh_handlers;
      std::vector<boost::signals2::connection> *native_connections;

    public:
      ListModelWrapper(::bec::ListModel *inn);
      ~ListModelWrapper();

      bool is_valid();
      bool equals(ListModelWrapper ^ other);
      virtual int count();
      virtual NodeIdWrapper ^ get_node(int index);
      virtual bool get_field(NodeIdWrapper ^ node, int column, [Out] String ^ % value);
      virtual bool get_field(NodeIdWrapper ^ node, int column, [Out] int % value);
      virtual bool get_field(NodeIdWrapper ^ node, int column, [Out] double % value);
      virtual String ^ get_field_description(NodeIdWrapper ^ node, int column);
      virtual IconId get_field_icon(NodeIdWrapper ^ node, int column, IconSize size);
      virtual GrtValue ^ get_grt_value(NodeIdWrapper ^ node, int column);
      virtual void refresh();
      virtual void reset();
      virtual GrtValueType ^ get_field_type(NodeIdWrapper ^ node, int column);
      virtual bool set_field(NodeIdWrapper ^ node, int column, String ^ value);
      virtual bool set_field(NodeIdWrapper ^ node, int column, double value);
      virtual bool set_field(NodeIdWrapper ^ node, int column, int value);
      virtual bool set_convert_field(NodeIdWrapper ^ node, int column, String ^ value);
      virtual bool activate_node(NodeIdWrapper ^ node);
      std::vector<bec::NodeId> convert_node_list(List<NodeIdWrapper ^> ^ nodes);
      virtual List<MySQL::Base::MenuItem ^> ^ get_popup_items_for_nodes(List<NodeIdWrapper ^> ^ nodes);
      virtual bool activate_popup_item_for_nodes(String ^ name, List<NodeIdWrapper ^> ^ nodes);
      virtual bool delete_node(NodeIdWrapper ^ node);
      void reorder(NodeIdWrapper ^ node, int index);
      void reorder_up(NodeIdWrapper ^ node);
      void reorder_down(NodeIdWrapper ^ node);
      bool is_editable(NodeIdWrapper ^ node);
      void add_tree_refresh_handler(TreeRefreshSlot::ManagedDelegate ^ slot);
      void remove_tree_refresh_handler(TreeRefreshSlot::ManagedDelegate ^ slot);
    };

    //----------------------------------------------------------------------------------------------

    /**
     * Wraps a native TreeModel to make it available to managed code.
     */
  public
    ref class TreeModelWrapper : public ListModelWrapper {
    public:
      TreeModelWrapper(bec::TreeModel *inn);

      bec::TreeModel *get_unmanaged_object();
      virtual NodeIdWrapper ^ get_root();
      virtual int get_node_depth(NodeIdWrapper ^ node);
      virtual NodeIdWrapper ^ get_parent(NodeIdWrapper ^ node);
      virtual int count_children(NodeIdWrapper ^ parent);
      virtual NodeIdWrapper ^ get_child(NodeIdWrapper ^ parent, int index);
      virtual bool expand_node(NodeIdWrapper ^ node);
      virtual void collapse_node(NodeIdWrapper ^ node);
      virtual bool is_expandable(NodeIdWrapper ^ node);
    };

    //----------------------------------------------------------------------------------------------

    /**
     * Wraps a native GridModel to make it available to managed code.
     */
  public
    ref class GridModelWrapper : public ListModelWrapper {
    public:
      enum class ColumnType {
        StringType = bec::GridModel::StringType,
        NumericType = bec::GridModel::NumericType,
        FloatType = bec::GridModel::FloatType,
        DatetimeType = bec::GridModel::DatetimeType,
        BlobType = bec::GridModel::BlobType
      };

      GridModelWrapper(bec::GridModel *inn);

      bec::GridModel *get_unmanaged_object();
      virtual int get_column_count();
      virtual String ^ get_column_caption(int column);
      virtual ColumnType get_column_type(int column);
      virtual bool is_readonly();
      virtual String ^ readonly_reason();
      virtual bool is_field_null(NodeIdWrapper ^ node, int column);
      virtual bool set_field_null(NodeIdWrapper ^ node, int column);
      virtual bool get_field_repr(NodeIdWrapper ^ node, int column, [Out] String ^ % value);
      virtual void set_edited_field(int row_index, int col_index);
      virtual void sort_columns([Out] List<int> ^ % indexes, [Out] List<int> ^ % orders);
    };

  } // namespace Grt
} // namespace MySQL
