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

#include "base/file_utilities.h"
#include "base/file_functions.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "base/log.h"

#include "wb_context_model.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_command_ui.h"

#include "wb_component.h"
#include "wb_component_basic.h"
#include "wb_component_physical.h"
#include "wb_component_logical.h"
#include "model/wb_model_diagram_form.h"
#include "wb_overview_physical.h"
#include "wb_overview_physical_schema.h"
#include "wb_catalog_tree_view.h"

#include "objimpl/wrapper/mforms_ObjectReference_impl.h"
#include "workbench/wb_model_file.h"
#include "model/wb_overview_physical.h"
#include "model/wb_user_datatypes.h"
#include "model/wb_history_tree.h"
#include "wbcanvas/model_diagram_impl.h"
#include "wbcanvas/workbench_physical_model_impl.h"
#include "workbench/wb_model_file.h"
#include "grtdb/db_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "grt/clipboard.h"

#include "grtpp_notifications.h"

#include "user_defined_type_editor.h"
#include "wb_template_list.h"

#include "mforms/tabview.h"
#include "mforms/tabview_dock.h"
#include "mforms/utilities.h"
#include "mforms/menubar.h"
#include "mforms/dockingpoint.h"

using namespace base;
using namespace bec;
using namespace wb;

DEFAULT_LOG_DOMAIN("ModelContext");

static std::map<std::string, std::string> auto_save_files;

WBContextModel::WBContextModel()
  : _file(0),
    _current_user_type_editor(0),
    _locked_view_for_plugin_exec(0),
    _auto_save_point(0),
    _last_auto_save_time(0),
    _auto_save_timer(NULL)

{
  _overview = new PhysicalOverviewBE(wb::WBContextUI::get()->get_wb());

  scoped_connect(bec::GRTManager::get()->get_clipboard()->signal_changed(),
                 std::bind(&WBContextModel::selection_changed, this));
  scoped_connect(_overview->signal_selection_changed(),
                 std::bind(&WBContextModel::selection_changed, this)); // make edit menu captions to update

  CommandUI *cmdui = wb::WBContextUI::get()->get_command_ui();
  std::function<bool()> validate = std::bind(&WBContextModel::has_selected_schema, this);
  cmdui->add_builtin_command("addModelDiagram", std::bind(&WBContextModel::add_model_diagram, this),
                             std::bind(&WBContextModel::has_selected_model, this));
  cmdui->add_builtin_command("addModelSchema", std::bind(&WBContextModel::add_model_schema, this),
                             std::bind(&WBContextModel::has_selected_model, this));
  cmdui->add_builtin_command("addModelTable", std::bind(&WBContextModel::add_model_table, this), validate);
  cmdui->add_builtin_command("addModelView", std::bind(&WBContextModel::add_model_view, this), validate);
  cmdui->add_builtin_command("addModelRoutine", std::bind(&WBContextModel::add_model_rgroup, this), validate);

  cmdui->add_builtin_command("removeFigure", std::bind([this]() { remove_figure(); }),
                             std::bind(&WBContextModel::has_selected_figures, this));

  base::NotificationCenter::get()->add_observer(this, "GNMainFormChanged");

  // Setup auto-save for model, only full seconds.
  int interval = (int)wb::WBContextUI::get()->get_wb()->get_root()->options()->options().get_int(
    "workbench:AutoSaveModelInterval", 60);
  if (interval > 0)
    _auto_save_timer =
      bec::GRTManager::get()->run_every(std::bind(&WBContextModel::auto_save_document, this), interval);
  _auto_save_interval = interval;

  _secondary_sidebar = NULL;
  _sidebar_dockpoint = NULL;
  _template_panel = NULL;

  scoped_connect(wb::WBContextUI::get()->get_wb()->get_root()->options()->signal_dict_changed(),
                 std::bind(&WBContextModel::option_changed, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3));

  setup_secondary_sidebar();
}

WBContextModel::~WBContextModel() {
  _grtmodel_panel.clear();

  if (_secondary_sidebar != NULL)
    _secondary_sidebar->release();
  if (_sidebar_dockpoint != NULL)
    _sidebar_dockpoint->release();

  delete _template_panel;

  base::NotificationCenter::get()->remove_observer(this);

  if (_doc.is_valid() && _doc->physicalModels().is_valid() && _doc->physicalModels().count() > 0)
    _doc->physicalModels().get(0)->get_data()->set_delegate(NULL);

  if (_auto_save_timer)
    bec::GRTManager::get()->cancel_timer(_auto_save_timer);
  CommandUI *cmdui = wb::WBContextUI::get()->get_command_ui();
  cmdui->remove_builtin_command("addModelDiagram");
  cmdui->remove_builtin_command("addModelSchema");
  cmdui->remove_builtin_command("addModelTable");
  cmdui->remove_builtin_command("addModelView");
  cmdui->remove_builtin_command("addModelRoutine");
  cmdui->remove_builtin_command("removeFigure");
  _file = 0;
  delete _overview;
}

void WBContextModel::setup_secondary_sidebar() {
  // Setup the secondary model sidebar, which should be shared between all model tabs
  _secondary_sidebar = mforms::manage(new mforms::TabView(mforms::TabViewSelectorSecondary));
  _template_panel = new TableTemplatePanel(this);
  _secondary_sidebar->add_page(_template_panel, _("Templates"));
}

void WBContextModel::notify_catalog_tree_view(const CatalogNodeNotificationType &notify_type, grt::ValueRef value,
                                              const std::string &diagram_id) {
  std::map<std::string, ModelDiagramForm *>::iterator it;
  if (diagram_id.empty()) {
    for (it = _model_forms.begin(); it != _model_forms.end(); ++it)
      it->second->notify_catalog_tree(notify_type, value);
  } else {
    it = _model_forms.find(diagram_id);
    if (it != _model_forms.end()) {
      it->second->notify_catalog_tree(notify_type, value);
    }
  }
}

void WBContextModel::refill_catalog_tree() {
  std::map<std::string, ModelDiagramForm *>::iterator it;
  for (it = _model_forms.begin(); it != _model_forms.end(); ++it)
    it->second->refill_catalog_tree();
}

