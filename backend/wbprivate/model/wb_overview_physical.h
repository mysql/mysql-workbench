/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench/wb_overview.h"

#include "base/notifications.h"

namespace mforms {
  class MenuBar;
  class ToolBar;
};

namespace wb {
  class PhysicalOverviewBE;

  namespace internal {
    class PhysicalSchemaNode;

    class PhysicalSchemataNode : public OverviewBE::ContainerNode {
      virtual OverviewBE::Node *create_child_node(db_SchemaRef schema);

      workbench_physical_ModelRef model;

      virtual bool add_object(WBContext *wb);
      virtual void delete_object(WBContext *wb);
      virtual void refresh_children();

    public:
      PhysicalSchemataNode(workbench_physical_ModelRef model);
      virtual void init();
    };

    class SQLScriptsNode : public OverviewBE::ContainerNode {
    private:
      PhysicalOverviewBE *_owner;
      std::string id;
      workbench_physical_ModelRef _model;

      bool add_new(WBContext *wb);

    public:
      SQLScriptsNode(workbench_physical_ModelRef model, PhysicalOverviewBE *owner);

      virtual void refresh_children();

      virtual int get_popup_menu_items(WBContext *wb, bec::MenuItemList &items);
      virtual std::string get_unique_id() {
        return id;
      }
    };

    class NotesNode : public OverviewBE::ContainerNode {
    private:
      PhysicalOverviewBE *_owner;
      std::string id;
      workbench_physical_ModelRef _model;

      bool add_new(WBContext *wb);

    public:
      NotesNode(workbench_physical_ModelRef model, PhysicalOverviewBE *owner);

      virtual void refresh_children();

      virtual int get_popup_menu_items(WBContext *wb, bec::MenuItemList &items);
      virtual std::string get_unique_id() {
        return id;
      }
    };
  };

  //
  // Node Layout:
  //
  // -root: ORoot  (model)   (stable nodeid)
  //   -diagrams: ODivision  (stable noteid)  <RefreshChildren>
  //     -[diagram nodes]: OItem   <RefreshNode>
  //   -schemata: ODivision     (stable nodeid) <RefreshChildren>
  //     -[schema]: OGroup   (kind of stable by oid*)  <RefreshNode>
  //       -table: OSection (stable for parent)   <RefreshChildren>
  //         -[table nodes]: OItem      <RefreshNode>
  //       -view: OSection (stable for parent)    <RefreshChildren>
  //         -[view nodes]: OItem       <RefreshNode>
  //       -routine: OSection (stable for parent) <RefreshChildren>
  //         -[routine nodes]: OItem    <RefreshNode>
  //       -routineGroup: OSection (stable for parent)<RefreshChildren>
  //         -[routineGroup nodes]: OItem <RefreshNode>
  //   -privileges: ODivision  (stable nodeid)  <RefreshChildren>
  //     -users: OSection (stable nodeid)
  //       -[users]: OItem
  //     -roles: OSection (stable nodeid) <RefreshChildren>
  //       -[roles]: OItem
  //   -scripts: ODivision (stable nodeid)  <RefreshChildren>
  //     -[scripts] OItem
  //   -notes: ODivision (stable nodeid)  <RefreshChildren>
  //     -[notes] OItem
  //
  // Nodes marked <RefreshChildren> must have their contents refreshable when
  // Refresh message is received by the frontend. Arguments are paths.
  class MYSQLWBBACKEND_PUBLIC_FUNC PhysicalOverviewBE : public OverviewBE, public base::Observer {
    workbench_physical_ModelRef _model;

    virtual OverviewBE::ContainerNode *create_root_node(workbench_physical_ModelRef model, PhysicalOverviewBE *owner);

  protected:
    mforms::MenuBar *_menu;
    mforms::ToolBar *_toolbar;
    int _schemata_node_index;

    void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
    void update_toolbar_icons();

  public: // backend internal
    virtual std::string identifier() const;
    virtual std::string get_title();

    virtual std::string get_form_context_name() const;

    virtual void send_refresh_diagram(const model_DiagramRef &view);
    void send_refresh_users();
    void send_refresh_roles();
    void send_refresh_scripts();
    void send_refresh_notes();
    void send_refresh_schema_list();
    void send_refresh_for_schema(const db_SchemaRef &schema, bool refresh_object_itself);
    void send_refresh_for_schema_object(const GrtObjectRef &object, bool refresh_object_itself);

    void set_model(workbench_physical_ModelRef model);

    virtual mforms::ToolBar *get_toolbar();
    virtual mforms::MenuBar *get_menubar();

  public:
    PhysicalOverviewBE(WBContext *wb);
    virtual ~PhysicalOverviewBE();

    virtual bool can_undo();
    virtual bool can_redo();
    virtual void undo();
    virtual void redo();

    virtual bool can_close();
    virtual void close();

    virtual model_ModelRef get_model();
    internal::PhysicalSchemaNode *get_active_schema_node();

    virtual int get_default_tab_page_index();

    virtual std::string get_node_drag_type(const bec::NodeId &node);
    virtual bool should_accept_file_drop_to_node(const bec::NodeId &node, const std::string &path);
    virtual void add_file_to_node(const bec::NodeId &node, const std::string &path);
    virtual bool get_file_data_for_node(const bec::NodeId &node, char *&data, size_t &length);
    virtual std::string get_file_for_node(const bec::NodeId &node);

    virtual void refresh_node(const bec::NodeId &node, bool children);
  };
};
