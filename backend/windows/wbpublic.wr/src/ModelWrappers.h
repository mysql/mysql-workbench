/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _TREE_MODEL_WRAPPER_H_
#define _TREE_MODEL_WRAPPER_H_

#pragma make_public(::bec::ListModel)

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
    public ref class NodeId
    {
    protected:
      ::bec::NodeId *inner;

    public:
      NodeId(const ::bec::NodeId *inn);
      NodeId();
      NodeId(int index);
      NodeId(String ^str);
      ~NodeId();

      ::bec::NodeId *get_unmanaged_object();
      bool operator == (NodeId^ node);
      bool equals(NodeId^ node);
      int operator[] (int i);
      int get_by_index (int i);
      int depth();
      int end();
      bool previous();
      bool next();
      NodeId^ append(int i);
      bool is_valid();
      String^ repr();
    };


    //----------------------------------------------------------------------------------------------

    /**
     * Helper methods.
     */
    inline NodeId^ nativeToManaged(const bec::NodeId &input)
    {
      return gcnew NodeId(&input);
    }

    inline List<NodeId^>^ nativeToManaged(const std::vector<bec::NodeId> &input)
    {
      //return CppVectorToObjectList<bec::NodeId, NodeId>(input);
      typedef const std::vector<bec::NodeId> SourceContainerType;
      typedef List<NodeId^> TargetContainerType;

      TargetContainerType^ result= gcnew TargetContainerType(static_cast<int>(input.size()));
      SourceContainerType::const_iterator e= input.end();

      for (SourceContainerType::const_iterator i= input.begin(); i != e; i++)
        result->Add(gcnew NodeId(&(*i)));

      return result;
    }

    //----------------------------------------------------------------------------------------------

    /**
     * Wraps a native ListModel to make it available to managed code. This wrapper also
     * takes care for callbacks from unmanaged to managed code.
     */
    typedef DelegateSlot2<void, void, bec::NodeId, NodeId^, int, int> TreeRefreshSlot;

    public ref class ListModel
    {
    protected:
      ::bec::ListModel *inner;
      List<TreeRefreshSlot^> tree_refresh_handlers;
      std::vector<boost::signals2::connection>* native_connections;

    public:
      ListModel(::bec::ListModel *inn);
      ~ListModel() ;

      bool is_valid();
      bool equals(ListModel^ other);
      virtual int count();
      virtual NodeId^ get_node(int index);
      virtual bool get_field(NodeId^ node, int column, [Out] String^ %value);
      virtual bool get_field(NodeId^ node, int column, [Out] int %value);
      virtual bool get_field(NodeId^ node, int column, [Out] double %value);
      virtual String^ get_field_description(NodeId^ node, int column);
      virtual IconId get_field_icon(NodeId^ node, int column, IconSize size);
      virtual GrtValue^ get_grt_value(NodeId^ node, int column);
      virtual void refresh();
      virtual void reset();
      virtual GrtValueType^ get_field_type(NodeId^ node, int column);
      virtual bool set_field(NodeId^ node, int column, String^ value);
      virtual bool set_field(NodeId^ node, int column, double value);
      virtual bool set_field(NodeId^ node, int column, int value);
      virtual bool set_convert_field(NodeId^ node, int column, String^ value);
      virtual bool activate_node(NodeId^ node);
      std::vector<bec::NodeId> convert_node_list(List<NodeId^> ^nodes);
      virtual List<MySQL::Base::MenuItem^> ^get_popup_items_for_nodes(List<NodeId^> ^nodes);
      virtual bool activate_popup_item_for_nodes(String ^name, List<NodeId^> ^nodes);
      virtual bool delete_node(NodeId^ node);
      void reorder(NodeId^ node, int index);
      void reorder_up(NodeId^ node);
      void reorder_down(NodeId^ node);
      bool is_editable(NodeId^ node);
      void add_tree_refresh_handler(TreeRefreshSlot::ManagedDelegate^ slot);
      void remove_tree_refresh_handler(TreeRefreshSlot::ManagedDelegate^ slot);
    };

    //----------------------------------------------------------------------------------------------

    /**
     * Wraps a native TreeModel to make it available to managed code.
     */
    public ref class TreeModel : public ListModel
    {
    public:
      TreeModel(::bec::TreeModel *inn);

      ::bec::TreeModel *get_unmanaged_object();
      virtual NodeId^ get_root();
      virtual int get_node_depth(NodeId^ node);
      virtual NodeId^ get_parent(NodeId^ node);
      virtual int count_children(NodeId^ parent);
      virtual NodeId^ get_child(NodeId^ parent, int index);
      virtual bool expand_node(NodeId^ node);
      virtual void collapse_node(NodeId^ node);
      virtual bool is_expandable(NodeId^ node);
    };

    //----------------------------------------------------------------------------------------------

    /**
     * Wraps a native GridModel to make it available to managed code.
     */
    public ref class GridModel : public ListModel
    {
    public:
      enum class ColumnType
      {
        StringType = ::bec::GridModel::StringType,
        NumericType = ::bec::GridModel::NumericType,
        FloatType = ::bec::GridModel::FloatType,
        DatetimeType = ::bec::GridModel::DatetimeType,
        BlobType = ::bec::GridModel::BlobType
      };

      GridModel(::bec::GridModel *inn);

      ::bec::GridModel *get_unmanaged_object();
      virtual int get_column_count() ;
      virtual String^ get_column_caption(int column);
      virtual ColumnType get_column_type(int column);
      virtual bool is_readonly();
      virtual String^ readonly_reason();
      virtual bool is_field_null(NodeId^ node, int column);
      virtual bool set_field_null(NodeId^ node, int column);
      virtual bool get_field_repr(NodeId^ node, int column, [Out] String^ %value);
      virtual void set_edited_field(int row_index, int col_index);
      virtual void sort_columns([Out] List<int>^ %indexes, [Out] List<int>^ %orders);
    };

  } // namespace Grt
} // namespace MySQL

#endif // _TREE_MODEL_WRAPPER_H_