mforms::TreeView *WBContextModel::create_user_type_list() {
  UserDatatypeList *type_list;

  type_list = new UserDatatypeList(wb::WBContextUI::get()->get_wb());
  type_list->set_catalog(wb::WBContextUI::get()->get_wb()->get_document()->physicalModels()[0]->catalog());
  type_list->refresh();

  type_list->scoped_connect(&_udt_list_changed, std::bind(&UserDatatypeList::refresh, type_list));

  return type_list;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeView *WBContextModel::create_history_tree() {
  HistoryTree *history_tree = new HistoryTree(grt::GRT::get()->get_undo_manager());
  history_tree->refresh();
  return history_tree;
}

//--------------------------------------------------------------------------------------------------

void WBContextModel::option_changed(grt::internal::OwnedDict *dict, bool, const std::string &key) {
  if (key == "workbench:AutoSaveModelInterval" &&
      dict == wb::WBContextUI::get()->get_wb()->get_wb_options().valueptr()) {
    auto_save_document();
  }
}

bool WBContextModel::auto_save_document() {
  WBContext *wb = wb::WBContextUI::get()->get_wb();
  ssize_t interval = wb->get_root()->options()->options().get_int("workbench:AutoSaveModelInterval", 60);
  if (interval <= 0)
    return false;

  workbench_DocumentRef doc(wb->get_document());

  mdc::Timestamp now = mdc::get_time();
  if (now - _last_auto_save_time > interval && _file && doc.is_valid() &&
      !bec::GRTManager::get()->get_dispatcher()->get_busy() &&
      grt::GRT::get()->get_undo_manager()->get_latest_closed_undo_action() != _auto_save_point) {
    _auto_save_point = grt::GRT::get()->get_undo_manager()->get_latest_closed_undo_action();
    _last_auto_save_time = now;
    try {
      // save the document in the same directory containing the expanded mwb file
      _file->store_document_autosave(doc);
    } catch (std::exception &exc) {
      wb->show_exception(_("Could not store document data to autosave file."), exc);
    }
  }

  if (interval != _auto_save_interval) {
    if (_auto_save_timer)
      bec::GRTManager::get()->cancel_timer(_auto_save_timer);
    // schedule new interval
    _auto_save_timer =
      bec::GRTManager::get()->run_every(std::bind(&WBContextModel::auto_save_document, this), (double)interval);
    return false;
  }

  return true;
}

void WBContextModel::detect_auto_save_files(const std::string &autosave_dir) {
  std::map<std::string, std::string> files;

  // look for .mwbd folders with autosave files
  std::list<std::string> autosaves;

  try {
    autosaves = base::scan_for_files_matching(base::makePath(autosave_dir, "*.mwbd*"));
  } catch (const std::runtime_error &) {
    return;
  }

  for (std::list<std::string>::const_iterator d = autosaves.begin(); d != autosaves.end(); ++d) {
    if (!g_file_test(d->c_str(), G_FILE_TEST_IS_DIR))
      continue;

    if (base::LockFile::check(base::makePath(*d, ModelFile::lock_filename.c_str())) != base::LockFile::NotLocked)
      continue;

    if (g_file_test(base::makePath(*d, MAIN_DOCUMENT_AUTOSAVE_NAME).c_str(), G_FILE_TEST_EXISTS)) {
      std::string path = base::makePath(*d, "real_path");
      gchar *orig_path;
      gsize length;
      if (g_file_test(path.c_str(), (GFileTest)(G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS)) &&
          g_file_get_contents(path.c_str(), &orig_path, &length, NULL)) {
        files[std::string(orig_path, length)] = *d;
        g_free(orig_path);
      } else {
        std::string fname = base::basename(*d);
        fname = fname.substr(0, fname.rfind('.')).append(".mwb");
        // if no real_path file in the autosave, this could be an autosave from an older version
        files[fname] = *d;
      }
    } else {
      logInfo("Found model auto-save %s, but it is empty. Deleting it...\n", d->c_str());
      base_rmdir_recursively(d->c_str());
    }
  }
  ::auto_save_files = files;
}

std::map<std::string, std::string> WBContextModel::auto_save_files() {
  return ::auto_save_files;
}

void WBContextModel::unrealize() {
  _page_settings_conn.disconnect();

  // unrealize all models
  if (_doc.is_valid() && _doc->physicalModels().is_valid()) {
    for (size_t c = _doc->physicalModels().count(), i = 0; i < c; i++) {
      _doc->physicalModels().get(i)->get_data()->unrealize();
    }
  }
}

model_DiagramRef WBContextModel::get_active_model_diagram(bool main_form) {
  bec::UIForm *form =
    main_form ? wb::WBContextUI::get()->get_active_main_form() : wb::WBContextUI::get()->get_active_form();

  if (dynamic_cast<ModelDiagramForm *>(form))
    return dynamic_cast<ModelDiagramForm *>(form)->get_model_diagram();

  return model_DiagramRef();
}

model_ModelRef WBContextModel::get_active_model(bool main_form) {
  bec::UIForm *form =
    main_form ? wb::WBContextUI::get()->get_active_main_form() : wb::WBContextUI::get()->get_active_form();

  if (dynamic_cast<OverviewBE *>(form))
    return dynamic_cast<OverviewBE *>(form)->get_model();
  else if (dynamic_cast<ModelDiagramForm *>(form))
    return dynamic_cast<ModelDiagramForm *>(form)->get_model_diagram()->owner();
  return model_ModelRef();
}

void WBContextModel::model_created(ModelFile *file, workbench_DocumentRef doc) {
  _file = file;
  _doc = doc;

  std::string target_version = bec::GRTManager::get()->get_app_option_string("DefaultTargetMySQLVersion");
  if (target_version.empty())
    target_version = base::getVersion();

  wb::WBContextUI::get()->get_wb()->get_component<WBComponentLogical>()->setup_logical_model(_doc);
  wb::WBContextUI::get()->get_wb()->get_component<WBComponentPhysical>()->setup_physical_model(_doc, "Mysql",
                                                                                               target_version);

  wb::WBContextUI::get()->get_wb()->foreach_component(std::bind(&WBComponent::reset_document, std::placeholders::_1));

  _doc->physicalModels().get(0)->get_data()->set_delegate(this);

  _doc->physicalModels()[0]->get_data()->realize();

  wb::WBContextUI::get()->get_wb()->request_refresh(RefreshNewModel, "", 0);

  // setup GRT proxy object
  _grtmodel_panel = ui_ModelPanelRef(grt::Initialized);
  _grtmodel_panel->model(_doc->physicalModels()[0]);

  if (_sidebar_dockpoint == NULL)
    _sidebar_dockpoint = mforms::manage(
      new mforms::DockingPoint(new mforms::TabViewDockingPoint(_secondary_sidebar, MODEL_DOCKING_POINT), true));
  _grtmodel_panel->commonSidebar(mforms_to_grt(_sidebar_dockpoint));

  grt::DictRef info(true);
  grt::GRTNotificationCenter::get()->send_grt("GRNModelCreated", _grtmodel_panel, info);
}

void WBContextModel::model_loaded(ModelFile *file, workbench_DocumentRef doc) {
  _file = file;
  _doc = doc;

  wb::WBContextUI::get()->get_wb()->foreach_component(std::bind(&WBComponent::reset_document, std::placeholders::_1));

  wb::WBContextUI::get()->get_wb()->foreach_component(std::bind(&WBComponent::document_loaded, std::placeholders::_1));

  _doc->physicalModels().get(0)->get_data()->set_delegate(this);

  wb::WBContextUI::get()->get_wb()->request_refresh(RefreshNewModel, "", 0);

  std::string temp_dir = _file->get_tempdir_path();
  for (std::map<std::string, std::string>::iterator iter = ::auto_save_files.begin(); iter != ::auto_save_files.end();
       ++iter) {
    if (iter->second == temp_dir) {
      ::auto_save_files.erase(iter);
      wb::WBContextUI::get()->refresh_home_documents();
      break;
    }
  }

  // setup GRT proxy object
  _grtmodel_panel = ui_ModelPanelRef(grt::Initialized);
  _grtmodel_panel->model(_doc->physicalModels()[0]);

  if (_sidebar_dockpoint == NULL)
    _sidebar_dockpoint = mforms::manage(
      new mforms::DockingPoint(new mforms::TabViewDockingPoint(_secondary_sidebar, MODEL_DOCKING_POINT), true));
  _grtmodel_panel->commonSidebar(mforms_to_grt(_sidebar_dockpoint));

  grt::DictRef info(true);
  grt::GRTNotificationCenter::get()->send_grt("GRNModelOpened", _grtmodel_panel, info);
}

void WBContextModel::model_closed() {
  grt::DictRef info(true);
  grt::GRTNotificationCenter::get()->send_grt("GRNModelClosed", _grtmodel_panel, info);
}

void WBContextModel::realize() {
  _page_settings_conn = _doc->pageSettings()->signal_changed()->connect(
    std::bind(&WBContextModel::page_settings_changed, this, std::placeholders::_1, std::placeholders::_2));

  _doc->physicalModels()[0]->get_data()->realize();
}

void WBContextModel::page_settings_changed(const std::string &field, const grt::ValueRef &value) {
  if (field == "paperType") {
    update_page_settings();
  }
}

/**
 ****************************************************************************
 * @brief Update the canvas view according to app.PageSettings
 *
 * This will update the page size and total view size to reflect changes
 * to the page/print settings.
 ****************************************************************************
 */
void WBContextModel::update_page_settings() {
  if (!_doc.is_valid() || !_doc->logicalModel().is_valid())
    return;

  grt::ListRef<model_Diagram> views(grt::ListRef<model_Diagram>::cast_from(_doc->logicalModel()->diagrams()));
  for (size_t vc = views.count(), v = 0; v < vc; v++) {
    views[v]->get_data()->update_size();
  }

  grt::ListRef<workbench_physical_Model> models(_doc->physicalModels());
  for (size_t c = models.count(), i = 0; i < c; i++) {
    views = grt::ListRef<model_Diagram>::cast_from(models[i]->diagrams());
    for (size_t vc = views.count(), v = 0; v < vc; v++) {
      views[v]->get_data()->update_from_page_size();
    }
  }
}

cairo_surface_t *WBContextModel::fetch_image(const std::string &file) {
  return wb::WBContextUI::get()->get_wb()->get_file()->get_image(file);
}

std::string WBContextModel::attach_image(const std::string &file) {
  return wb::WBContextUI::get()->get_wb()->get_file()->add_image_file(file);
}

void WBContextModel::release_image(const std::string &file) {
  // QQQ
  // wb::WBContextUI::get()->get_wb()->get_file()->release_image(file);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Delegate method for creating canvas views
 *
 * This is called by the GRT bridge when a new view object is created.
 * It will in turn, call a frontend supplied callback which should create
 * the canvas view (and its viewer) and then return the canvas view.
 *
 * The free_canvas_view() method will be called once the view can be freed.
 *
 * @param name the name of the canvas. Will be the object-id of the view.
 */
mdc::CanvasView *WBContextModel::create_diagram(const model_DiagramRef &view) {
  return wb::WBContextUI::get()->get_wb()->execute_in_main_thread<mdc::CanvasView *>(
    "create_diagram", std::bind(&WBContextModel::create_diagram_main, this, view));
}

//--------------------------------------------------------------------------------------------------

/**
 * Notification trigger for canvas view destructions. Can be called via two different paths.
 * If called from the front end (because the UI caused closing the editor) then the diagram form
 * is already unregistered and we don't need to call the UI again (it was the trigger after all).
 */
void WBContextModel::free_canvas_view(mdc::CanvasView *view) {
  ModelDiagramForm *diagram = get_diagram_form(view);
  if (diagram != NULL) {
    // This function is expected to be called from the main thread.
    notify_diagram_destroyed(diagram);

    // Notify front end so it can close its editor for this view.
    if (bec::GRTManager::get()->in_main_thread())
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->destroy_view(view);
    else
      wb::WBContextUI::get()->get_wb()->execute_in_main_thread<void>(
        "destroy view", std::bind(wb::WBContextUI::get()->get_wb()->_frontendCallbacks->destroy_view, view));
  }
}

//--------------------------------------------------------------------------------------------------

mdc::CanvasView *WBContextModel::create_diagram_main(const model_DiagramRef &diagram_reference) {
  ModelDiagramForm *diagram = 0;
  WBContext *wb = wb::WBContextUI::get()->get_wb();

  FOREACH_COMPONENT(wb->_components, iter) {
    if (diagram_reference.is_instance((*iter)->get_diagram_class_name()) &&
        (*iter)->get_diagram_class_name() != model_Diagram::static_class_name()) {
      diagram = new ModelDiagramForm(*iter, diagram_reference);
      break;
    }
  }

  // fallback
  if (!diagram)
    diagram = new ModelDiagramForm(wb->get_component_named("basic"), diagram_reference);

  if (!diagram) {
    mforms::Utilities::show_error("Internal error adding a new diagram.", "Unknown diagram type.", _("Close"));
    return 0;
  }

  scoped_connect(
    diagram_reference->signal_objectActivated(),
    (std::bind(&WBContextModel::activate_canvas_object, this, std::placeholders::_1, std::placeholders::_2)));

  scoped_connect(diagram_reference->signal_list_changed(),
                 std::bind(&WBContextModel::diagram_object_list_changed, this, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3, diagram));

  register_diagram_form(diagram);

  // Forward creation to front end.
  mdc::CanvasView *view = wb->_frontendCallbacks->create_diagram(diagram_reference);
  if (view) {
    diagram->attach_canvas_view(view);

    notify_diagram_created(diagram);

    // use this signal instead of selection_change from canvas so that when the callback is called
    // the grt diagram object already reflects the selection changes
    scoped_connect(diagram_reference->get_data()->signal_selection_changed(),
                   std::bind(&WBContextModel::selection_changed, this));

    wb->request_refresh(RefreshNewDiagram, diagram_reference.id(), (NativeHandle)view->get_user_data());
  } else {
    delete diagram;
    mforms::Utilities::show_error("Internal error adding a new diagram.", "Frontend did not return a diagram.",
                                  _("Close"));
  }

  if (getenv("DEBUG_CANVAS"))
    view->enable_debug(true);

  return view;
}

void WBContextModel::activate_canvas_object(const model_ObjectRef &object, ssize_t flags) {
  bool newwindow = flags & 1;

  FOREACH_COMPONENT(wb::WBContextUI::get()->get_wb()->_components, iter) {
    if ((*iter)->handles_figure(object))
      (*iter)->activate_canvas_object(object, newwindow);
  }
}

void WBContextModel::register_diagram_form(ModelDiagramForm *view) {
  _model_forms[view->get_model_diagram().id()] = view;
  view->refill_catalog_tree();
}

ModelDiagramForm *WBContextModel::get_diagram_form(mdc::CanvasView *view) {
  for (std::map<std::string, ModelDiagramForm *>::const_iterator iter = _model_forms.begin();
       iter != _model_forms.end(); ++iter) {
    if (iter->second->get_view() == view)
      return iter->second;
  }
  return 0;
}

void WBContextModel::notify_diagram_created(ModelDiagramForm *view) {
  view->scoped_connect(
    view->get_model_diagram()->signal_changed(),
    std::bind(&WBContextModel::diagram_object_changed, this, std::placeholders::_1, std::placeholders::_2, view));

  // now called from wb_component_physical.cpp:model_list_changed
  // wb::WBContextUI::get()->get_physical_overview()->send_refresh_diagram(model_DiagramRef());
}

void WBContextModel::notify_diagram_destroyed(ModelDiagramForm *diagram) {
  if (diagram != NULL) {
    std::string id = diagram->get_model_diagram().id();
    delete diagram;
    _model_forms.erase(id);
  }

  // now called from wb_component_physical.cpp:model_list_changed
  // wb::WBContextUI::get()->get_physical_overview()->send_refresh_diagram(model_DiagramRef());
}

void WBContextModel::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNMainFormChanged")
    update_current_diagram(wb::WBContextUI::get()->get_active_main_form());
}

void WBContextModel::update_current_diagram(bec::UIForm *form) {
  ModelDiagramForm *dform = dynamic_cast<ModelDiagramForm *>(form);
  if (dform) {
    model_DiagramRef diagram(dform->get_model_diagram());
    if (diagram.is_valid() && diagram->owner().is_valid())
      diagram->owner()->currentDiagram(diagram);

    wb::WBContextUI::get()->get_command_ui()->revalidate_edit_menu_items();
  }
}

void WBContextModel::diagram_object_changed(const std::string &member, const grt::ValueRef &ovalue,
                                            ModelDiagramForm *view) {
  if (member == "name") {
    if (view->get_model_diagram().is_valid()) {
      base::NotificationInfo info;
      info["form"] = view->form_id();
      info["title"] = view->get_title();
      base::NotificationCenter::get()->send("GNFormTitleDidChange", view, info);
      wb::WBContextUI::get()->get_physical_overview()->send_refresh_diagram(view->get_model_diagram());
    }
  } else if (member == "zoom") {
    wb::WBContextUI::get()->get_wb()->request_refresh(RefreshZoom, "");
  }
}

void WBContextModel::diagram_object_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value,
                                                 ModelDiagramForm *vform) {
  if (vform == wb::WBContextUI::get()->get_active_main_form()) {
    if (vform->get_model_diagram()->selection().valueptr() == list)
      wb::WBContextUI::get()->get_wb()->request_refresh(RefreshSelection, "",
                                                        reinterpret_cast<NativeHandle>(vform->get_frontend_data()));
  }
}

