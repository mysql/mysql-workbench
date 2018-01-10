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

#ifndef _WB_COMPONENT_PHYSICAL_H_
#define _WB_COMPONENT_PHYSICAL_H_

// Physical Model Handling

#include "workbench/wb_backend_public_interface.h"

#include "base/trackable.h"
#include "base/notifications.h"

#include "wb_component.h"

#include "grt/icon_manager.h"

#include "grts/structs.workbench.h"

#include "wbcanvas/workbench_physical_model_impl.h"

namespace wb {

  enum RelationshipType {
    Relationship11Id,
    Relationship1nId,
    RelationshipnmId,
    Relationship11NonId,
    Relationship1nNonId,
    RelationshipPick
  };

  enum ObjectType { ObjectTable, ObjectView, ObjectRoutineGroup };

  class CatalogTreeBE;
  class RelationshipFloater;

#define WB_TOOL_PTABLE "physical/table"
#define WB_TOOL_PROUTINEGROUP "physical/routinegroup"
#define WB_TOOL_PVIEW "physical/view"

#define WB_TOOL_PREL11_NOID "physical/rel11_noid"
#define WB_TOOL_PREL1n_NOID "physical/rel1n_noid"
#define WB_TOOL_PREL11 "physical/rel11"
#define WB_TOOL_PREL1n "physical/rel1n"
#define WB_TOOL_PRELnm "physical/relnm"
#define WB_TOOL_PREL_PICK "physical/relpick"

  class MYSQLWBBACKEND_PUBLIC_FUNC WBComponentPhysical : virtual public WBComponent {
  public:
    WBComponentPhysical(WBContext *wb);
    virtual ~WBComponentPhysical();

    static std::string name() {
      return "physical";
    }
    virtual std::string get_name() {
      return WBComponentPhysical::name();
    }
    virtual std::string get_diagram_class_name() {
      return workbench_physical_Diagram::static_class_name();
    }

    // Model
    db_SchemaRef add_new_db_schema(const workbench_physical_ModelRef &model);
    void delete_db_schema(const db_SchemaRef &schema);

    db_DatabaseObjectRef add_new_db_table(const db_SchemaRef &schema, const std::string &template_name = "");
    db_DatabaseObjectRef add_new_db_view(const db_SchemaRef &schema);
    db_DatabaseObjectRef add_new_db_routine(const db_SchemaRef &schema);
    db_DatabaseObjectRef add_new_db_routine_group(const db_SchemaRef &schema);

    db_ScriptRef add_new_stored_script(const workbench_physical_ModelRef &model, const std::string &path = "");
    GrtStoredNoteRef add_new_stored_note(const workbench_physical_ModelRef &model, const std::string &path = "");

    std::list<model_FigureRef> interactive_place_db_objects(ModelDiagramForm *vform, int x, int y,
                                                            const std::list<db_DatabaseObjectRef> &objects);
    std::list<model_FigureRef> interactive_place_db_objects(ModelDiagramForm *vform, int x, int y,
                                                            const std::list<db_DatabaseObjectRef> &objects,
                                                            grt::CopyContext &copy_context);

    model_FigureRef place_db_object(ModelDiagramForm *view, const base::Point &pos, const db_DatabaseObjectRef &object,
                                    bool select_figure = true);
    void place_new_db_object(ModelDiagramForm *view, const base::Point &pos, ObjectType type);

    db_DatabaseObjectRef clone_db_object_to_schema(const db_SchemaRef &schema, const db_DatabaseObjectRef &object,
                                                   grt::CopyContext &copy_context);

    virtual void block_model_notifications();
    virtual void unblock_model_notifications();

    virtual void delete_db_object(const db_DatabaseObjectRef &object);

    void setup_physical_model(workbench_DocumentRef &doc, const std::string &rdbms_name,
                              const std::string &rdbms_version);

    bool has_figure_for_object_in_active_view(const GrtObjectRef &object, ModelDiagramForm *vform = 0);

    void privilege_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value,
                                const db_CatalogRef &catalog);
    void remove_user(const db_UserRef &user);
    void remove_role(const db_RoleRef &role);

    db_UserRef add_new_user(const workbench_physical_ModelRef &model);
    db_RoleRef add_new_role(const workbench_physical_ModelRef &model);

    void remove_references_to_object(const db_DatabaseObjectRef &object);
    virtual void close_document();

  protected:
    enum RelationshipToolState { RIdle, RPickingStart, RPickingEnd, RFinished, RCancelled };

    class RelationshipToolContext : public base::trackable {
    private:
      WBComponentPhysical *owner;
      ModelDiagramForm *view;
      RelationshipToolState state;
      std::string last_message;
      RelationshipType type;
      workbench_physical_TableFigureRef hovering;
      std::vector<db_ColumnRef> columns;
      std::vector<db_ColumnRef> refcolumns;

      RelationshipFloater *floater;

      workbench_physical_TableFigureRef itable;
      workbench_physical_TableFigureRef ftable;

      bool pick_table(const workbench_physical_TableFigureRef &table);
      bool pick_reftable(const workbench_physical_TableFigureRef &table);

