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

#pragma once

#include "grt/tree_model.h"
#include "grt/grt_manager.h"
#include "base/ui_form.h"

#include "wb_command_ui.h"

#include "grts/structs.workbench.h"

#include "wb_backend_public_interface.h"

#define DEFAULT_SECTION_HEIGHT 120

namespace wb {

  class WBContext;

  class MYSQLWBBACKEND_PUBLIC_FUNC OverviewBE : public bec::TreeModel, public bec::UIForm {
  public:
    enum OverviewColumn {
      Label, // editable
      NodeType,
      ChildNodeType,
      Expanded,
      Height,
      DisplayMode,

      FirstDetailField = 100
    };

    enum OverviewNodeType {
      ORoot,
      ODivision,
      OGroup,
      OSection,
      OItem,

      OSpecial
    };

    enum OverviewDisplayMode { MNone = 0, MLargeIcon = 1, MSmallIcon = 2, MList = 3 };

  public:
    OverviewBE(WBContext *wb);
    virtual ~OverviewBE();

    WBContext *get_wb() {
      return _wb;
    }

    virtual std::string get_title();
    virtual std::string identifier() const = 0;

    virtual bool is_main_form() {
      return true;
    }

    virtual bec::NodeId get_child(const bec::NodeId &parent, size_t index);
    virtual size_t count_children(const bec::NodeId &parent);

    virtual grt::ValueRef get_grt_value(const bec::NodeId &node, ColumnId column);
    virtual bool get_field(const bec::NodeId &node, ColumnId column, std::string &value);
    virtual bool get_field(const bec::NodeId &node, ColumnId column, ssize_t &value);

    int get_details_field_count(const bec::NodeId &node);

    std::string get_field_name(const bec::NodeId &node, ColumnId column);
    virtual std::string get_field_description(const bec::NodeId &node, ColumnId column);
    virtual bec::IconId get_field_icon(const bec::NodeId &node, ColumnId column, bec::IconSize size);

    virtual bool set_field(const bec::NodeId &node, ColumnId column, const std::string &value);

    // only leaf nodes can be selected (for now)
    void begin_selection_marking();
    void end_selection_marking();
    void unselect_all(const bec::NodeId &node);
    void select_node(const bec::NodeId &node);

    boost::signals2::signal<void()> *signal_selection_changed() {
      return &_selection_change_signal;
    }
    grt::ListRef<GrtObject> get_selection();

    std::list<int> get_selected_children(const bec::NodeId &node);

    // only 1 node can be focused in each container node
    void focus_node(const bec::NodeId &node);
    bec::NodeId get_focused_child(const bec::NodeId &node);

    bec::NodeId get_node_child_for_object(const bec::NodeId &node, const grt::ObjectRef &object);

    virtual bool is_expansion_disabled() {
      return false;
    }
    virtual int get_default_tab_page_index() {
      return -1;
    }

    bec::NodeId search_child_item_node_matching(const bec::NodeId &node, const bec::NodeId &starting_node,
                                                const std::string &text);

    virtual bool activate_node(const bec::NodeId &node);
    std::string get_node_unique_id(const bec::NodeId &node);

    virtual bool is_editable(const bec::NodeId &node) const;
    virtual bool is_deletable(const bec::NodeId &node) const;
    virtual bool is_copyable(const bec::NodeId &node) const;
    bool request_add_object(const bec::NodeId &node);
    int request_delete_selected();
    bool request_delete_object(const bec::NodeId &node);

    virtual model_ModelRef get_model() = 0;