bool WBContextModel::has_selected_schema() {
  PhysicalOverviewBE *active_form = dynamic_cast<PhysicalOverviewBE *>(wb::WBContextUI::get()->get_active_main_form());
  if (active_form == _overview && _overview->get_active_schema_node())
    return true;

  return false;
}

bool WBContextModel::has_selected_figures() {
  ModelDiagramForm *view;
  model_DiagramRef diagram(get_active_model_diagram(false));
  if (!diagram.is_valid()) // in case an editor in a diagram tab is active
  {
    diagram = get_active_model_diagram(true);
    view = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());
  } else
    view = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_form());
  if (view && view->has_selection())
    return true;
  return false;
}

bool WBContextModel::has_selected_model() {
  if (wb::WBContextUI::get()->get_active_main_form() == _overview)
    return true;
  return false;
}

void WBContextModel::add_model_schema() {
  wb::WBContextUI::get()->get_wb()->get_component<WBComponentPhysical>()->add_new_db_schema(
    workbench_physical_ModelRef::cast_from(get_active_model(true)));
}

void WBContextModel::add_model_diagram() {
  add_new_diagram(get_active_model(true));
}

void WBContextModel::add_model_table() {
  if (_overview->get_active_schema_node())
    _overview->get_active_schema_node()->add_new_db_table(wb::WBContextUI::get()->get_wb());
}

