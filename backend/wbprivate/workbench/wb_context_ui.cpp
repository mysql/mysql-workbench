/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <string>

#include "base/ui_form.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/log.h"
#include "base/notifications.h"

#include "grt.h"

#include "grts/structs.h"
#include "grts/structs.app.h"

#include "grt/editor_base.h"

#include "wb_context_ui.h"
#include "wb_context.h"
#include "model/wb_component.h"
#include "model/wb_component_physical.h"
#include "model/wb_context_model.h"
#include "model/wb_model_diagram_form.h"
#include "sqlide/wb_context_sqlide.h"

#include "wb_overview.h"
#include "model/wb_overview_physical.h"
#include "model/wb_diagram_options.h"

#include "grt/clipboard.h"
#include "grtui/gui_plugin_base.h"

#include "grt_shell_window.h"
#include "license_view.h"

#include "mforms/appview.h"

#include "mforms/home_screen.h"
#include "mforms/home_screen_connections.h"
#include "mforms/home_screen_documents.h"

#include "wb_command_ui.h"

#include "plugin_install_window.h"

using namespace wb;
using namespace bec;
using namespace base;

DEFAULT_LOG_DOMAIN(DOMAIN_WB_CONTEXT_UI)

//--------------------------------------------------------------------------------------------------

std::shared_ptr<WBContextUI> WBContextUI::get() {
  static std::shared_ptr<WBContextUI> _singleton(new WBContextUI());
  return _singleton;
}

//--------------------------------------------------------------------------------------------------

