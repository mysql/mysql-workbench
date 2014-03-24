/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WBCONTEXT_MODEL_H_
#define _WBCONTEXT_MODEL_H_


#include "workbench/wb_backend_public_interface.h"
#include "base/notifications.h"

#include "wbcanvas/model_model_impl.h"

#include <grts/structs.app.h>
#include <grts/structs.model.h>
#include <grts/structs.workbench.h>
#include <grts/structs.ui.h>


#define MODEL_DOCKING_POINT "workbench.physical.Model:main"

namespace grt {
  class UndoAction;
};

namespace mforms
{
  class View;
  class TabView;
  class TreeNodeView;
  class DockingPoint;
};

class TableTemplatePanel;
class UserDefinedTypeEditor;

namespace wb
{
  class WBContextUI;
  class ModelDiagramForm;
  class PhysicalOverviewBE;
  class ModelFile;
  class UserDatatypeList;
  class HistoryTree;
  
  class MYSQLWBBACKEND_PUBLIC_FUNC WBContextModel : public ModelBridgeDelegate, public base::trackable, base::Observer
  {    
  public:
    WBContextModel(WBContextUI *wbui);
    virtual ~WBContextModel();
    
    static void detect_auto_save_files(const std::string &autosave_dir);
    static std::map<std::string, std::string> auto_save_files();
    bool auto_save_document();

    mforms::View *shared_secondary_sidebar();
  public:
    WBContextUI *get_wbui() { return _wbui; }
    PhysicalOverviewBE *get_overview() { return _overview; }
    grt::GRT *get_grt();

    mforms::TreeNodeView *create_user_type_list();
    void show_user_type_editor(workbench_physical_ModelRef model);

    GrtVersionRef get_target_version();

    mforms::TreeNodeView *create_history_tree();
    mforms::TreeNodeView *create_catalog_tree();

    model_DiagramRef get_active_model_diagram(bool main_form);
    model_ModelRef get_active_model(bool main_form);
    
    // return the named toolbar    
    void model_created(ModelFile *file, workbench_DocumentRef doc);
    void model_loaded(ModelFile *file, workbench_DocumentRef doc);
    void model_closed();
    
    void register_diagram_form(ModelDiagramForm *view);
    
    ModelDiagramForm *get_diagram_form_for_diagram_id(const std::string &id) { return _model_forms.find(id) == _model_forms.end() ? 0 : _model_forms[id]; }
    ModelDiagramForm *get_diagram_form(mdc::CanvasView *view);
    
    void notify_diagram_created(ModelDiagramForm *view);
    void notify_diagram_destroyed(ModelDiagramForm *view);
    
    void realize();
    void unrealize();
    
    void activate_canvas_object(const model_ObjectRef &object, int flags);
    
    void update_page_settings();
    
    void export_png(const std::string &path);
    void export_pdf(const std::string &path);
    void export_ps(const std::string &path);
    void export_svg(const std::string &path);
    
    // Diagrams
    model_DiagramRef get_view_with_id(const std::string &id);

    void add_new_diagram(const model_ModelRef &model);
    
    void switch_diagram(const model_DiagramRef &view);
        
    bool delete_diagram(const model_DiagramRef &view);
    bool delete_object(model_ObjectRef object);
    bool remove_figure(model_ObjectRef object);
    
    GrtObjectRef duplicate_object(const db_DatabaseObjectRef &object, grt::CopyContext &copy_context);

  public:
    void update_plugin_arguments_pool(bec::ArgumentPool &args);
    
    int get_object_list_popup_items(bec::UIForm *form, const std::vector<bec::NodeId> &nodes,
                                    const grt::ListRef<GrtObject> &objects,
                                    const std::string &label, const std::list<std::string> &groups, bec::MenuItemList &items);
    
    void begin_plugin_exec();
    void end_plugin_exec();

  public:
    boost::signals2::signal<void ()> _udt_list_changed;

  private:
    // delegate functions from ModelBridgeDelegate
    virtual cairo_surface_t *fetch_image(const std::string &file);
    virtual std::string attach_image(const std::string &file);
    virtual void release_image(const std::string &name);
    
    virtual mdc::CanvasView *create_diagram(const model_DiagramRef &view);
    virtual void free_canvas_view(mdc::CanvasView *view);
    
    mdc::CanvasView* create_diagram_main(const model_DiagramRef &mview);
        
    void update_current_diagram(bec::UIForm *form);
    
    void diagram_object_changed(const std::string &member, const grt::ValueRef &ovalue, ModelDiagramForm *view);
    void diagram_object_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value, ModelDiagramForm *vform);
    void option_changed(grt::internal::OwnedDict*, bool, const std::string&);
    
    bool has_selected_model();
    bool has_selected_schema();
    bool has_selected_figures();
    void add_model_schema();
    void add_model_table();
    void add_model_view();
    void add_model_rgroup();
    void add_model_diagram();
    void remove_figure();

    void page_settings_changed(const std::string &field, const grt::ValueRef &value);
    
    int add_object_plugins_to_popup_menu(const grt::ListRef<GrtObject> &objects,
                                         const std::list<std::string> &groups, bec::MenuItemList &items);
    
    void history_changed();
    void selection_changed();
    
    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);

    void setup_secondary_sidebar();
  private:
    WBContextUI *_wbui;
    PhysicalOverviewBE *_overview;
    ModelFile *_file;
    UserDefinedTypeEditor* _current_user_type_editor;
    mdc::CanvasView *_locked_view_for_plugin_exec;

    ui_ModelPanelRef _grtmodel_panel;
    mforms::TabView *_secondary_sidebar;
    TableTemplatePanel *_template_panel;
    mforms::DockingPoint *_sidebar_dockpoint;

    workbench_DocumentRef _doc;
    boost::signals2::connection _page_settings_conn;

    grt::UndoAction *_auto_save_point;
    mdc::Timestamp _last_auto_save_time;
    int _auto_save_interval;
    bec::GRTManager::Timer *_auto_save_timer;

    std::map<std::string, ModelDiagramForm*> _model_forms;
  };
};

#endif