void WBContextModel::add_model_view() {
  if (_overview->get_active_schema_node())
    _overview->get_active_schema_node()->add_new_db_view(wb::WBContextUI::get()->get_wb());
}

void WBContextModel::add_model_rgroup() {
  if (_overview->get_active_schema_node())
    _overview->get_active_schema_node()->add_new_db_routine(wb::WBContextUI::get()->get_wb());
}

void WBContextModel::remove_figure() {
  ModelDiagramForm *view;
  model_DiagramRef diagram(get_active_model_diagram(false));
  if (!diagram.is_valid()) { // in case an editor in a diagram tab is active
    diagram = get_active_model_diagram(true);
    view = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());
  } else
    view = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_form());
  if (view)
    view->remove_selection();
}

GrtObjectRef WBContextModel::duplicate_object(const db_DatabaseObjectRef &object, grt::CopyContext &copy_context) {
  std::set<std::string> skip;
  skip.insert("oldName");

  if (object.is_instance(db_Table::static_class_name())) {
    db_TableRef table(db_TableRef::cast_from(object));

    // copy table
    db_TableRef dbtable(db_TableRef::cast_from(copy_context.copy(table, skip)));

    copy_context.update_references();

    // post-processing
    // - Make foreign key names unique.
    ssize_t max_fk_len =
      workbench_physical_ModelRef::cast_from(dbtable->owner()->owner()->owner())->rdbms()->maximumIdentifierLength();
    grt::ListRef<db_ForeignKey> fks(dbtable->foreignKeys());
    std::set<std::string> used_fk_names =
      bec::SchemaHelper::get_foreign_key_names(db_SchemaRef::cast_from(dbtable->owner()));
    for (size_t c = fks.count(), i = 0; i < c; i++) {
      db_ForeignKeyRef fk(fks[i]);
      fk->name(bec::SchemaHelper::get_unique_foreign_key_name(used_fk_names, fk->name(), (int)max_fk_len));
    }

    if (grt::find_named_object_in_list(db_SchemaRef::cast_from(dbtable->owner())->tables(), dbtable->name()).is_valid())
      dbtable->name(grt::get_name_suggestion_for_list_object(db_SchemaRef::cast_from(dbtable->owner())->tables(),
                                                             *dbtable->name() + "_copy"));
    grt::AutoUndo undo;

    db_SchemaRef::cast_from(dbtable->owner())->tables().insert(dbtable);

    undo.end(strfmt(_("Duplicate Table '%s'"), dbtable->name().c_str()));

    return dbtable;
  } else if (object.is_instance(db_View::static_class_name())) {
    db_ViewRef view(db_ViewRef::cast_from(object));

    // copy view
    db_ViewRef dbview(db_ViewRef::cast_from(copy_context.copy(view)));

    if (grt::find_named_object_in_list(db_SchemaRef::cast_from(dbview->owner())->views(), dbview->name()).is_valid())
      dbview->name(grt::get_name_suggestion_for_list_object(db_SchemaRef::cast_from(dbview->owner())->views(),
                                                            *dbview->name() + "_copy"));
    grt::AutoUndo undo;

    db_SchemaRef::cast_from(dbview->owner())->views().insert(dbview);

    undo.end(strfmt(_("Duplicate View '%s'"), dbview->name().c_str()));

    return dbview;
  } else if (object.is_instance(db_RoutineGroup::static_class_name())) {
    db_RoutineGroupRef routineGroup(db_RoutineGroupRef::cast_from(object));

    // copy routineGroup
    db_RoutineGroupRef dbroutineGroup(db_RoutineGroupRef::cast_from(copy_context.copy(routineGroup)));

    if (grt::find_named_object_in_list(db_SchemaRef::cast_from(dbroutineGroup->owner())->routineGroups(),
                                       dbroutineGroup->name())
          .is_valid())
      dbroutineGroup->name(grt::get_name_suggestion_for_list_object(
        db_SchemaRef::cast_from(dbroutineGroup->owner())->routineGroups(), *dbroutineGroup->name() + "_copy"));
    grt::AutoUndo undo;

    db_SchemaRef::cast_from(dbroutineGroup->owner())->routineGroups().insert(dbroutineGroup);

    undo.end(strfmt(_("Duplicate Routine Group '%s'"), dbroutineGroup->name().c_str()));
    return dbroutineGroup;
  }
  return GrtObjectRef();
}