WBContextUI::WBContextUI() : _wb(new WBContext(false)), _command_ui(new CommandUI(_wb)) {
  _shell_window = 0;
  _active_form = 0;
  _active_main_form = 0;

  _addon_download_window = 0;
  _plugin_install_window = 0;

  _last_unsaved_changes_state = false;
  _quitting = false;
  _processing_action_open_connection = false;

  _home_screen = nullptr;

  // to notify that the save status of the doc has changed
  scoped_connect(grt::GRT::get()->get_undo_manager()->signal_changed(), std::bind(&WBContextUI::history_changed, this));

  // stuff to do when the active form is switched in the UI (through set_active_form)
  _form_change_signal.connect(std::bind(&WBContextUI::form_changed, this));
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::cleanUp() {
  if (_wb != nullptr) {
    _wb->do_close_document(true);
  }
  delete _addon_download_window;
  _addon_download_window = nullptr;

  delete _plugin_install_window;
  _plugin_install_window = nullptr;

  delete _shell_window;
  _shell_window = nullptr;

  if (_wb != nullptr && !_wb->cancel_idle_tasks()) {
    // Idle tasks are currently being executed. Wait a moment and then try again.
    g_usleep(static_cast<int>(100000));
    _wb->cancel_idle_tasks();
  }
  
  delete _wb;
  _wb = nullptr;

  delete _command_ui;
  _command_ui = nullptr;


  // home screen should be released by undock operation,
  // but it will be left in bad state, we have to clear this
  _home_screen = nullptr;
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::reinit() {
  if (_wb == nullptr) {
    _wb = new WBContext(false);
    _command_ui = new CommandUI(_wb);
  }

}
//--------------------------------------------------------------------------------------------------

WBContextUI::~WBContextUI() {
  cleanUp();
}

//--------------------------------------------------------------------------------------------------

bool WBContextUI::init(WBFrontendCallbacks *callbacks, WBOptions *options) {
  // Log set folders.
  logInfo(
    "Initializing workbench context UI with these values:\n"
    "\tbase dir: %s\n\tplugin path: %s\n\tstruct path: %s\n\tmodule path: %s\n\t"
    "library path: %s\n\tuser data dir: %s\n\topen at start: %s\n\topen type: %s\n\trun at startup: %s\n\t"
    "run type: %s\n\tForce SW rendering: %s\n\tForce OpenGL: %s\n\tquit when done: %s\n",
    options->basedir.c_str(), options->plugin_search_path.c_str(), options->struct_search_path.c_str(),
    options->module_search_path.c_str(), options->library_search_path.c_str(), options->user_data_dir.c_str(),
    options->open_at_startup.c_str(), options->open_at_startup_type.c_str(), options->run_at_startup.c_str(),
    options->run_language.c_str(), options->force_sw_rendering ? "Yes" : "No",
    options->force_opengl_rendering ? "Yes" : "No", options->quit_when_done ? "Yes" : "No");

  bool flag = false;
  try {
    // this needs to be created after the search paths have been set
    // WBContext::init_ will call get_shell_window() at the right time, which
    // will trigger the instantiation
    //_shell_window = new GRTShellWindow(_wb->get_grt_manager());
    flag = _wb->init_(callbacks, options);

    if (!options->testing) {
      // has to be called after WBContext is initialized
      add_backend_builtin_commands();

      // look for auto-save files (must be done at startup to not confuse with autosaves created by ourself)
      WBContextModel::detect_auto_save_files(_wb->get_auto_save_dir());
      WBContextSQLIDE::detect_auto_save_files(_wb->get_auto_save_dir());
    }

  } catch (const std::exception &e) {
    logError("WBContextUI::init, exception '%s'\n", e.what()); // log_error logs to stderr too in debug mode.
  } catch (...) {
    logError("Some exception has happened. It was caught at WBContextUI::init.\n");
  }

  return flag;
}

//--------------------------------------------------------------------------------------------------

GRTShellWindow *WBContextUI::get_shell_window() {
  if (!_shell_window)
    _shell_window = new GRTShellWindow(_wb);
  return _shell_window;
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::init_finish(WBOptions *options) {
  g_assert(_wb->get_root().is_valid());
  show_home_screen();
  _wb->init_finish_(options);

  NotificationCenter::get()->send("GNAppStarted", nullptr);
}

void WBContextUI::finalize() {
  _wb->finalize();
  _command_ui->clearBuildInCommands();
  if (_home_screen != nullptr)
    mforms::App::get()->undock_view(_home_screen);
}

bool WBContextUI::request_quit() {
  if (_quitting)
    return true;

  if (!bec::GRTManager::get()->in_main_thread())
    logWarning("request_quit() called in worker thread\n");

  {
    NotificationInfo info;
    info["cancel"] = "0";

    NotificationCenter::get()->send("GNAppShouldClose", nullptr, info);

    if (info["cancel"] != "0")
      return false;
  }

  if (!_wb->can_close_document())
    return false;

  if (_wb->get_sqlide_context() && !_wb->get_sqlide_context()->request_quit())
    return false;

  if (_shell_window != nullptr && !_shell_window->request_quit())
    return false;

  return true;
}

void WBContextUI::perform_quit() {
  _quitting = true;
  _wb->do_close_document(true);
  _wb->_frontendCallbacks->quit_application();
}

void WBContextUI::reset() {
  if (!dynamic_cast<OverviewBE *>(_active_form))
    _active_form = 0;
  if (!dynamic_cast<OverviewBE *>(_active_main_form))
    _active_main_form = 0;

  scoped_connect(get_physical_overview()->signal_selection_changed(),
                 std::bind(&WBContextUI::overview_selection_changed, this));

  get_physical_overview()->set_model(_wb->get_document()->physicalModels()[0]);

  _wb->request_refresh(RefreshSelection, "", 0);

  get_physical_overview()->send_refresh_children(bec::NodeId());

  _wb->get_model_context()->refill_catalog_tree();
}

void WBContextUI::history_changed() {
  if (!_wb->_file) // check if model is still opened, if not, leave
    return;

  if (_wb->has_unsaved_changes() != _last_unsaved_changes_state)
    _wb->request_refresh(RefreshDocument, "", (NativeHandle)0);

  bec::GRTManager::get()->run_once_when_idle(std::bind(&CommandUI::revalidate_edit_menu_items, get_command_ui()));

  _last_unsaved_changes_state = _wb->has_unsaved_changes();
}

void WBContextUI::update_current_diagram(bec::UIForm *form) {
  ModelDiagramForm *dform = dynamic_cast<ModelDiagramForm *>(form);
  if (dform) {
    model_DiagramRef diagram(dform->get_model_diagram());
    if (diagram.is_valid() && diagram->owner().is_valid())
      diagram->owner()->currentDiagram(diagram);
  }
}

void WBContextUI::overview_selection_changed() {
  if (get_active_main_form() == get_physical_overview()) {
    _wb->request_refresh(RefreshSelection, "", (NativeHandle)get_physical_overview()->get_frontend_data());
    get_command_ui()->revalidate_edit_menu_items();
  }
}

void WBContextUI::load_app_options(bool update) {
  if (!update)
    _command_ui->load_data();
}

static void add_script_file(WBContextUI *wbui) {
  std::string file = wbui->get_wb()->_frontendCallbacks->show_file_dialog("open", _("Add SQL Script File"), "sql");
  if (!file.empty()) {
    workbench_physical_ModelRef model;

    model = workbench_physical_ModelRef::cast_from(wbui->get_wb()->get_model_context()->get_active_model(false));
    if (model.is_valid())
      wbui->get_wb()->get_component<WBComponentPhysical>()->add_new_stored_script(model, file);
  }
}

static void add_note_file(WBContextUI *wbui) {
  std::string file =
    wbui->get_wb()->_frontendCallbacks->show_file_dialog("open", _("Add Note File"), "Text Files (*.txt)|*.txt");
  if (!file.empty()) {
    workbench_physical_ModelRef model;

    model = workbench_physical_ModelRef::cast_from(wbui->get_wb()->get_model_context()->get_active_model(false));
    if (model.is_valid())
      wbui->get_wb()->get_component<WBComponentPhysical>()->add_new_stored_note(model, file);
  }
}

/** builtin: commands for use in menus and toolbars that are handled by ourselves
  */
void WBContextUI::add_backend_builtin_commands() {
  _command_ui->add_builtin_command("show_about", std::bind(&WBContextUI::show_about, this));
  _command_ui->add_builtin_command("overview.home", std::bind(&WBContextUI::show_home_screen, this));

  _command_ui->add_builtin_command("add_script_file", std::bind(add_script_file, this));
  _command_ui->add_builtin_command("add_note_file", std::bind(add_note_file, this));
  _command_ui->add_builtin_command("web_mysql_home",
                                   std::bind(&WBContextUI::show_web_page, this, "http://mysql.com/", true));
  _command_ui->add_builtin_command(
      "web_mysql_docs", std::bind(&WBContextUI::show_web_page, this, "https://dev.mysql.com/doc/workbench/en/", true));

  _command_ui->add_builtin_command("web_mysql_blog",
                                   std::bind(&WBContextUI::show_web_page, this, "https://mysqlworkbench.org/", true));

  _command_ui->add_builtin_command(
      "web_mysql_forum", std::bind(&WBContextUI::show_web_page, this, "https://forums.mysql.com/list.php?152", true));

  if (_wb->is_commercial()) {
    _command_ui->add_builtin_command("web_support",
                                     std::bind(&WBContextUI::show_web_page, this, "http://support.oracle.com", true));
  }

  _command_ui->add_builtin_command("help_index", std::bind(&WBContextUI::show_help_index, this));
  _command_ui->add_builtin_command("show-license", std::bind(&WBContextUI::showLicense, this));
  _command_ui->add_builtin_command("locate_log_file", std::bind(&WBContextUI::locate_log_file, this));
  _command_ui->add_builtin_command("show_log_file", std::bind(&WBContextUI::show_log_file, this));
}

#ifndef ___specialforms

//--------------------------------------------------------------------------------------------------

PhysicalOverviewBE *WBContextUI::get_physical_overview() {
  return get_wb()->get_model_context() ? get_wb()->get_model_context()->get_overview() : 0;
}

//--------------------------------------------------------------------------------------------------

/*
 * Opens the given web page in the system's default browser.
 */
void WBContextUI::show_web_page(const std::string &url, bool internal_browser) {
  mforms::Utilities::open_url(url);
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::showLicense() {
  LicenseView *view = mforms::manage(new LicenseView(this));
  mforms::App::get()->dock_view(view, "maintab");
  view->set_title(_("License Info"));
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::show_help_index() {
  GUILock lock(_wb, _("Starting Doc Lib"), _("The MySQL Doc Library is opening currently, "
                                             "which should be finished in a moment .\n\nPlease stand by..."));

  mforms::Utilities::open_url("http://dev.mysql.com/doc/refman/8.0/en");
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::locate_log_file() {
  if (!base::Logger::log_dir().empty())
    mforms::Utilities::open_url(base::Logger::log_dir());
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::show_log_file() {
  if (!base::Logger::log_filename().empty())
    mforms::Utilities::open_url(base::Logger::log_filename());
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::activate_figure(const grt::ValueRef &value) {
  ModelDiagramForm *form = 0;
  if (model_FigureRef::can_wrap(value)) {
    model_FigureRef figure(model_FigureRef::cast_from(value));
    form = get_wb()->get_model_context()->get_diagram_form_for_diagram_id(figure->owner().id());
    if (form)
      form->focus_and_make_visible(figure, true);
  } else if (model_ConnectionRef::can_wrap(value)) {
    model_ConnectionRef conn(model_ConnectionRef::cast_from(value));
    ModelDiagramForm *form = get_wb()->get_model_context()->get_diagram_form_for_diagram_id(conn->owner().id());
    if (form)
      form->focus_and_make_visible(conn, true);
  } else if (model_LayerRef::can_wrap(value)) {
    model_LayerRef layer(model_LayerRef::cast_from(value));
    ModelDiagramForm *form = get_wb()->get_model_context()->get_diagram_form_for_diagram_id(layer->owner().id());
    if (form)
      form->focus_and_make_visible(layer, true);
  }
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::form_changed() {
  _wb->request_refresh(RefreshZoom, "", (NativeHandle)0);

  bec::UIForm *form = get_active_main_form();
  if (form && form->get_menubar())
    get_command_ui()->revalidate_menu_bar(form->get_menubar());
}

bec::ValueInspectorBE *WBContextUI::create_inspector_for_selection(bec::UIForm *form, std::vector<std::string> &items) {
  grt::ListRef<model_Object> selection;

  // grt::ListRef<model_Object> selection(form->get_selection());

  if (dynamic_cast<ModelDiagramForm *>(form))
    selection = dynamic_cast<ModelDiagramForm *>(form)->get_selection();
  else
    return 0;

  if (selection.is_valid() && selection.count() > 0) {
    if (selection.count() == 1) {
      items.push_back(
        strfmt("%s: %s", selection[0]->name().c_str(), selection[0].get_metaclass()->get_attribute("caption").c_str()));

      return ValueInspectorBE::create(selection[0], false, true);
    } else {
      std::vector<grt::ObjectRef> list;

      items.push_back(_("Multiple Items"));
      for (size_t c = selection.count(), i = 0; i < c; i++) {
        items.push_back(strfmt("%s: %s", selection[i]->name().c_str(),
                               selection[i].get_metaclass()->get_attribute("caption").c_str()));
        list.push_back(selection.get(i));
      }

      return ValueInspectorBE::create(list);
    }
  }

  return 0;
}

bec::ValueInspectorBE *WBContextUI::create_inspector_for_selection(std::vector<std::string> &items) {
  std::string res;

  grt::ListRef<GrtObject> selection(get_physical_overview()->get_selection());
  std::string name_mem_name("name");

  if (selection.is_valid() && selection.count() > 0) {
    if (selection.count() == 1) {
      GrtObjectRef obj = selection[0];
      if (obj.is_valid() && obj->has_member(name_mem_name)) {
        items.push_back(strfmt("%s: %s", obj.get_string_member(name_mem_name).c_str(),
                               obj.get_metaclass()->get_attribute("caption").c_str()));

        return ValueInspectorBE::create(selection[0], false, true);
      }
    } else {
      std::vector<grt::ObjectRef> list;

      items.push_back(_("Multiple Items"));
      for (size_t c = selection.count(), i = 0; i < c; i++) {
        if (!selection[i].is_valid()) // skip "Add item" entry in objects list
          continue;
        items.push_back(strfmt("%s: %s", selection[i].get_string_member(name_mem_name).c_str(),
                               selection[i].get_metaclass()->get_attribute("caption").c_str()));
        list.push_back(selection.get(i));
      }

      return ValueInspectorBE::create(list);
    }
  }

  return 0;
}

std::string WBContextUI::get_description_for_selection(grt::ListRef<GrtObject> &activeObjList,
                                                       std::vector<std::string> &items) {
  std::string res;

  if (get_physical_overview() != nullptr) {
    grt::ListRef<GrtObject> selection(get_physical_overview()->get_selection());
    activeObjList = selection;

    std::string comment_mem_name("comment");
    std::string name_mem_name("name");

    if (selection.is_valid() && selection.count() > 0) {
      if (selection.count() == 1) {
        GrtObjectRef obj(selection[0]);
        if (obj.is_valid() && obj.has_member(comment_mem_name) && obj.has_member(name_mem_name)) {
          items.push_back(strfmt("%s: %s", obj->name().c_str(), obj.get_metaclass()->get_attribute("caption").c_str()));
          res = obj.get_string_member(comment_mem_name);
        }
      } else {
        items.push_back(_("Multiple Items"));
        for (size_t c = selection.count(), i = 0; i < c; i++) {
          GrtObjectRef obj(selection[i]);
          if (obj.is_valid() && obj.has_member(comment_mem_name) && obj.has_member(name_mem_name)) {
            items.push_back(
              strfmt("%s: %s", obj->name().c_str(), obj.get_metaclass()->get_attribute("caption").c_str()));
            std::string comment = obj.get_string_member(comment_mem_name);
            if (0 == i)
              res = comment;
            else if (0 != res.compare(comment))
              res =
                "<Multiple Items>\nThat means not all selected items have same comment.\nBeware applying changes will "
                "override comments for all selected objects.";
          }
        }
      }
    }
  }
  return res;
}

std::string WBContextUI::get_description_for_selection(bec::UIForm *form, grt::ListRef<GrtObject> &activeObjList,
                                                       std::vector<std::string> &items) {
  grt::ListRef<model_Object> selection;

  if (dynamic_cast<ModelDiagramForm *>(form))
    selection = dynamic_cast<ModelDiagramForm *>(form)->get_selection();
  else
    return get_description_for_selection(activeObjList, items);

  std::string res;

  activeObjList = grt::ListRef<model_Object>(true);

  std::string comment_mem_name("comment");
  std::string descr_mem_name("description");

  if (selection.is_valid() && selection.count() > 0) {
    bool first = true;
    for (size_t c = selection.count(), i = 0; i < c; i++) {
      model_ObjectRef figure(selection[i]);
      WBComponent *comp = _wb->get_component_handling(figure);
      GrtObjectRef dbobject;
      if (comp)
        dbobject = comp->get_object_for_figure(figure);

      if (dbobject.is_valid() && dbobject.has_member(comment_mem_name)) {
        activeObjList.insert(dbobject);

        items.push_back(
          strfmt("%s: %s", figure->name().c_str(), figure->get_metaclass()->get_attribute("caption").c_str()));
        std::string comment = dbobject.get_string_member(comment_mem_name);
        if (first)
          res = comment;
        else if (0 != res.compare(comment))
          res =
            _("<Multiple Items>\nThat means not all selected items have same comment.\nBeware applying changes will "
              "override comments for all selected objects.");

        first = false;
      } else if (!dbobject.is_valid() && figure.is_valid() && figure.has_member(descr_mem_name)) {
        activeObjList.insert(figure);

        items.push_back(
          strfmt("%s: %s", figure->name().c_str(), figure->get_metaclass()->get_attribute("caption").c_str()));
        std::string comment = figure.get_string_member(descr_mem_name);
        if (first)
          res = comment;
        else if (0 != res.compare(comment))
          res =
            _("<Multiple Items>\nThat means not all selected items have same comment.\nBeware applying changes will "
              "override comments for all selected objects.");

        first = false;
      }
    }
    if (items.size() > 1)
      items.insert(items.begin(), _("Multiple Items"));
  }

  return res;
}

void WBContextUI::set_description_for_selection(const grt::ListRef<GrtObject> &objList, const std::string &val) {
  if (objList.is_valid() && objList.count() > 0) {
    std::string comment_mem_name("comment");
    std::string descr_mem_name("description");

    grt::AutoUndo undo;

    for (size_t c = objList.count(), i = 0; i < c; i++) {
      GrtObjectRef obj(objList[i]);
      if (obj.is_valid()) {
        if (obj.has_member(comment_mem_name)) {
          obj.set_member(comment_mem_name, grt::StringRef(val));
          get_physical_overview()->send_refresh_for_schema_object(obj, true);
        } else if (obj.has_member(descr_mem_name)) {
          obj.set_member(descr_mem_name, grt::StringRef(val));
          get_physical_overview()->send_refresh_for_schema_object(obj, true);
        }
      }
    }

    undo.end(_("Set Object Description"));
  }
}

DiagramOptionsBE *WBContextUI::create_diagram_options_be(mdc::CanvasView *view) {
  model_DiagramRef model_diagram(get_wb()->get_model_context()->get_active_model_diagram(true));

  if (model_diagram.is_valid())
    return new DiagramOptionsBE(view, model_diagram, _wb);
  else
    return 0;
}

std::string WBContextUI::get_active_diagram_info() {
  wb::ModelDiagramForm *form = dynamic_cast<wb::ModelDiagramForm *>(get_active_main_form());

  if (form)
    return form->get_diagram_info_text();

  return "";
}

#endif // ___specialforms

#ifndef ___preferences
//-----------------------------------------------------------------------------------
// utility functions for user preferences

void WBContextUI::get_doc_properties(std::string &caption, std::string &version, std::string &author,
                                     std::string &project, std::string &date_created, std::string &date_changed,
                                     std::string &description) {
  app_DocumentInfoRef info = _wb->get_document()->info();

  caption = info->caption();
  version = info->version();
  author = info->author();
  project = info->project();
  date_created = info->dateCreated();
  date_changed = info->dateChanged();
  description = info->description();
}

void WBContextUI::set_doc_properties(const std::string &caption, const std::string &version, const std::string &author,
                                     const std::string &project, const std::string &date_created,
                                     const std::string &date_changed, const std::string &description) {
  app_DocumentInfoRef info = _wb->get_document()->info();

  grt::AutoUndo undo;
  info->caption(caption);
  info->version(version);
  info->author(author);
  info->project(project);
  info->dateCreated(date_created);
  info->dateChanged(date_changed);
  info->description(description);
  undo.end("Change document properties");
}

std::list<WBPaperSize> WBContextUI::get_paper_sizes(bool descr_in_inches) {
  std::list<WBPaperSize> sizes;

  grt::ListRef<app_PaperType> types(_wb->get_root()->options()->paperTypes());

  for (size_t c = types.count(), i = 0; i < c; i++) {
    WBPaperSize size;
    size.name = types[i]->name();
    size.caption = types[i]->caption();
    size.width = types[i]->width();
    size.height = types[i]->height();
    size.margins_set = types[i]->marginsSet() != 0;
    size.margin_top = types[i]->marginTop();
    size.margin_bottom = types[i]->marginBottom();
    size.margin_left = types[i]->marginLeft();
    size.margin_right = types[i]->marginRight();

    if (descr_in_inches)
      size.description = strfmt("%.2f in x %.2f in", size.width * 0.03937, size.height * 0.03937);
    else
      size.description = strfmt("%.2f cm x %.2f cm", size.width / 10, size.height / 10);

    sizes.push_back(size);
  }

  return sizes;
}

bool WBContextUI::add_paper_size(const std::string &name, double width, double height, bool margins, double margin_top,
                                 double margin_bottom, double margin_left, double margin_right) {
  if (grt::find_named_object_in_list(_wb->get_root()->options()->paperTypes(), name).is_valid())
    return false;

  app_PaperTypeRef type(grt::Initialized);
  type->owner(_wb->get_root()->options());
  type->name(name);
  type->width(width);
  type->height(height);
  type->marginsSet(margins ? 1 : 0);
  type->marginTop(margin_top);
  type->marginBottom(margin_bottom);
  type->marginLeft(margin_left);
  type->marginRight(margin_right);
  _wb->get_root()->options()->paperTypes().insert(type);

  return true;
}

app_PageSettingsRef WBContextUI::get_page_settings() {
  if (_wb->get_document().is_valid())
    return _wb->get_document()->pageSettings();
  else {
    // XXX add proper initialization for non-trivial types in structs.app.h too.
    app_PageSettingsRef settings = app_PageSettingsRef(grt::Initialized);
    settings->scale(1);
    settings->paperType(app_PaperTypeRef());

    return settings;
  }
}

grt::DictRef WBContextUI::get_model_options(const std::string &model_id) {
  grt::ListRef<workbench_physical_Model> pmodels(_wb->get_document()->physicalModels());

  for (size_t c = pmodels.count(), i = 0; i < c; i++) {
    if (pmodels.get(i).id() == model_id) {
      return pmodels.get(i)->options();
    }
  }
  return grt::DictRef();
}

std::vector<std::string> WBContextUI::get_wb_options_keys(const std::string &model) {
  std::vector<std::string> keylist;
  grt::DictRef options = _wb->get_wb_options();

  for (grt::DictRef::const_iterator iter = options.begin(); iter != options.end(); ++iter) {
    keylist.push_back(iter->first);
  }

  return keylist;
}

bool WBContextUI::get_wb_options_value(const std::string &model, const std::string &key, std::string &value) {
  grt::DictRef options = _wb->get_wb_options();
  grt::ValueRef val;

  // If a model is given check if it is set to use global values or its own.
  if (!model.empty()) {
    grt::DictRef model_options(get_model_options(model));
    bool use_global = model_options.get_int("useglobal", 1) != 0;

    if (key == "useglobal") {
      if (use_global)
        value = "1";
      else
        value = "0";
      return true;
    }

    if (!use_global && model_options.has_key(key))
      val = model_options.get(key);
  }

  if (!val.is_valid() && options.has_key(key))
    val = options.get(key);

  switch (val.type()) {
    case grt::StringType:
    case grt::DoubleType:
    case grt::IntegerType:
      value = val.toString();
      return true;
    default:
      return false;
  }
}

void WBContextUI::set_wb_options_value(const std::string &model, const std::string &key, const std::string &value,
                                       const grt::Type default_type) {
  grt::DictRef options;
  grt::Type type;
  if (_wb->get_wb_options().has_key(key))
    type = _wb->get_wb_options().get(key).type();
  else
    type = default_type;

  if (!model.empty()) {
    options = get_model_options(model);

    options.gset("useglobal", 0);

    if (options.has_key(key))
      type = options.get(key).type();
  }

  if (!options.is_valid())
    options = _wb->get_wb_options();

  switch (type) {
    case grt::DoubleType: {
      grt::DoubleRef v(base::atof<double>(value, 0.0));
      if (!options.has_key(key) || options.get_double(key) != *v)
        options.set(key, v);
      break;
    }
    case grt::IntegerType: {
      grt::IntegerRef v(base::atoi<int>(value, 0));
      if (!options.has_key(key) || options.get_int(key) != *v)
        options.set(key, v);
      break;
    }
    case grt::StringType: {
      grt::StringRef v(value);
      if (!options.has_key(key) || options.get_string(key) != *v)
        options.set(key, v);
      break;
    }
    default:
      throw std::runtime_error("No valid grt type specified when setting options value.");
  }
}

void WBContextUI::discard_wb_model_options(const std::string &model) {
  grt::DictRef opts = get_model_options(model);
  if (opts.is_valid()) {
    for (grt::DictRef::const_iterator item = opts.begin(); item != opts.end(); ++item) {
      opts.set(item->first, grt::ValueRef());
    }
    opts.gset("useglobal", 1);
  }
}

#endif // ___preferences

#ifndef ___forms

//--------------------------------------------------------------------------------
// Form Management

void *WBContextUI::form_destroyed(void *data) {
  UIForm *form = reinterpret_cast<UIForm *>(data);
  WBContextUI *wb = reinterpret_cast<WBContextUI *>(form->get_owner_data());

  if (wb->_active_form == form)
    wb->set_active_form(0);
  if (wb->_active_main_form == form)
    wb->_active_main_form = 0;

  return 0;
}

std::string WBContextUI::get_active_context(bool main_context) {
  bec::UIForm *form = main_context ? get_active_main_form() : get_active_form();

  if (form)
    return form->get_form_context_name();

  return "";
}

void WBContextUI::set_active_form(bec::UIForm *form) {
  if (_active_form == form)
    return;
  // register callbacks to form if needed

  if (_active_form && form)
    _active_form->remove_destroy_notify_callback(_active_form);
  _active_form = form;
  if (form) {
    form->add_destroy_notify_callback(reinterpret_cast<void *>(form), &WBContextUI::form_destroyed);
    form->set_owner_data(reinterpret_cast<void *>(this));
  }

  if (form && form->is_main_form()) {
    if (_active_main_form != form) {
      _active_main_form = form;

      base::NotificationInfo info;
      info["form"] = form ? form->form_id() : "";
      info["context"] = get_active_context(true);
      NotificationCenter::get()->send("GNMainFormChanged", 0, info);
    }
  }

  _form_change_signal(form);
}

bec::UIForm *WBContextUI::get_active_main_form() {
  return _active_main_form;
}

bec::UIForm *WBContextUI::get_active_form() {
  return _active_form;
}

#endif // ___forms

#ifndef ___others

//-----------------------------------------------------------------------------------
// other functionality for UI

std::string WBContextUI::get_document_name() {
  if (_wb->get_filename().empty())
    return "Untitled";
  else
    return base::basename(_wb->get_filename());
}

std::string WBContextUI::get_title() {
  if (_wb->get_model_context()) {
#ifndef __APPLE__
    if (_wb->has_unsaved_changes())
      return get_document_name() + "* - MySQL Workbench";
    else
#endif
      return get_document_name() + " - MySQL Workbench";
  } else
    return "MySQL Workbench";
}

#endif // ___others

void WBContextUI::start_plugin_net_install(const std::string &url) {
  if (!_addon_download_window)
    _addon_download_window = new AddOnDownloadWindow(this);
  _addon_download_window->install_addon_from_url(url);
}

bool WBContextUI::start_plugin_install(const std::string &path) {
  if (!_plugin_install_window)
    _plugin_install_window = new PluginInstallWindow(this);
  return _plugin_install_window->install_plugin(path);
}

//--------------------------------------------------------------------------------------------------

static struct RegisterNotifDocs_wb_context_ui {
  RegisterNotifDocs_wb_context_ui() {
    base::NotificationCenter::get()->register_notification(
      "GNAppStarted", "application", "Sent when Workbench starts up and finishes with various initialization routines.",
      "", "");

    base::NotificationCenter::get()->register_notification("GNAppShouldClose", "application",
                                                           "Sent when the user requests Workbench to close. Close can "
                                                           "be cancelled by setting the 'cancel' field in the info "
                                                           "dict to 1.",
                                                           "", "cancel - set to 1 if exit should be cancelled");

    base::NotificationCenter::get()->register_notification(
      "GNMainFormChanged", "application", "Sent when the main tab from the application is switched.", "",
      "form - the ID of the newly active form\n"
      "context - the context name of the newly active form");

    base::NotificationCenter::get()->register_notification("GNApplicationActivated", "application",
                                                           "Sent when the application was activated.", "", "");
    base::NotificationCenter::get()->register_notification("GNApplicationDeactivated", "application",
                                                           "Sent when the application lost the active status.", "", "");
  }
} initdocs_wb_context_ui;