      bool pick_column(const workbench_physical_TableFigureRef &table, const db_ColumnRef &column);
      bool pick_refcolumn(const workbench_physical_TableFigureRef &table, const db_ColumnRef &column);
      bool done_picking_columns() {
        return (!columns.empty() && columns.size() == refcolumns.size());
      }

      bool finish_for_tables();
      bool finish_for_columns();
      bool finish();

      bool add_column(const db_ColumnRef &column);
      bool add_refcolumn(const db_ColumnRef &column);

      void on_figure_crossed(const model_ObjectRef &owner, mdc::CanvasItem *item, bool enter, const base::Point &pos);
      void enter_table(const workbench_physical_TableFigureRef &table);
      void leave_table(const workbench_physical_TableFigureRef &table);

      void source_picking_done();

    public:
      RelationshipToolContext(WBComponentPhysical *owner, ModelDiagramForm *form, RelationshipType type);

      void cancel();

      bool button_press(ModelDiagramForm *view, const base::Point &pos);
    };

    virtual void load_app_options(bool update);

    virtual void setup_context_grt(WBOptions *options);

    void init_catalog_grt(const db_mgmt_RdbmsRef &rdbms, const std::string &db_versionRef,
                          workbench_physical_ModelRef &model);

    grt::ListRef<db_UserDatatype> create_builtin_user_datatypes(const db_CatalogRef &catalog,
                                                                const db_mgmt_RdbmsRef &rdbms);

    virtual void setup_canvas_tool(ModelDiagramForm *view, const std::string &tool);

    virtual app_ToolbarRef get_tools_toolbar();
    virtual app_ToolbarRef get_tool_options(const std::string &tool);
    virtual grt::ListRef<app_ShortcutItem> get_shortcut_items();

    virtual void reset_document();
    virtual void document_loaded();
    void add_schema_listeners(const db_SchemaRef &schema);
    void add_schema_object_listeners(const grt::ObjectRef &object);

    virtual bool delete_model_object(const model_ObjectRef &object, bool figure_only);

    virtual bool handles_figure(const model_ObjectRef &figure);
    virtual bool can_paste_object(const grt::ObjectRef &object);
    virtual model_ObjectRef paste_object(ModelDiagramForm *view, const grt::ObjectRef &object,
                                         grt::CopyContext &copy_context);
    virtual void copy_object_to_clipboard(const grt::ObjectRef &object, grt::CopyContext &copy_context);

    virtual void activate_canvas_object(const model_ObjectRef &figure, bool newwindow);

    virtual std::string get_object_tooltip(const model_ObjectRef &object, mdc::CanvasItem *item);

    // Toolbar Handling
    virtual std::vector<std::string> get_command_dropdown_items(const std::string &option);

    // drag&drop
    virtual bool accepts_drop(ModelDiagramForm *view, int x, int y, const std::string &type,
                              const std::list<GrtObjectRef> &objects);
    virtual bool perform_drop(ModelDiagramForm *view, int x, int y, const std::string &type,
                              const std::list<GrtObjectRef> &objects);
    virtual bool perform_drop(ModelDiagramForm *view, int x, int y, const std::string &type, const std::string &data);

  public:
    virtual GrtObjectRef get_object_for_figure(const model_ObjectRef &object);

  private:
    grt::DictRef delete_db_schema(const db_SchemaRef &schema, bool check_empty);

    RelationshipToolContext *start_relationship(ModelDiagramForm *view, const base::Point &pos, RelationshipType type);
    void cancel_relationship(ModelDiagramForm *view, RelationshipToolContext *rctx);

    bool create_nm_relationship(ModelDiagramForm *view, workbench_physical_TableFigureRef table1,
                                workbench_physical_TableFigureRef table2, bool imandatory, bool fmandatory);

  private:
    std::map<std::string, app_ToolbarRef> _toolbars;
    grt::ListRef<app_ShortcutItem> _shortcuts;

    std::vector<std::string> _collation_list;

    std::map<std::string, boost::signals2::connection> _object_listeners;

    std::map<std::string, boost::signals2::connection> _schema_content_listeners;
    std::map<std::string, boost::signals2::connection> _schema_list_listeners;
    //    std::list<sigc::connection> _blockable_listeners;

    std::map<std::string, boost::signals2::connection> _figure_list_listeners;
    boost::signals2::connection _model_list_listener;
    boost::signals2::connection _catalog_object_list_listener;

    void refresh_ui_for_object(const GrtObjectRef &object);

    bool update_table_fk_connection(const db_TableRef &table, const db_ForeignKeyRef &fk, bool added);

    // Listeners
    void model_object_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value);

    void view_object_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value,
                                  const model_DiagramRef &view);

    void catalog_object_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value,
                                     const db_CatalogRef &catalog);
    void schema_object_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value,
                                    const db_SchemaRef &schema);

    void foreign_key_changed(const db_ForeignKeyRef &fk);

    void schema_content_object_changed(const db_DatabaseObjectRef &object);

    void schema_member_changed(const std::string &name, const grt::ValueRef &ovalue, const db_SchemaRef &schema);

    bool handle_button_event(ModelDiagramForm *, mdc::MouseButton, bool, base::Point, mdc::EventState, void *data);
  };
};

#endif