void WBContextModel::update_plugin_arguments_pool(ArgumentPool &args) {
  model_ModelRef model(get_active_model(true));

  if (!model.is_valid())
    return;

  args.add_entries_for_object("", model, "model.Model");
  args.add_entries_for_object("activeModel", model, "model.Model");

  if (workbench_physical_ModelRef::can_wrap(model)) {
    workbench_physical_ModelRef pmodel(workbench_physical_ModelRef::cast_from(model));
    args.add_entries_for_object("", pmodel->catalog(), "db.Catalog");
    args.add_entries_for_object("activeCatalog", pmodel->catalog(), "db.Catalog");
  }

  ModelDiagramForm *view;
  model_DiagramRef diagram(get_active_model_diagram(false));
  if (!diagram.is_valid()) { // in case an editor in a diagram tab is active
    diagram = get_active_model_diagram(true);
    view = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());
  } else
    view = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_form());

  if (diagram.is_valid()) {
    args.add_entries_for_object("", diagram, "model.Diagram");
    args.add_entries_for_object("activeDiagram", diagram, "model.Diagram");

    if (view) {
      // diagram selection
      grt::ListRef<model_Object> selection(view->get_selection());

      args.add_list_for_selection("activeDiagram", selection);

      if (selection.count() == 1) {
        model_ObjectRef object(selection[0]);

        // args.add_entries_for_object("", object, "model.Object");
        args.add_entries_for_object("", object, "GrtObject");

        // check if object represented by this is wanted
        FOREACH_COMPONENT(wb::WBContextUI::get()->get_wb()->_components, iter) {
          if ((*iter)->handles_figure(object)) {
            grt::ObjectRef fobject((*iter)->get_object_for_figure(object));
            if (fobject.is_valid())
              args.add_entries_for_object("", fobject, "db.DatabaseObject");
          }
        }
      }
    }
  }

  // overview selection
  OverviewBE *overview = dynamic_cast<OverviewBE *>(wb::WBContextUI::get()->get_active_form());
  if (overview) {
    grt::ListRef<GrtObject> selection(overview->get_selection());

    if (selection.count() == 1) {
      GrtObjectRef object(selection[0]);

      args.add_entries_for_object("", object, "GrtObject");
    }
  }
}

/**
 * Adds a list of common menu entries to the menu item list. Returns the number of items added.
 */
