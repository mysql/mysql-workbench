/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __OVERVIEW_H__
#define __OVERVIEW_H__

namespace MySQL {
  namespace Workbench {

  public
    ref class Overview : public MySQL::Grt::TreeModelWrapper {
    protected:
      ref class OverviewUIForm : public ::MySQL::Base::UIForm {
      public:
        OverviewUIForm(::bec::UIForm *inn) : MySQL::Base::UIForm(inn) {
        }
      };

      OverviewUIForm ^ uiform;

    public:
      enum class Columns {
        Label = ::wb::OverviewBE::Label,
        NodeType = ::wb::OverviewBE::NodeType,
        ChildNodeType = ::wb::OverviewBE::ChildNodeType,
        Expanded = ::wb::OverviewBE::Expanded,
        Height = ::wb::OverviewBE::Height,
        DisplayMode = ::wb::OverviewBE::DisplayMode,

        FirstDetailField = ::wb::OverviewBE::FirstDetailField
      };
      enum class NodeType {
        Root = ::wb::OverviewBE::ORoot,
        Division = ::wb::OverviewBE::ODivision,
        Group = ::wb::OverviewBE::OGroup,
        Section = ::wb::OverviewBE::OSection,
        Item = ::wb::OverviewBE::OItem
      };
      enum class DisplayMode {
        None = ::wb::OverviewBE::MNone,
        LargeIcon = ::wb::OverviewBE::MLargeIcon,
        SmallIcon = ::wb::OverviewBE::MSmallIcon,
        List = ::wb::OverviewBE::MList
      };

      inline ::wb::OverviewBE *get_unmanaged_object() {
        return static_cast<::wb::OverviewBE *>(TreeModelWrapper::get_unmanaged_object());
      }

      Overview(::wb::OverviewBE *inn) : MySQL::Grt::TreeModelWrapper(inn) {
        uiform = gcnew OverviewUIForm(inn);
      }

      ~Overview() {
        uiform->init(
          NULL); // Reset the BE MySQL::Base::UIForm reference. It is a shared one not managed by the overview.
        delete uiform;
      }

      MySQL::Base::UIForm ^ get_uiform() { return uiform; }

        bool matches_handle(System::IntPtr handle) {
        return (dynamic_cast<bec::UIForm *>(get_unmanaged_object()) ==
                reinterpret_cast<bec::UIForm *>(handle.ToPointer()));
      }

      System::String ^
        get_title() {
          std::string title = get_unmanaged_object()->get_title();
          if (title.empty())
            return nullptr;
          return CppStringToNative(title);
        }

        System::String
        ^
        get_node_unique_id(MySQL::Grt::NodeIdWrapper ^ node) {
          std::string id = get_unmanaged_object()->get_node_unique_id(*node->get_unmanaged_object());
          if (id.empty())
            return nullptr;
          return CppStringToNative(id);
        }

        MySQL::Grt::NodeIdWrapper
        ^
        get_focused_child(MySQL::Grt::NodeIdWrapper ^ node) {
          return gcnew MySQL::Grt::NodeIdWrapper(
            &get_unmanaged_object()->get_focused_child(*node->get_unmanaged_object()));
        }

        void select_node(MySQL::Grt::NodeIdWrapper ^ node) {
        get_unmanaged_object()->select_node(*node->get_unmanaged_object());
      }

      System::Collections::Generic::List<int> ^
        get_selected_children(MySQL::Grt::NodeIdWrapper ^ node) {
          System::Collections::Generic::List<int> ^ list = gcnew System::Collections::Generic::List<int>();

          std::list<int> items = get_unmanaged_object()->get_selected_children(*node->get_unmanaged_object());

          for (std::list<int>::iterator i = items.begin(); i != items.end(); ++i)
            list->Add(*i);

          return list;
        }

        bool is_expansion_disabled() {
        return get_unmanaged_object()->is_expansion_disabled();
      }

      int get_default_tab_page_index() {
        return get_unmanaged_object()->get_default_tab_page_index();
      }

      void focus_node(MySQL::Grt::NodeIdWrapper ^ node) {
        get_unmanaged_object()->focus_node(*node->get_unmanaged_object());
      }

      void begin_selection_marking() {
        get_unmanaged_object()->begin_selection_marking();
      }

      void end_selection_marking() {
        get_unmanaged_object()->end_selection_marking();
      }

      bool is_editable(MySQL::Grt::NodeIdWrapper ^ node) {
        return get_unmanaged_object()->is_editable(*node->get_unmanaged_object());
      }

      bool request_add_object(MySQL::Grt::NodeIdWrapper ^ node) {
        return get_unmanaged_object()->request_add_object(*node->get_unmanaged_object());
      }

      bool request_delete_object(MySQL::Grt::NodeIdWrapper ^ node) {
        return get_unmanaged_object()->request_delete_object(*node->get_unmanaged_object());
      }

      bool request_delete_selection() {
        return get_unmanaged_object()->request_delete_selected() != 0;
      }

      void refresh_node(MySQL::Grt::NodeIdWrapper ^ node, bool children) {
        try {
          get_unmanaged_object()->refresh_node(*node->get_unmanaged_object(), children);
        } catch (std::exception &exc) {
          OutputDebugStringA(base::strfmt("Exception during overview refresh: %s\n", exc.what()).c_str());
        }
      }

      System::String ^
        get_field_name(MySQL::Grt::NodeIdWrapper ^ node, int column) {
          std::string name = get_unmanaged_object()->get_field_name(*node->get_unmanaged_object(), column);
          return CppStringToNative(name);
        }

        int get_details_field_count(MySQL::Grt::NodeIdWrapper ^ node) {
        return get_unmanaged_object()->get_details_field_count(*node->get_unmanaged_object());
      }

      MySQL::Grt::NodeIdWrapper ^
        search_child_item_node_matching(MySQL::Grt::NodeIdWrapper ^ node, MySQL::Grt::NodeIdWrapper ^ starting_node,
                                        System::String ^ text) {
          bec::NodeId cnode = node != nullptr ? *node->get_unmanaged_object() : bec::NodeId();
          bec::NodeId cstarting_node =
            starting_node != nullptr ? *starting_node->get_unmanaged_object() : bec::NodeId();

          bec::NodeId result =
            get_unmanaged_object()->search_child_item_node_matching(cnode, cstarting_node, NativeToCppString(text));

          if (result.is_valid())
            return gcnew MySQL::Grt::NodeIdWrapper(&result);
          return nullptr;
        }

        bool can_close() {
        return get_unmanaged_object()->can_close();
      }

      void close() {
        get_unmanaged_object()->close();
      }

      System::Collections::Generic::List<::MySQL::Base::ToolbarItem ^> ^
        get_toolbar_items(MySQL::Grt::NodeIdWrapper ^ node) {
          bec::ToolbarItemList items = get_unmanaged_object()->get_toolbar_items(*node->get_unmanaged_object());
          return MySQL::Grt::CppVectorToObjectList<::bec::ToolbarItem, ::MySQL::Base::ToolbarItem>(items);
        }

        bool activate_toolbar_item(MySQL::Grt::NodeIdWrapper ^ node, System::String ^ name) {
        return get_unmanaged_object()->activate_toolbar_item(*node->get_unmanaged_object(), NativeToCppString(name));
      }
    };

  } // namespace Workbench
} // namespace MySQL

#endif // __OVERVIEW_H__