    virtual bec::MenuItemList get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes);
    virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<bec::NodeId> &nodes);

    virtual bec::ToolbarItemList get_toolbar_items(const bec::NodeId &node);
    virtual bool activate_toolbar_item(const bec::NodeId &node, const std::string &name);

    // for use by backend
    void send_refresh_node(const bec::NodeId &node);
    void send_refresh_children(const bec::NodeId &node);

    // for use by frontend
    std::function<void()> pre_refresh_groups;
    void refresh();
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
    // This is ok, as Overview contains children which also need to be refreshed.
    virtual void refresh_node(const bec::NodeId &node, bool children) = 0;
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

    virtual std::string get_edit_target_name();
    std::string get_target_name_for_nodes(const std::vector<bec::NodeId> &nodes);

    virtual bool can_cut();
    virtual bool can_copy();
    virtual bool can_paste();
    virtual bool can_delete();

    virtual void cut();
    virtual void copy();
    virtual void paste();
    virtual void delete_selection();

    // external drag & drop
    virtual std::string get_node_drag_type(const bec::NodeId &node) {
      return "";
    }

    virtual bool should_accept_file_drop_to_node(const bec::NodeId &node, const std::string &path) {
      return false;
    }

    virtual void add_file_to_node(const bec::NodeId &node, const std::string &path) {
    }
    virtual void add_file_data_to_node(const bec::NodeId &node, const char *data, size_t length) {
    }
    virtual std::string get_file_for_node(const bec::NodeId &node) {
      return "";
    }
    virtual bool get_file_data_for_node(const bec::NodeId &node, char *&data, size_t &length) {
      return true;
    }

    class MYSQLWBBACKEND_PUBLIC_FUNC Node {
    public:
      GrtObjectRef object;
      OverviewNodeType type;
      std::string label;
      std::string description;
      bec::IconId small_icon;
      bec::IconId large_icon;
      OverviewDisplayMode display_mode;
      bool expanded;
      bool selected;

      virtual Node *get_child(size_t i) {
        return 0;
      }
      virtual size_t count_children() {
        return 0;
      }
      virtual void refresh() {
      }

      virtual void focus(OverviewBE *sender) {
      }
      virtual bool activate(WBContext *wb) {
        return false;
      }
      virtual bool add_object(WBContext *wb) {
        return false;
      }
      virtual void delete_object(WBContext *wb) {
      }
      virtual bool is_deletable() {
        return false;
      }
      virtual void copy_object(WBContext *wb, bec::Clipboard *clip) {
      }
      virtual bool is_copyable() {
        return false;
      }
      virtual void paste_object(WBContext *wb, bec::Clipboard *clip) {
      }
      virtual bool is_pasteable(bec::Clipboard *clip) {
        return false;
      }

      virtual bool rename(WBContext *wb, const std::string &name) {
        return false;
      }
      virtual bool is_renameable() {
        return false;
      }

      virtual std::string get_unique_id() {
        return object.is_valid() ? object.id() : "";
      }

      virtual std::string get_detail(int field) {
        return "";
      }

      virtual int get_popup_menu_items(WBContext *wb, bec::MenuItemList &items);

      virtual workbench_OverviewPanelRef get_state() {
        workbench_OverviewPanelRef panel = workbench_OverviewPanelRef(grt::Initialized);

        panel->expandedHeight(0);
        panel->expanded(expanded ? 1 : 0);
        panel->itemDisplayMode((int)display_mode);

        return panel;
      }

      virtual void restore_state(const workbench_OverviewPanelRef &panel) {
        expanded = *panel->expanded() ? true : false;
        display_mode = (OverviewDisplayMode)*panel->itemDisplayMode();
      }

      Node() : type(ORoot), small_icon(0), large_icon(0), display_mode(MNone), expanded(false), selected(false) {
      }

      Node(const Node &node)
        : type(node.type),
          label(node.label),
          description(node.description),
          small_icon(node.small_icon),
          large_icon(node.large_icon),
          display_mode(MNone),
          expanded(node.expanded),
          selected(false) {
      }

      virtual ~Node() {
      }
    };

    class MYSQLWBBACKEND_PUBLIC_FUNC ObjectNode : public Node {
    public:
      ObjectNode() {
        type = OverviewBE::OItem;
      }
      ObjectNode(const ObjectNode &copy) : Node(copy) {
        type = OverviewBE::OItem;
      }

      virtual bool activate(WBContext *wb);
      virtual bool rename(WBContext *wb, const std::string &name);

      virtual void refresh() {
        label = object->name();
      }
    };

    class MYSQLWBBACKEND_PUBLIC_FUNC AddObjectNode : public virtual Node {
      std::function<bool(WBContext *)> _add_slot;

    public:
      AddObjectNode(const std::function<bool(WBContext *)> &add_slot) : _add_slot(add_slot) {
        type = OverviewBE::OItem;
      }

      virtual bool activate(WBContext *wb) {
        return _add_slot(wb);
      }
    };

    class MYSQLWBBACKEND_PUBLIC_FUNC ContainerNode : public virtual Node {
    public:
      virtual void init() {
      }

    public:
      std::vector<Node *> children;
      Node *focused;
      OverviewNodeType child_type;

      ContainerNode(OverviewNodeType subtype) : focused(0), child_type(subtype) {
      }
      ContainerNode(const ContainerNode &node)
        : Node(node), children(node.children), focused(node.focused), child_type(node.child_type) {
      }

      virtual workbench_OverviewPanelRef get_state() {
        workbench_OverviewPanelRef panel = Node::get_state();

        // XXXfor (std::list<int>::const_iterator i= selection.begin(); i != selection.end(); ++i)
        //  panel.selectedItems().insert(*i);

        return panel;
      }

      virtual void restore_state(const workbench_OverviewPanelRef &panel) {
        Node::restore_state(panel);

        // selection.clear();
        // XXX for (size_t c= panel.selectedItems().count(), i= 0; i < c; i++)
        //  selection.push_back(panel.selectedItems().get(i));
      }

      virtual ~ContainerNode() {
        clear_children();
      }

      virtual Node *get_child(size_t i) {
        if (i >= children.size())
          return 0;
        return children[i];
      }

      virtual size_t count_children() {
        return children.size();
      }

      void clear_children() {
        for (std::vector<Node *>::iterator iter = children.begin(); iter != children.end(); ++iter)
          delete *iter;
        children.clear();
      }

      virtual int count_detail_fields() {
        return 0;
      }
      virtual std::string get_detail_name(int field) {
        return "";
      }

      int get_focused_index() {
        int i = 0;
        for (std::vector<Node *>::iterator iter = children.begin(); iter != children.end(); ++iter) {
          if ((*iter) == focused)
            return i;
          ++i;
        }
        return -1;
      }

      virtual void refresh_children() {
      }
    };

  protected:
    WBContext *_wb;
    boost::signals2::signal<void()> _selection_change_signal;

    ContainerNode *_root_node;

    Node *get_node_by_id(const bec::NodeId &node) const {
      return do_get_node(node);
    }
    virtual Node *do_get_node(const bec::NodeId &node) const;

    Node *get_deepest_focused();

    void store_node_states(Node *node);
    void store_state();
    void restore_state();
  };
};