int WBContextModel::get_object_list_popup_items(bec::UIForm *form, const std::vector<bec::NodeId> &nodes,
                                                const grt::ListRef<GrtObject> &objects, const std::string &label,
                                                const std::list<std::string> &groups, bec::MenuItemList &items) {
  size_t initial_count = items.size();
  bec::TreeModel *model = dynamic_cast<bec::TreeModel *>(form);
  WBContext *wb = wb::WBContextUI::get()->get_wb();

  // Start with clipboard commands.
  // First check if all items in the selection list are deletable (used for cut and delete items).
  bec::MenuItem item;
  bool can_delete = objects->count() > 0;
  bool can_copy = can_delete;
  bool can_remove = true;
  if (model != NULL)
    for (std::vector<bec::NodeId>::const_iterator iterator = nodes.begin(); iterator != nodes.end(); iterator++) {
      if (can_delete && !model->is_deletable(*iterator))
        can_delete = false;
      if (can_copy && !model->is_copyable(*iterator))
        can_copy = false;
    }

  // can't copy/paste diagrams
  if (nodes.empty() || nodes[0][0] != 0) {
    item.checked = false;
    item.enabled = can_copy && can_delete;
    item.caption = label.empty() ? _("Cut") : strfmt(_("Cut %s"), label.c_str());
    item.accessibilityName = "Cut";
    item.internalName = "builtin:cut";
    item.type = MenuAction;
    items.push_back(item);

    item.enabled = can_copy;
    item.caption = label.empty() ? _("Copy") : strfmt(_("Copy %s"), label.c_str());
    item.accessibilityName = "Copy";
    item.internalName = "builtin:copy";
    item.type = MenuAction;
    items.push_back(item);

    item.enabled = form->can_paste();
    item.caption = strfmt(_("Paste %s"), wb->get_clipboard()->get_content_description().c_str());
    item.accessibilityName = "Paste";
    item.internalName = "builtin:paste";
    item.type = MenuAction;
    items.push_back(item);

    // this is already implemented, but not yet exposed.. uncomment once the whole thing is working
    //  item.enabled= form->can_paste();
    //  item.caption= strfmt(_("Duplicate %s"), wb->get_clipboard()->get_content_description().c_str());
    //  item.name= "builtin:duplicate";
    //  item.type= MenuAction;
    //  items.push_back(item);

    item.type = MenuSeparator;
    items.push_back(item);
  }

  // Plugin dependent menu entries.
  size_t old_count = items.size();

  if (objects.count() > 0) {
    add_object_plugins_to_popup_menu(objects, groups, items);

    bool has_objects = false;
    bool connections_only = true;
    grt::ListRef<GrtObject> model_objects(true);
    for (size_t c = objects.count(), i = 0; i < c; i++) {
      if (!objects[i].is_instance(model_Connection::static_class_name()))
        connections_only = false;
      if (objects[i].is_instance(model_Object::static_class_name())) {
        has_objects = true;
        FOREACH_COMPONENT(wb->_components, compo) {
          GrtObjectRef model_object((*compo)->get_object_for_figure(model_ObjectRef::cast_from(objects[i])));

          if (model_object.is_valid()) {
            model_objects.insert(model_object);
            break;
          }
        }
      }
    }
    if (connections_only || !has_objects)
      can_remove = false;

    if (model_objects.count() > 0)
      add_object_plugins_to_popup_menu(model_objects, groups, items);
    else
      can_remove = false;
  }

  // At the end of the list there is the delete command.
  item.checked = false;
  item.enabled = can_delete;

  // Add a separator item if we added plugin specific menu commands above.
  if (old_count != items.size()) {
    item.type = MenuSeparator;
    items.push_back(item);
  }
  item.caption = label.empty() ? _("Delete") : strfmt(_("Delete %s"), label.c_str());
  item.accessibilityName = "Delete";
  item.internalName = "builtin:delete";
  item.type = MenuAction;
  items.push_back(item);

  if ((nodes.empty() || nodes[0][0] != 0) && objects.count() > 0) {
    item.caption = label.empty() ? _("Remove Figure") : strfmt(_("Remove Figure %s"), label.c_str());
    item.internalName = "builtin:removeFigure";
    item.accessibilityName = "Remove Figure";
    item.type = MenuAction;
    item.enabled = can_remove;
    items.push_back(item);
  }
  return int(items.size() - initial_count);
}

struct sortplugin {
  bool operator()(const app_PluginRef &a, const app_PluginRef &b) const {
    return a->rating() < b->rating();
  }
};

int WBContextModel::add_object_plugins_to_popup_menu(const grt::ListRef<GrtObject> &objects,
                                                     const std::list<std::string> &groups, bec::MenuItemList &items) {
  bec::ArgumentPool argpool;
  wb::WBContextUI::get()->get_wb()->update_plugin_arguments_pool(argpool);
  if (objects.count() > 0)
    argpool.add_entries_for_object("", objects[0], "GrtObject");

  int count = 0;
  // look for plugins that take this object type as input
  std::vector<app_PluginRef> plugins(wb::WBContextUI::get()->get_wb()->get_plugin_manager()->get_plugins_for_objects(
    grt::ObjectListRef::cast_from(objects)));

  // sort by rating
  std::sort(plugins.begin(), plugins.end(), sortplugin());

  for (std::vector<app_PluginRef>::const_iterator iter = plugins.begin(); iter != plugins.end(); ++iter) {
    bool match = false;
    //    bool matchPrefix= false;

    for (size_t c = (*iter)->groups().count(), i = 0; i < c; i++) {
      std::string str = (*iter)->groups().get(i);
      for (std::list<std::string>::const_iterator ctx = groups.begin(); ctx != groups.end(); ++ctx) {
        if ((*ctx)[ctx->size() - 1] == '*' &&
            g_ascii_strncasecmp(ctx->c_str(), str.c_str(), (guint)ctx->size() - 1) == 0) {
          match = true;
          break;
        } else if ((*ctx)[ctx->size() - 1] != '*' && g_ascii_strcasecmp(ctx->c_str(), str.c_str()) == 0) {
          match = true;
          break;
        }
      }
    }
    if (!match)
      continue;

    bec::MenuItem item;

    item.type = MenuAction;
    item.caption = *(*iter)->caption() + ((*iter)->pluginType() == "gui" ? "..." : "");
    item.checked = false;
    item.enabled = bec::GRTManager::get()->check_plugin_runnable(*iter, argpool);
    item.shortcut = "";
    item.internalName = "plugin:" + *(*iter)->name();
    item.accessibilityName = (*iter)->accessibilityName();
    if (item.caption.empty())
      item.caption = item.accessibilityName;
    items.push_back(item);
    count++;

    if ((*iter)->groups().get_index("catalog/Editors") != grt::BaseListRef::npos ||
        (*iter)->groups().get_index("model/Editors") != grt::BaseListRef::npos) {
      app_PluginRef plugin(
        wb::WBContextUI::get()->get_wb()->get_plugin_manager()->get_plugin("wb.edit.editSelectedFigureInNewWindow"));

      item.caption = _("Edit in New Tab...");
      if (wb::WBContextUI::get()->get_active_form() &&
          !wb::WBContextUI::get()->get_active_form()->get_edit_target_name().empty()) {
        items.back().caption =
          base::strfmt(_("Edit %s..."), wb::WBContextUI::get()->get_active_form()->get_edit_target_name().c_str());

        item.caption = base::strfmt(_("Edit %s in New Tab..."),
                                    wb::WBContextUI::get()->get_active_form()->get_edit_target_name().c_str());
      }

      item.internalName = "plugin:" + *plugin->name();
      item.accessibilityName = *plugin->accessibilityName();
      item.type = MenuAction;
      // state for this item will be the same as for the previous one
      // item.enabled= check_plugin_runnable(plugin, "", objects);
      items.push_back(item);
      count++;
    }
  }

  return count;
}

void WBContextModel::history_changed() {
  std::string undo_description(grt::GRT::get()->get_undo_manager()->undo_description());
  std::string redo_description(grt::GRT::get()->get_undo_manager()->redo_description());

  std::list<bec::UIForm *> forms;
  forms.push_back(_overview);
  for (std::map<std::string, ModelDiagramForm *>::const_iterator iter = _model_forms.begin();
       iter != _model_forms.end(); ++iter)
    forms.push_back(iter->second);

  mforms::MenuItem *item;

  for (std::list<bec::UIForm *>::const_iterator iter = forms.begin(); iter != forms.end(); ++iter) {
    bec::UIForm *form = *iter;
    mforms::MenuBar *menu = form->get_menubar();
    if (menu) {
      item = menu->find_item("undo");
      if (item) {
        if (!undo_description.empty())
          item->set_title(strfmt(_("Undo %s"), undo_description.c_str()));
        else
          item->set_title(_("Undo"));
      }
      item = menu->find_item("redo");
      if (item) {
        if (!redo_description.empty())
          item->set_title(strfmt(_("Redo %s"), redo_description.c_str()));
        else
          item->set_title(_("Redo"));
      }
    }
  }
}

void WBContextModel::selection_changed() {
  if (!bec::GRTManager::get()->in_main_thread()) {
    bec::GRTManager::get()->run_once_when_idle(std::bind(&WBContextModel::selection_changed, this));
    return;
  }

  bec::Clipboard *clip = wb::WBContextUI::get()->get_wb()->get_clipboard();

  std::list<bec::UIForm *> forms;
  forms.push_back(_overview);
  for (std::map<std::string, ModelDiagramForm *>::const_iterator iter = _model_forms.begin();
       iter != _model_forms.end(); ++iter)
    forms.push_back(iter->second);

  mforms::MenuItem *item;

  for (std::list<bec::UIForm *>::const_iterator iter = forms.begin(); iter != forms.end(); ++iter) {
    bec::UIForm *form = *iter;
    mforms::MenuBar *menu = form->get_menubar();
    if (menu) {
      std::string target(form->get_edit_target_name());
      std::string content(clip->get_content_description());

      item = menu->find_item("copy");
      if (item) {
        if (!target.empty())
          item->set_title(strfmt(_("Copy %s"), target.c_str()));
        else
          item->set_title(_("Copy"));
      }

      item = menu->find_item("cut");
      if (item) {
        if (!target.empty())
          item->set_title(strfmt(_("Cut %s"), target.c_str()));
        else
          item->set_title(_("Cut"));
      }

      item = menu->find_item("delete");
      if (item) {
        if (!target.empty())
          item->set_title(strfmt(_("Delete %s"), target.c_str()));
        else
          item->set_title(_("Delete"));
      }

      item = menu->find_item("paste");
      if (item) {
        if (!content.empty())
          item->set_title(strfmt(_("Paste %s"), content.c_str()));
        else
          item->set_title(_("Paste"));
      }
    }
  }
  wb::WBContextUI::get()->get_command_ui()->revalidate_edit_menu_items();
}

GrtVersionRef WBContextModel::get_target_version() {
  if (get_active_model(true).is_valid()) {
    return GrtVersionRef::cast_from(bec::getModelOption(workbench_physical_ModelRef::cast_from(get_active_model(true)), "CatalogVersion"));
  }
  return GrtVersionRef();
}

#ifndef BasicExport____

void WBContextModel::export_png(const std::string &path) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());
  if (form) {
    wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
      strfmt(_("Exporting to %s..."), path.c_str()));
    try {
      form->get_view()->export_png(path, true);
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
        strfmt(_("Exported diagram image to %s"), path.c_str()));
    } catch (const std::exception &exc) {
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(_("Could not export to PNG file."));
      wb::WBContextUI::get()->get_wb()->show_exception(_("Export to PNG"), exc);
    }
  } else
    wb::WBContextUI::get()->get_wb()->show_error(
      _("Cannot Export Diagram"), _("Current diagram cannot be exported as image, please select a diagram first."));
}

void WBContextModel::export_pdf(const std::string &path) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());
  if (form) {
    Size size = form->get_view()->get_total_view_size();
    double scale = wb::WBContextUI::get()->get_wb()->get_document()->pageSettings()->scale();

    size.width = size.width / scale * 2.834;
    size.height = size.height / scale * 2.834;

    wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
      strfmt(_("Exporting full model diagram to %s..."), path.c_str()));

    try {
      form->get_view()->export_pdf(path, size);
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
        strfmt(_("Exported PDF to %s"), path.c_str()));
    } catch (const std::exception &exc) {
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(_("Could not export to PDF"));
      wb::WBContextUI::get()->get_wb()->show_exception(_("Export to PDF"), exc);
    }
  } else
    wb::WBContextUI::get()->get_wb()->show_error(
      _("Cannot Export Diagram"), _("Current diagram cannot be exported as image, please select a diagram first."));
}

void WBContextModel::export_svg(const std::string &path) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());
  if (form) {
    Size size = form->get_view()->get_total_view_size();
    double scale = wb::WBContextUI::get()->get_wb()->get_document()->pageSettings()->scale();

    size.width = MM_TO_PT(size.width / scale);
    size.height = MM_TO_PT(size.height / scale);

    wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
      strfmt(_("Exporting full model diagram to %s..."), path.c_str()));

    try {
      form->get_view()->export_svg(path, size);
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
        strfmt(_("Exported SVG to %s"), path.c_str()));
    } catch (const std::exception &exc) {
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(_("Could not export to SVG"));
      wb::WBContextUI::get()->get_wb()->show_exception(_("Export to SVG"), exc);
    }
  } else
    wb::WBContextUI::get()->get_wb()->show_error(
      _("Cannot Export Diagram"), _("Current diagram cannot be exported as image, please select a diagram first."));
}

void WBContextModel::exportPng(const model_DiagramRef &diagram, const std::string &path) {
  wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
        strfmt(_("Exporting full model diagram to %s..."), path.c_str()));
  try {
    diagram->get_data()->get_canvas_view()->export_png(path, true);
    wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
          strfmt(_("Exported diagram image to %s"), path.c_str()));
  } catch (const std::exception &exc) {
    wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(_("Could not export to PNG file."));
    wb::WBContextUI::get()->get_wb()->show_exception(_("Export to PNG"), exc);
  }
}

void WBContextModel::export_ps(const std::string &path) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());
  if (form) {
    Size size = form->get_view()->get_total_view_size();
    double scale = wb::WBContextUI::get()->get_wb()->get_document()->pageSettings()->scale();

    size.width = MM_TO_PT(size.width / scale);
    size.height = MM_TO_PT(size.height / scale);

    wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
      strfmt(_("Exporting full model diagram to %s..."), path.c_str()));

    try {
      form->get_view()->export_ps(path, size);
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(
        strfmt(_("Exported PS to %s"), path.c_str()));
    } catch (std::exception &exc) {
      wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(_("Could not export to PS file"));
      wb::WBContextUI::get()->get_wb()->show_exception(_("Export to PS File"), exc);
    }
  } else
    wb::WBContextUI::get()->get_wb()->show_error(
      _("Cannot Export Diagram"), _("Current diagram cannot be exported as image, please select a diagram first."));
}

#endif // BasicExport____

#ifndef Diagrams_and_Canvas____

//--------------------------------------------------------------------------------
// Canvas Management

void WBContextModel::add_new_diagram(const model_ModelRef &model) {
  wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(_("Creating Diagram..."));

  wb::WBContextUI::get()->get_wb()->_frontendCallbacks->lock_gui(true);
  model_DiagramRef view = model->addNewDiagram(true);
  if (view.is_valid()) {
    model->currentDiagram(view);
    view->get_data()->realize();
  }
  wb::WBContextUI::get()->get_wb()->_frontendCallbacks->lock_gui(false);
  wb::WBContextUI::get()->get_wb()->_frontendCallbacks->show_status_text(_("Diagram added."));
}

void WBContextModel::switch_diagram(const model_DiagramRef &view) {
  wb::WBContextUI::get()->get_wb()->_frontendCallbacks->switched_view(view->get_data()->get_canvas_view());
}

#endif // Diagrams_and_Canvas____
#ifndef Canvas_Objects____
/**
 ****************************************************************************
 * @brief Delete a canvas object from the canvas/model
 *
 * Will remove the figure from the model and delete all known
 * references to it. There may be some references left, which will cause
 * the GRT object and its bridge to stay alive for some more time, so the
 * bridge is marked as invalid to prevent the canvas item from appearing
 * on screen.
 *
 * @param figure to be removed
 ****************************************************************************
 */
bool WBContextModel::delete_object(model_ObjectRef object) {
  model_DiagramRef view(model_DiagramRef::cast_from(object->owner()));

  FOREACH_COMPONENT(wb::WBContextUI::get()->get_wb()->_components, iter) {
    if ((*iter)->handles_figure(object)) {
      grt::ValueRef value;
      grt::ObjectRef obj;
      if (object.is_instance(model_Figure::static_class_name())) {
        obj = (*iter)->get_object_for_figure(model_FigureRef::cast_from(object));
        value = (*iter)->get_object_for_figure(model_FigureRef::cast_from(object));
      }

      if ((*iter)->delete_model_object(object, false)) {
        notify_catalog_tree_view(NodeDelete, value);
        return true;
      }
      return false;
    }
  }
  return false;
}

bool WBContextModel::remove_figure(model_ObjectRef object) {
  model_DiagramRef view(model_DiagramRef::cast_from(object->owner()));

  FOREACH_COMPONENT(wb::WBContextUI::get()->get_wb()->_components, iter) {
    if ((*iter)->handles_figure(object)) {
      grt::ValueRef value;
      if (object.is_instance(model_Figure::static_class_name()))
        value = (*iter)->get_object_for_figure(model_FigureRef::cast_from(object));

      if ((*iter)->delete_model_object(object, true)) {
        notify_catalog_tree_view(NodeUnmark, value, view->id());
        return true;
      }
      return false;
    }
  }
  return false;
}

#endif // Canvas_Objects____

model_DiagramRef WBContextModel::get_view_with_id(const std::string &id) {
  return model_DiagramRef::cast_from(grt::GRT::get()->find_object_by_id(id, "/wb/doc"));
}

bool WBContextModel::delete_diagram(const model_DiagramRef &view) {
  grt::AutoUndo undo;
  view->owner()->diagrams().remove_value(view);
  undo.end(strfmt(_("Delete Diagram '%s'"), view->name().c_str()));

#ifdef _MSC_VER
  // in windows, a diagram is released as soon as the tab is closed. That means
  // if a user closes a diagram before deleting it, the diagram is released before
  // it's removed from the list of diagrams in the list. That will cause the overview
  // to be refreshed too early and with outdated contents. We force another explicit refresh
  // here to workaround that.
  if (wb::WBContextUI::get()->get_physical_overview())
    wb::WBContextUI::get()->get_physical_overview()->send_refresh_diagram(model_DiagramRef());
#endif
  return true;
}

void WBContextModel::begin_plugin_exec() {
  // lock the canvas so that it doesn't keep refreshing all the time.
  // XXX we have to find some way to allow plugins control refresh freezing
  ModelDiagramForm *view = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());
  _locked_view_for_plugin_exec = 0;

  if (view) {
    _locked_view_for_plugin_exec = view->get_view();
    _locked_view_for_plugin_exec->lock_redraw();
  }
}

void WBContextModel::end_plugin_exec() {
  // form can get destroyed after a plugin is executed
  if (_locked_view_for_plugin_exec && get_diagram_form(_locked_view_for_plugin_exec))
    _locked_view_for_plugin_exec->unlock_redraw();
  _locked_view_for_plugin_exec = 0;
}

/**
 * Called by the front end when the user data type editor has been closed. We can then remove our reference to it.
 */
static void userTypeEditorClosed(UserDefinedTypeEditor **editor_ptr) {
  *editor_ptr = NULL;
}

void WBContextModel::show_user_type_editor(workbench_physical_ModelRef model) {
  if (_current_user_type_editor == NULL) {
    _current_user_type_editor = new UserDefinedTypeEditor(model);
    scoped_connect(_current_user_type_editor->signal_closed(),
                   std::bind(userTypeEditorClosed, &_current_user_type_editor));
  }
  _current_user_type_editor->show_modal(NULL, NULL);
}

mforms::View *WBContextModel::shared_secondary_sidebar() {
  return _secondary_sidebar;
}

//--------------------------------------------------------------------------------------------------

static struct RegisterNotifDocs_wb_context_model {
  RegisterNotifDocs_wb_context_model() {
    base::NotificationCenter::get()->register_notification(
      "GRNModelOpened", "modeling", "Sent when a model document finishes loading.", "ui.ModelPanel instance", "");

    base::NotificationCenter::get()->register_notification(
      "GRNModelCreated", "modeling", "Sent when a new model document is created.", "ui.ModelPanel instance", "");

    base::NotificationCenter::get()->register_notification(
      "GRNModelClosed", "modeling", "Sent when a model document is closed.", "ui.ModelPanel instance", "");
  }
} initdocs_wb_context_model;
