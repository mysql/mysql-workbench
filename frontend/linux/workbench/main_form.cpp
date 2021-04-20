/*
 * Copyright (c) 2008, 2021, Oracle and/or its affiliates.
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

#include "gtk/lf_mforms.h"
#include "grid_view.h"
#include <gtkmm/statusbar.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/fixed.h>
#include <gtkmm/main.h>
#include <atkmm.h>
#include "main_form.h"
#include "active_label.h"
// the rest, backend, etc ...
#include "workbench/wb_context_ui.h"
#include "workbench/wb_context.h"
#include "model/wb_model_diagram_form.h"
#include "workbench/wb_overview.h"
#include "model/wb_context_model.h"
#include "model/wb_overview_physical.h"
#include "plugin_editor_base.h"
#include "form_view_base.h"

#include "diagram_size_form.h"

#include "mdc.h"

#include "gtk_helpers.h"

#include "model_panel.h"
#include "model_diagram_panel.h"
#include "overview_panel.h"
#include "mforms/find_panel.h"

#include "gtk_helpers.h"
#include "image_cache.h"

#include "base/string_utilities.h"
#include "base/geometry.h"
#include "base/drawing.h"
#include "mforms/../gtk/lf_menubar.h"
#include "mforms/../gtk/lf_toolbar.h"
#include "mforms/../gtk/lf_form.h"

#include <base/log.h>
#include "main_app.h"

using base::strfmt;

static void set_window_icons(Gtk::Window *window) {
  std::vector<Glib::RefPtr<Gdk::Pixbuf> > icons;

  icons.push_back(ImageCache::get_instance()->image_from_filename("MySQLWorkbench-16.png", false));
  icons.push_back(ImageCache::get_instance()->image_from_filename("MySQLWorkbench-32.png", false));
  icons.push_back(ImageCache::get_instance()->image_from_filename("MySQLWorkbench-48.png", false));
  icons.push_back(ImageCache::get_instance()->image_from_filename("MySQLWorkbench-128.png", false));

  window->set_default_icon_list(icons);
}

static Gdk::Color _sys_selection_color;

const double defaultDpi = 96;
//------------------------------------------------------------------------------
// @ctx is passed from Program class, see Program::Program
MainForm::MainForm() : _exiting(false) {
  setup_mforms_app();


  _ui = Gtk::Builder::create_from_file(bec::GRTManager::get()->get_data_file_path("wb.glade"));
  set_name("Main Tab Bar");

  get_mainwindow()->signal_delete_event().connect(sigc::mem_fun(this, &MainForm::close_window));
  get_mainwindow()->signal_set_focus().connect(sigc::mem_fun(this, &MainForm::on_focus_widget));
  get_mainwindow()->signal_configure_event().connect_notify(sigc::mem_fun(this, &MainForm::on_configure_window));
  get_mainwindow()->signal_window_state_event().connect_notify(sigc::mem_fun(this, &MainForm::on_window_state));
  get_mainwindow()->property_is_active().signal_changed().connect(sigc::mem_fun(this, &MainForm::is_active_changed));
  get_mainwindow()->set_events(Gdk::FOCUS_CHANGE_MASK);
  get_mainwindow()->signal_focus_in_event().connect(
    sigc::bind_return(sigc::hide(sigc::mem_fun(mforms::Form::main_form(), &mforms::Form::activated)), false));
  get_mainwindow()->signal_focus_out_event().connect(
    sigc::bind_return(sigc::hide(sigc::mem_fun(mforms::Form::main_form(), &mforms::Form::deactivated)), false));
  get_mainwindow()->set_title("MySQL Workbench");

  _model_panel = nullptr;

  get_upper_note()->signal_switch_page().connect(sigc::mem_fun(*this, &MainForm::switch_page));

  Gtk::Statusbar *status = 0;
  _ui->get_widget("statusbar1", status);
  if (status) {
    status->pack_end(_progress_bar, false, false);
    _progress_bar.show();
  }
  _gui_locked = false;

  Gdk::Screen::get_default()->property_resolution().signal_changed().connect([this]{
	  base::NotificationCenter::get()->send("GNBackingScaleChanged", nullptr);
  });

  get_mainwindow()->property_scale_factor().signal_changed().connect([this]{
	  base::NotificationCenter::get()->send("GNBackingScaleChanged", nullptr);
  });

  //  switch_page(0, 0);
  mforms::gtk::FormImpl::init_main_form(this->get_mainwindow());

  base::NotificationCenter::get()->add_observer(this, "GNFormTitleDidChange");
  base::NotificationCenter::get()->add_observer(this, "GNFocusChanged");
}
//------------------------------------------------------------------------------
MainForm::~MainForm() {
  _sig_flush_idle.disconnect();
  _sig_set_current_page.disconnect();
  _sig_close_tab.disconnect();
  _sig_change_status.disconnect();

  base::NotificationCenter::get()->remove_observer(this);

  const size_t n_slots = _slots.size();
  for (size_t i = 0; i < n_slots; ++i)
    _slots[i].disconnect();
  _slots.clear();

  std::map<mdc::CanvasView *, ModelDiagramPanel *>::iterator it;
  for (it = _diagram_panel_list.begin(); it != _diagram_panel_list.end(); it++)
    delete it->second;
  _diagram_panel_list.clear();

  notify_callbacks(); // disconnect all signals

  delete _model_panel;
}

//------------------------------------------------------------------------------

void MainForm::register_form_view_factory(const std::string &name, FormViewFactory factory) {
  _form_view_factories[name] = factory;
}

//------------------------------------------------------------------------------

static void close_plugin(PluginEditorBase *editor, wb::WBContext *wb) {
  wb->close_gui_plugin(dynamic_cast<GUIPluginBase *>(editor));
}

//------------------------------------------------------------------------------
bool MainForm::close_window(GdkEventAny *ev) { /*
                                                if (_wbui_context->request_quit())
                                                {
                                                  get_mainwindow()->hide();
                                                  _wbui_context->perform_quit();
                                                }*/
  wb::WBContextUI::get()->get_wb()->_frontendCallbacks->quit_application();
  return true; // true means stop processing the event
}
//------------------------------------------------------------------------------
void MainForm::is_active_changed() {
  static bool reentrancy_preventer = false;
  if (!reentrancy_preventer && get_mainwindow()->property_is_active()) {
    // We sent notification on each focus_in of the main window,
    // because we have no reliable way to detect when the app itself got focus,
    // as opposed to focus switch between windows of the same app
    reentrancy_preventer = true;
    base::NotificationInfo info;
    base::NotificationCenter::get()->send("GNApplicationActivated", NULL, info);
    reentrancy_preventer = false;
  }
}
//------------------------------------------------------------------------------
void MainForm::on_focus_widget(Gtk::Widget *w) {
  if (_ui) {
    void *data = 0;
    bec::UIForm *form = 0;

    while (!data && w) {
      data = w->get_data("uiform");
      if (data) {
        form = reinterpret_cast<bec::UIForm *>(data);
        break;
      }
      w = w->get_parent();
    }

    if (form)
      wb::WBContextUI::get()->set_active_form(form);
  }

  wb::WBContextUI::get()->get_command_ui()->revalidate_edit_menu_items();
}

//------------------------------------------------------------------------------

void MainForm::on_configure_window(GdkEventConfigure *conf) {
  if (get_mainwindow()->is_visible()) {
    int x, y;
    get_mainwindow()->get_position(x, y);
    std::string geom = base::strfmt("%i %i %i %i", x, y, conf->width, conf->height);
    wb::WBContextUI::get()->get_wb()->save_state("MainWindow", "geometry", geom);
  }
}

//------------------------------------------------------------------------------

void MainForm::on_window_state(GdkEventWindowState *conf) {
  if (get_mainwindow()->is_visible() && (conf->changed_mask & GDK_WINDOW_STATE_MAXIMIZED)) {
    if (conf->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
      wb::WBContextUI::get()->get_wb()->save_state("MainWindow", "geometry", std::string("maximized"));
    else {
      int x, y, width, height;
      get_mainwindow()->get_position(x, y);
      get_mainwindow()->get_size(width, height);
      std::string geom = base::strfmt("%i %i %i %i", x, y, width, height);
      wb::WBContextUI::get()->get_wb()->save_state("MainWindow", "geometry", geom);
    }
  }
}

//------------------------------------------------------------------------------

void MainForm::setup_ui() {
  set_window_icons(get_mainwindow());

  register_commands();

  reset_layout();

  switch_page(0, 0);
}

//------------------------------------------------------------------------------
void MainForm::show() {
  Gtk::Window *window = get_mainwindow();
  // restore saved size/pos
  std::string geom = wb::WBContextUI::get()->get_wb()->read_state("MainWindow", "geometry", std::string());
  if (!geom.empty()) {
    int x, y, width, height;
    if (geom == "maximized")
      window->maximize();
    else if (sscanf(geom.c_str(), "%i %i %i %i", &x, &y, &width, &height) == 4) {
      window->move(x, y);
      window->resize(width, height);
    }
  }
  window->show();
}

//------------------------------------------------------------------------------
Gtk::Window *MainForm::get_mainwindow() const {
  Gtk::Window *win = 0;
  _ui->get_widget("wb_main_window", win);

  return win;
}

Gtk::Notebook *MainForm::get_upper_note() const {
  Gtk::Notebook *note = 0;
  _ui->get_widget("model_tabs", note);
  return note;
}

//------------------------------------------------------------------------------
static bool change_status(Gtk::Statusbar *status, const std::string &text) {
  status->pop();
  status->push(text);
  return false;
}

void MainForm::show_status_text_becb(const std::string &text) {
  Gtk::Statusbar *status = 0;

  _ui->get_widget("statusbar1", status);

  if (bec::GRTManager::get()->in_main_thread())
    change_status(status, text);
  else
    // execute when idle in case we're being called from the worker thread
    _sig_change_status = Glib::signal_idle().connect(
      sigc::bind<Gtk::Statusbar *, std::string>(sigc::ptr_fun(change_status), status, text));
}

//------------------------------------------------------------------------------
bool MainForm::quit_app_becb() {
  // close the model 1st
  if (wb::WBContextUI::get()->get_wb()->can_close_document())
    wb::WBContextUI::get()->get_wb()->close_document();
  else
    return false;

  Gtk::Notebook *note = get_upper_note();
  for (int i = note->get_n_pages() - 1; i > 0; --i) {
    // skip diagram tabs from check
    if (i < note->get_n_pages()) {
      bool is_diagram = dynamic_cast<ModelDiagramPanel *>(
        reinterpret_cast<FormViewBase *>(note->get_nth_page(i)->get_data("FormViewBase")));
      if (!close_tab(note, note->get_nth_page(i)) && !is_diagram)
        return false;
    }
  }
  runtime::app::get().quit();

  return true;
}

//------------------------------------------------------------------------------
bool MainForm::show_progress_becb(const std::string &title, const std::string &status, float pct) {
  if (pct < 0.0)
    pct = 0.0;

  _progress_bar.set_fraction(pct);
  _progress_bar.set_text(status);

  return true;
}

//------------------------------------------------------------------------------
NativeHandle MainForm::open_plugin_becb(grt::Module *grtmodule, const std::string &shlib,
                                        const std::string &editor_class, grt::BaseListRef args,
                                        bec::GUIPluginFlags flags) {
  GUIPluginCreateFunction create_function = 0;
  std::string path = grtmodule->path();
  std::string full_path = path.substr(0, path.rfind('/') + 1) + shlib;

  // if not forcing a new window creation, check if there's an editor already open for it
  if (!(flags & bec::ForceNewWindowFlag) && !(flags & bec::StandaloneWindowFlag)) {
    std::vector<NativeHandle> handles =
      bec::GRTManager::get()->get_plugin_manager()->get_similar_open_plugins(grtmodule, editor_class, args);

    if (!handles.empty()) {
      GUIPluginBase *guiplugin = reinterpret_cast<GUIPluginBase *>(handles[0]);
      // try to reuse the plugin
      PluginEditorBase *editor = dynamic_cast<PluginEditorBase *>(guiplugin);

      if (flags & bec::StandaloneWindowFlag) {
        add_plugin_form(editor);
      } else {
        // add it to the bottom panel of the current tab
        bring_plugin_pane(editor);
      }

      if (editor && !editor->is_editing_live_object() && editor->can_close() && editor->switch_edited_object(args)) {
        bec::GRTManager::get()->get_plugin_manager()->forget_gui_plugin_handle(handles[0]);
        return handles[0];
      }
    }
  }

  // lookup for the editor_class symbol (create<class>) in the shlib
  GModule *module = g_module_open(full_path.c_str(), (GModuleFlags)G_MODULE_BIND_LOCAL);
  if (!module) {
    g_warning("Could not open editor shared object '%s'", full_path.c_str());
    return 0;
  }

  if (!g_module_symbol(module, ("create" + editor_class).c_str(), (void **)&create_function) || !create_function) {
    g_warning("UI creation function '%s' not found in %s", ("create" + editor_class).c_str(), full_path.c_str());
    g_module_close(module);
    return 0;
  }

  // call the initializer with arguments
  GUIPluginBase *object = (*create_function)(grtmodule, args);
  if (!object) {
    g_warning("UI creation function from %s returned 0", full_path.c_str());
    g_module_close(module);
    return 0;
  }

  g_module_close(module);

  PluginEditorBase *editor = dynamic_cast<PluginEditorBase *>(object);
  if (editor) {
    if (flags & bec::StandaloneWindowFlag) {
      add_plugin_form(editor);
    } else {
      // add it to the bottom panel
      add_plugin_pane(editor);
    }
  } else {
    // object->execute();
  }

  // return the pointer
  return reinterpret_cast<NativeHandle>(object);
}

//------------------------------------------------------------------------------
void MainForm::show_plugin_becb(NativeHandle handle) {
  GUIPluginBase *plugin = reinterpret_cast<GUIPluginBase *>(handle);

  PluginEditorBase *editor = dynamic_cast<PluginEditorBase *>(plugin);

  if (editor)
    bring_plugin_pane(editor);
  else
    g_message("Can't show plugin");
}

//------------------------------------------------------------------------------
void MainForm::hide_plugin_becb(NativeHandle handle) {
  //  reinterpret_cast<GUIPluginBase*>(handle)->hide_plugin();
}

static mforms::CodeEditor *get_focused_code_editor(Gtk::Window *w) {
  Gtk::Widget *focused = w->get_focus();
  if (focused)
    return dynamic_cast<mforms::CodeEditor *>(reinterpret_cast<mforms::View *>(focused->get_data("mforms")));
  return NULL;
}

void MainForm::call_find() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor) {
    // generic handling for Scintilla editors
    editor->show_find_panel(false);
    return;
  }
  if (wb::WBContextUI::get()->get_active_context(true) != WB_CONTEXT_QUERY) {
    if (validate_find_replace())
      call_find_replace();
  }

  FormViewBase *form = reinterpret_cast<FormViewBase *>(
    get_upper_note()->get_nth_page(get_upper_note()->get_current_page())->get_data("FormViewBase"));
  if (form) {
    mforms::ToolBar *toolbar = form->get_form()->get_toolbar();
    if (toolbar) {
      Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(mforms::widget_for_toolbar_item_named(toolbar, "find"));
      if (entry) {
        entry->grab_focus();
        form->find_text(entry->get_text());
      }
    }
  }
}

void MainForm::call_find_replace() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor) {
    // generic handling for Scintilla editors
    editor->show_find_panel(true);
    return;
  }

  //  FormViewBase *fview = get_active_pane();
  //  if (fview && !fview->show_find(true))
  //     return;
}

void MainForm::call_undo() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor) {
    editor->undo();
    return;
  }

  auto wbui = wb::WBContextUI::get();
  if (wbui->get_active_main_form() && wbui->get_active_main_form()->can_undo())
    wbui->get_active_main_form()->undo();
}

void MainForm::call_redo() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor) {
    editor->redo();
    return;
  }

  auto wbui = wb::WBContextUI::get();
  if (wbui->get_active_main_form() && wbui->get_active_main_form()->can_redo())
    wbui->get_active_main_form()->redo();
}

void MainForm::call_copy() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  Gtk::Widget *focused = get_mainwindow()->get_focus();

  if (editor) {
    editor->copy();
    return;
  }

  auto wbui = wb::WBContextUI::get();
  GridView *gv = NULL;
  if (dynamic_cast<Gtk::Editable *>(focused))
    dynamic_cast<Gtk::Editable *>(focused)->copy_clipboard();
  else if (wbui->get_active_form() && wbui->get_active_form()->can_copy())
    wbui->get_active_form()->copy();
  else if ((gv = dynamic_cast<GridView *>(focused)))
    gv->copy();
}

void MainForm::call_cut() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  Gtk::Widget *focused = get_mainwindow()->get_focus();

  if (editor) {
    editor->cut();
    return;
  }

  if (dynamic_cast<Gtk::Editable *>(focused))
    dynamic_cast<Gtk::Editable *>(focused)->cut_clipboard();
  else if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_cut())
    wb::WBContextUI::get()->get_active_form()->cut();
}

void MainForm::call_paste() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  Gtk::Widget *focused = get_mainwindow()->get_focus();

  if (editor) {
    editor->paste();
    return;
  }

  if (dynamic_cast<Gtk::Editable *>(focused))
    dynamic_cast<Gtk::Editable *>(focused)->paste_clipboard();
  else if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_paste())
    wb::WBContextUI::get()->get_active_form()->paste();
}

void MainForm::call_delete() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  Gtk::Widget *focused = get_mainwindow()->get_focus();

  if (editor) {
    editor->do_delete();
    return;
  }

  if (dynamic_cast<Gtk::Editable *>(focused))
    dynamic_cast<Gtk::Editable *>(focused)->delete_selection();
  else if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_delete())
    wb::WBContextUI::get()->get_active_form()->delete_selection();
}

void MainForm::call_select_all() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  Gtk::Widget *focused = get_mainwindow()->get_focus();
  if (editor) {
    editor->select_all();
  } else if (dynamic_cast<Gtk::Editable *>(focused))
    dynamic_cast<Gtk::Editable *>(focused)->select_region(0, -1);
  else if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_select_all())
    wb::WBContextUI::get()->get_active_form()->select_all();
}

bool MainForm::validate_find() {
  std::string context = wb::WBContextUI::get()->get_active_context();

  if (context == WB_CONTEXT_MODEL || context == WB_CONTEXT_QUERY || context == WB_CONTEXT_PHYSICAL_OVERVIEW)
    return true;
  return validate_find_replace();
}

bool MainForm::validate_undo() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor)
    return editor->can_undo();

  bec::UIForm *form = wb::WBContextUI::get()->get_active_main_form();
  if (form)
    return form->can_undo();

  return false;
}

bool MainForm::validate_redo() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor)
    return editor->can_redo();

  bec::UIForm *form = wb::WBContextUI::get()->get_active_main_form();
  if (form)
    return form->can_redo();

  return false;
}

bool MainForm::validate_copy() {
  bool ret = false;
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor)
    return editor->can_copy();

  if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_copy())
    ret = true;
  else {
    Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(get_mainwindow()->get_focus());
    int s, e;
    if (edit && edit->get_selection_bounds(s, e))
      ret = true;
    else {
      GridView *gv = dynamic_cast<GridView *>(get_mainwindow()->get_focus());
      if (gv)
        ret = true;
    }
  }

  return ret;
}

bool MainForm::validate_cut() {
  bool ret = false;
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor)
    return editor->can_cut();

  if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_copy())
    ret = true;
  else {
    Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(get_mainwindow()->get_focus());
    int s, e;
    if (edit && edit->get_selection_bounds(s, e) && edit->get_editable())
      ret = true;
  }
  return ret;
}

bool MainForm::validate_paste() {
  bool ret = false;
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor)
    return editor->can_paste();

  if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_paste())
    ret = true;
  else {
    Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(get_mainwindow()->get_focus());
    if (edit && edit->get_editable())
      ret = true;
  }

  return ret;
}

bool MainForm::validate_delete() {
  bool ret = false;
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
  if (editor)
    return editor->can_delete();

  if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_delete())
    ret = true;
  else {
    Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(get_mainwindow()->get_focus());
    int s, e;
    if (edit && edit->get_selection_bounds(s, e) && edit->get_editable())
      ret = true;
  }
  return ret;
}

bool MainForm::validate_select_all() {
  bool ret = false;
  Gtk::Widget *focused = get_mainwindow()->get_focus();

  if (focused) {
    mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());
    if (editor)
      return true;
  }

  if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_select_all())
    ret = true;
  else {
    Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focused);
    if (edit)
      ret = true;
  }

  return ret;
}

bool MainForm::validate_find_replace() {
  mforms::CodeEditor *editor = get_focused_code_editor(get_mainwindow());

  if (editor)
    return true;

  return false;
}

void MainForm::register_commands() {
  std::list<std::string> commands;

  commands.push_back("diagram_size");
  //  commands.push_back("view_model_navigator");
  //  commands.push_back("view_catalog");
  //  commands.push_back("view_layers");
  //  commands.push_back("view_user_datatypes");
  //  commands.push_back("view_object_properties");
  //  commands.push_back("view_object_description");
  //  commands.push_back("view_undo_history");
  commands.push_back("reset_layout");
  commands.push_back("wb.page_setup");
  commands.push_back("closetab");
  commands.push_back("close_tab");
  commands.push_back("close_editor");
  commands.push_back("wb.next_tab");
  commands.push_back("wb.back_tab");
  commands.push_back("wb.next_query_tab");
  commands.push_back("wb.back_query_tab");
  //  commands.push_back("help_index");
  //  commands.push_back("help_version_check");
  commands.push_back("wb.sidebarHide");

  // SQLIDE
  commands.push_back("wb.toggleSidebar");
  commands.push_back("wb.toggleSecondarySidebar");
  commands.push_back("wb.toggleOutputArea");

  auto wbui = wb::WBContextUI::get();

  wbui->get_command_ui()->add_frontend_commands(commands);

  wbui->get_command_ui()->add_builtin_command("find_replace", make_slot(&MainForm::call_find_replace),
                                              make_slot(&MainForm::validate_find_replace));
  wbui->get_command_ui()->add_builtin_command("find", make_slot(&MainForm::call_find),
                                              make_slot(&MainForm::validate_find));
  wbui->get_command_ui()->add_builtin_command("undo", make_slot(&MainForm::call_undo),
                                              make_slot(&MainForm::validate_undo));
  wbui->get_command_ui()->add_builtin_command("redo", make_slot(&MainForm::call_redo),
                                              make_slot(&MainForm::validate_redo));
  wbui->get_command_ui()->add_builtin_command("copy", make_slot(&MainForm::call_copy),
                                              make_slot(&MainForm::validate_copy));
  wbui->get_command_ui()->add_builtin_command("cut", make_slot(&MainForm::call_cut),
                                              make_slot(&MainForm::validate_cut));
  wbui->get_command_ui()->add_builtin_command("paste", make_slot(&MainForm::call_paste),
                                              make_slot(&MainForm::validate_paste));
  wbui->get_command_ui()->add_builtin_command("delete", make_slot(&MainForm::call_delete),
                                              make_slot(&MainForm::validate_delete));
  wbui->get_command_ui()->add_builtin_command("selectAll", make_slot(&MainForm::call_select_all),
                                              make_slot(&MainForm::validate_select_all));
}

//------------------------------------------------------------------------------
void MainForm::perform_command_becb(const std::string &command) {
  if (command == "reset_layout")
    reset_layout();
  else if (command == "diagram_size")
    show_diagram_options();
  else if (command == "wb.page_setup")
    show_page_setup();
  else if (command == "closetab")
    close_active_tab();
  else if (command == "close_tab")
    close_main_tab();
  else if (command == "close_editor")
    close_inner_tab();
  else if (command == "wb.next_tab") {
    int i = get_upper_note()->get_current_page();
    get_upper_note()->next_page();
    if (i == get_upper_note()->get_current_page())
      get_upper_note()->set_current_page(0);
  } else if (command == "wb.back_tab") {
    int i = get_upper_note()->get_current_page();
    get_upper_note()->prev_page();
    if (i == get_upper_note()->get_current_page())
      get_upper_note()->set_current_page(get_upper_note()->get_n_pages() - 1);
  }
  /*
   // Help
   else if (command == "help_index")
   Help.ShowHelp(null, System.IO.Path.Combine(Application.StartupPath, "MySQLWorkbench.chm"));
   else if (command == "help_version_check")
   Program.CheckForNewVersion();
   */
  else if (command == "wb.toggleSidebar" || command == "wb.toggleSecondarySidebar") {
    FormViewBase *form = get_active_pane();
    if (form)
      form->perform_command(command);
  } else {
    FormViewBase *form = get_active_pane();
    if (!form || !form->perform_command(command))
      g_message("Command '%s' not handled!\n", command.c_str());
  }
}

//------------------------------------------------------------------------------
mdc::CanvasView *MainForm::create_view_becb(const model_DiagramRef &diagram) {
  ModelDiagramPanel *model_panel = Gtk::manage(ModelDiagramPanel::create());

  model_panel->set_close_editor_callback(sigc::bind(sigc::ptr_fun(close_plugin), wb::WBContextUI::get()->get_wb()));

  _signal_close_editor.connect(
    sigc::hide_return(sigc::mem_fun(model_panel, &ModelDiagramPanel::close_editors_for_object)));

  model_panel->init(diagram.id());

  TabStateInfo tabState;
  if (!model_panel->get_diagram_form()->is_closed()) {
    if (model_ModelRef::cast_from(diagram->owner())->currentDiagram() == diagram)
      tabState = TabOpenActive;
    else
      tabState = TabOpen;
  } else {
    tabState = TabClosed;
  }

// even considering this widget is not managed, we need to reference it as later we can end up with crash because gtk3
// will release it
// this will be deleted later in destroy_view_becb
// TODO: check if this can be removed after refactoring modeling
#if GTK_VERSION_GE(3, 14)
  model_panel->reference();
#endif
  add_form_pane(model_panel, tabState);
  model_panel->show_all_children(true);
  // model_panel->set_data("model_panel", model_panel);
  mdc::CanvasView *view = model_panel->get_canvas();
  _diagram_panel_list.insert(std::pair<mdc::CanvasView *, ModelDiagramPanel *>(view, model_panel));

  return view;
}

//------------------------------------------------------------------------------
void MainForm::destroy_view_becb(mdc::CanvasView *view) {
  Gtk::Notebook *note = get_upper_note();

  if (!bec::GRTManager::get()->in_main_thread())
    G_BREAKPOINT();

  for (int c = note->get_n_pages(), i = 1; i < c; i++) {
    FormViewBase *form = reinterpret_cast<FormViewBase *>(note->get_nth_page(i)->get_data("FormViewBase"));
    ModelDiagramPanel *model_panel = dynamic_cast<ModelDiagramPanel *>(form);
    // ModelDiagramPanel *model_panel =
    // reinterpret_cast<ModelDiagramPanel*>(note->get_nth_page(i)->get_data("model_panel"));

    if (model_panel && model_panel->get_canvas() == view) {
      note->remove_page(i);
      break;
    }
  }

  std::map<mdc::CanvasView *, ModelDiagramPanel *>::iterator it;
  it = _diagram_panel_list.find(view);
  if (it != _diagram_panel_list.end()) {
    delete it->second;
    _diagram_panel_list.erase(it);
  }
}

//------------------------------------------------------------------------------
void MainForm::switched_view_becb(mdc::CanvasView *view) {
  Gtk::Notebook *note = get_upper_note();

  bec::UIForm *view_form = wb::WBContextUI::get()->get_wb()->get_model_context()->get_diagram_form(view);

  for (int c = note->get_n_pages(), i = 1; i < c; i++) {
    if (note->get_nth_page(i)->get_data("uiform") == view_form) {
      note->get_nth_page(i)->show();
      note->set_current_page(i);
      break;
    }
  }
}

//------------------------------------------------------------------------------

void MainForm::create_main_form_view_becb(const std::string &name, std::shared_ptr<bec::UIForm> form) {
  FormViewBase *view;

  if (_form_view_factories.find(name) != _form_view_factories.end()) {
    view = _form_view_factories[name](form);

    add_form_pane(view, TabOpenActive);
  } else {
    throw std::runtime_error("Form type not supported.");
  }
}

//------------------------------------------------------------------------------

void MainForm::destroy_main_form_view_becb(bec::UIForm *form) {
}

//------------------------------------------------------------------------------
void MainForm::tool_changed_becb(mdc::CanvasView *view) {
  ModelDiagramPanel *panel = get_panel_for_view(view);
  if (panel) {
    // update cursor in canvas
    panel->update_tool_cursor();
  }
}

//------------------------------------------------------------------------------
void MainForm::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNFormTitleDidChange") {
    std::string form_id = info["form"];
    Gtk::Notebook *note = get_upper_note();
    for (int c = note->get_n_pages(), i = 1; i < c; i++) {
      FormViewBase *form = reinterpret_cast<FormViewBase *>(note->get_nth_page(i)->get_data("FormViewBase"));
      bec::UIForm *fui = form ? form->get_form() : 0;
      if (fui && fui->form_id() == form_id) {
        form->signal_title_changed().emit(info["title"]);
        break;
      }
    }
  } else if (name == "GNFocusChanged") {
    wb::WBContextUI::get()->get_command_ui()->revalidate_edit_menu_items();
  }
}

const char *RefreshTypeStr[] = {"RefreshNeeded",
                                "RefreshNothing",
                                "RefreshSchemaNoReload",
                                "RefreshNewDiagram",
                                "RefreshSelection",
                                "RefreshMessages",
                                "RefreshCloseEditor",
                                "RefreshNewModel",
                                "RefreshOverviewNodeInfo",
                                "RefreshOverviewNodeChildren",
                                "RefreshDocument",
                                "RefreshCloseDocument",
                                "RefreshZoom",
                                "RefreshTimer",
                                "RefreshFinishEdit"};

//------------------------------------------------------------------------------

void MainForm::refresh_gui_becb(wb::RefreshType type, const std::string &arg_id, NativeHandle arg_ptr) {
  if (_exiting)
    return;

  // _overview is OverviewPanel
  switch (type) {
    case wb::RefreshNeeded: {
      _sig_flush_idle = Glib::signal_idle().connect(sigc::bind(
        sigc::bind_return(sigc::mem_fun(wb::WBContextUI::get()->get_wb(), &wb::WBContext::flush_idle_tasks), false), false));
      break;
    }

    case wb::RefreshCloseDocument: {
      prepare_close_document();

      wb::WBContextUI::get()->get_wb()->flush_idle_tasks(true);

      wb::WBContextUI::get()->get_wb()->close_document_finish();

      handle_model_closed();
    }
    case wb::RefreshNothing:
      break;

    case wb::RefreshSchemaNoReload: {
      ModelDiagramPanel *panel = dynamic_cast<ModelDiagramPanel *>(get_active_pane());
      if (panel)
        panel->refresh_catalog(false);
      break;
    }

    case wb::RefreshNewDiagram: {
      ModelDiagramPanel *panel = dynamic_cast<ModelDiagramPanel *>(reinterpret_cast<FormViewBase *>(arg_ptr));
      if (panel)
        panel->setup_navigator();
      //         switch_to_diagram(panel);
      break;
    }

    case wb::RefreshOverviewNodeInfo: {
      if (_model_panel &&
          (arg_ptr == dynamic_cast<bec::UIForm *>(_model_panel->get_overview()->get_be()) || arg_ptr == NULL)) {
        _model_panel->get_overview()->refresh_node(bec::NodeId(arg_id));
      }
      break;
    }

    case wb::RefreshOverviewNodeChildren: {
      if (_model_panel && (arg_ptr == dynamic_cast<bec::UIForm *>(wb::WBContextUI::get()->get_physical_overview()) ||
                           arg_ptr == NULL)) {
        if (arg_id.empty()) {
          _model_panel->get_overview()->show();
          _model_panel->get_overview()->rebuild_all();
        }
        _model_panel->get_overview()->refresh_children(bec::NodeId(arg_id));
      }
      break;
    }

    case wb::RefreshDocument: {
      get_mainwindow()->set_title(wb::WBContextUI::get()->get_title());

      ///_model_sidebar.refresh_usertypes();
      break;
    }
    case wb::RefreshSelection: {
      FormViewBase *form = get_active_pane();

      if (form == _model_panel) {
        if (_model_panel)
          _model_panel->selection_changed();
      } else if (dynamic_cast<ModelDiagramPanel *>(form)) {
        ModelDiagramPanel *model = dynamic_cast<ModelDiagramPanel *>(form);
        model->selection_changed();
      }
      break;
    }

    case wb::RefreshNewModel:
      handle_model_created();
      wb::WBContextUI::get()->get_wb()->new_model_finish();
      break;

    case wb::RefreshZoom: {
      ModelDiagramPanel *panel = dynamic_cast<ModelDiagramPanel *>(get_active_pane());
      if (panel)
        panel->refresh_zoom();
      break;
    }

    case wb::RefreshCloseEditor:
      _signal_close_editor.emit(arg_id);
      break;

    case wb::RefreshTimer:
      update_timer();
      break;

    case wb::RefreshFinishEdits: {
      Gtk::Widget *focused = get_mainwindow()->get_focus();
      if (focused) {
        if (dynamic_cast<Gtk::Entry *>(focused)) {
          Gtk::TreeView *tree = dynamic_cast<Gtk::TreeView *>(focused->get_parent());
          if (!tree && focused->get_parent())
            tree = dynamic_cast<Gtk::TreeView *>(focused->get_parent()->get_parent());

          if (tree) {
            Gtk::TreePath path;
            Gtk::TreeViewColumn *column = 0;
            tree->get_cursor(path, column);
            if (column) {
              // this is not working...
              std::vector<Gtk::CellRenderer *> rends(column->get_cells());
              for (std::vector<Gtk::CellRenderer *>::iterator iter = rends.begin(); iter != rends.end(); ++iter) {
                (*iter)->stop_editing(true);
              }
              // fallback
              tree->set_cursor(path, *column, false);
            }
          }
        }
      }
    } break;
  }
}

//------------------------------------------------------------------------------

void MainForm::handle_model_created() {
  wb::OverviewBE *overview_be = wb::WBContextUI::get()->get_physical_overview();
  // Create the model overview panel
  _model_panel = ModelPanel::create(overview_be);
  _model_panel->set_close_editor_callback(sigc::bind(sigc::ptr_fun(close_plugin), wb::WBContextUI::get()->get_wb()));
  _model_panel->get_overview()->get_be()->set_frontend_data(dynamic_cast<FormViewBase *>(_model_panel));
  _signal_close_editor.connect(sigc::hide_return(sigc::mem_fun(_model_panel, &ModelPanel::close_editors_for_object)));

// even considering this widget is not managed, we need to reference it as later we can end up with crash because gtk3
// will release it
// this will be deleted later in handle_model_closed
// TODO: check if this can be removed after refactoring modeling
#if GTK_VERSION_GE(3, 14)
  _model_panel->reference();
#endif

  add_form_pane(dynamic_cast<FormViewBase *>(_model_panel), TabOpenActive);

  _model_panel->get_overview()->rebuild_all();
}
//------------------------------------------------------------------------------
void MainForm::handle_model_closed() {
  if (_model_panel != nullptr) {
    get_upper_note()->remove_page(*_model_panel);
    delete _model_panel;
    _model_panel = nullptr;
  }
}

//------------------------------------------------------------------------------
void MainForm::lock_gui_becb(bool lock) {
  _gui_locked = lock;
}

//------------------------------------------------------------------------------
void MainForm::switch_page(Gtk::Widget *, guint pagenum) {
  Gtk::Notebook *note = get_upper_note();
  Gtk::Widget *page = note->get_nth_page(pagenum);
  if (page) {
    page->show();

    bec::UIForm *uiform = static_cast<bec::UIForm *>(page->get_data("uiform"));
    wb::WBContextUI::get()->set_active_form(uiform);

    FormViewBase *form = static_cast<FormViewBase *>(page->get_data("FormViewBase"));
    if (form)
      form->on_activate();
  }
  wb::WBContextUI::get()->get_command_ui()->revalidate_edit_menu_items();
}

//==============================================================================
ModelDiagramPanel *MainForm::get_panel_for_view(mdc::CanvasView *view) {
  bec::UIForm *form;
  if ((form = wb::WBContextUI::get()->get_wb()->get_model_context()->get_diagram_form(view)))
    return dynamic_cast<ModelDiagramPanel *>(static_cast<FormViewBase *>(form->get_frontend_data()));

  return 0;
}

void MainForm::add_plugin_pane(PluginEditorBase *pane) {
  // locate the active main tab
  Gtk::Notebook *note = get_upper_note();
  int pagenum = note->get_current_page();

  if (pagenum < 0) {
    g_warning("Cannot add plugin with no active tab");
    return;
  }

  Gtk::Widget *page = note->get_nth_page(pagenum);
  if (page) {
    FormViewBase *form = reinterpret_cast<FormViewBase *>(page->get_data("FormViewBase"));

    if (form) {
      // add the editor to the active tab
      form->add_plugin_tab(pane);

      pane->set_data("ContainerForm", form);
    }
  }
}

void MainForm::bring_plugin_pane(PluginEditorBase *pane) {
  // locate the active main tab
  Gtk::Notebook *note = get_upper_note();
  int pagenum = note->get_current_page();

  if (pagenum < 0) {
    g_warning("Cannot add plugin with no active tab");
    return;
  }

  FormViewBase *old_form = reinterpret_cast<FormViewBase *>(pane->get_data("ContainerForm"));

  Gtk::Widget *page = note->get_nth_page(pagenum);
  if (page) {
    FormViewBase *form = reinterpret_cast<FormViewBase *>(page->get_data("FormViewBase"));

    if (form && old_form != form && old_form->get_form()->get_form_context_name() != WB_CONTEXT_QUERY) {
      pane->reference();
      if (old_form)
        old_form->remove_plugin_tab(pane);
      // remove from where it is now
      // add the editor to the active tab
      form->add_plugin_tab(pane);
      pane->unreference();
      pane->set_data("ContainerForm", form);
    }
  }
}

static bool close_plugin_form(GdkEventAny *ev, PluginEditorBase *frame) {
  // wnd window is deleted inside closE_live_object_editor
  frame->close_live_object_editor();
  return true;
}

void MainForm::add_plugin_form(PluginEditorBase *frame) {
  Gtk::Window *window = new Gtk::Window();
  window->add(*frame);
  window->set_title(frame->get_title());
  window->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(close_plugin_form), frame));
  frame->signal_title_changed().connect(sigc::mem_fun(window, &Gtk::Window::set_title));
  int width = 800;
  int height = 600;
  window->resize(width, height);
  window->show_all();
}

void MainForm::close_main_tab() {
  Gtk::Widget *focused = get_mainwindow()->get_focus();
  Gtk::Notebook *upper = get_upper_note();

  // go up the hierarchy to see if a child of the upper notebooks is focused
  while (focused && focused != upper)
    focused = focused->get_parent();

  if (focused) {
    int curpagenum = upper->get_current_page();
    if (curpagenum >= 0) {
      Gtk::Widget *page = upper->get_nth_page(curpagenum);
      bec::UIForm *form = reinterpret_cast<bec::UIForm *>(page->get_data("uiform"));
      if (form && form->get_form_context_name() == "home")
        return;

      close_tab(upper, page);
    }
  }
}

void MainForm::close_inner_tab() {
  Gtk::Widget *focused = get_mainwindow()->get_focus();
  Gtk::Notebook *upper = get_upper_note();

  // go up the hierarchy to see if a child of the upper notebooks is focused
  while (focused && focused != upper)
    focused = focused->get_parent();

  if (focused) {
    int curpagenum = upper->get_current_page();
    if (curpagenum >= 0) {
      Gtk::Widget *page = upper->get_nth_page(curpagenum);
      FormViewBase *panel = reinterpret_cast<FormViewBase *>(page->get_data("FormViewBase"));
      bec::UIForm *form = reinterpret_cast<bec::UIForm *>(page->get_data("uiform"));
      if (form && form->get_form_context_name() == "home")
        return;

      if (panel)
        panel->close_focused_tab();
    }
  }
}

void MainForm::close_active_tab() {
  Gtk::Widget *focused = get_mainwindow()->get_focus();
  Gtk::Notebook *upper = get_upper_note();

  // go up the hierarchy to see if a child of the upper notebooks is focused
  while (focused && focused != upper)
    focused = focused->get_parent();

  if (focused) {
    int curpagenum = upper->get_current_page();
    if (curpagenum >= 0) {
      Gtk::Widget *page = upper->get_nth_page(curpagenum);
      FormViewBase *panel = reinterpret_cast<FormViewBase *>(page->get_data("FormViewBase"));
      bec::UIForm *form = reinterpret_cast<bec::UIForm *>(page->get_data("uiform"));
      if (form && form->get_form_context_name() == "home")
        return;

      if (panel) {
        if (!panel->close_focused_tab())
          close_tab(upper, page);
      } else
        close_tab(upper, page);
    }
  }
}

static bool note_contains_page(Gtk::Notebook *note, Gtk::Widget *page) {
  for (int i = note->get_n_pages() - 1; i >= 0; --i)
    if (note->get_nth_page(i) == page)
      return true;
  return false;
}

bool MainForm::close_tab(Gtk::Notebook *note, Gtk::Widget *page) {
  // on_close should return true if the form should be closed/removed
  mforms::AppView *app_view = reinterpret_cast<mforms::AppView *>(page->get_data("AppView"));
  FormViewBase *form = reinterpret_cast<FormViewBase *>(page->get_data("FormViewBase"));

  if (app_view || form) {
    if ((app_view && app_view->on_close()) || (form && form->on_close())) {
      // can't use page_num, because it will dereference the pointer and it could be deleted (shouldn't be, but it is
      // happening)
      // if (note->page_num(*page) >= 0)
      if (note_contains_page(note, page)) {
        if (form)
          form->dispose();

        note->remove_page(*page);
        page->unreference();

        if (form == _model_panel) {
          delete _model_panel;
          _model_panel = nullptr;
        }
      }
    } else {
      for (int index = 0; index < note->get_n_pages(); ++index) {
        Gtk::Widget *page =  note->get_nth_page(index);
        if (page->is_visible())
            page->child_focus(Gtk::DIR_DOWN);
      }
      return false;
    }
  } else
    page->hide();

  bool visible = false;
  for (int c = note->get_n_pages(), i = 0; i < c; i++) {
    if (note->get_nth_page(i)->is_visible()) {
      visible = true;
      break;
    }
  }

  if (!visible)
    note->hide();

  return true;
}

void MainForm::append_tab_page(Gtk::Notebook *note, Gtk::Widget *widget, const std::string &title,
                               TabStateInfo tabState, ActiveLabel **title_label_ret) {
  // Make a lookup of existing page with the same content
  bool already_have_page = false;
  for (int i = note->get_n_pages() - 1; i >= 0; --i) {
    if (note->get_nth_page(i) == widget) {
      already_have_page = true;
      break;
    }
  }

  if (!already_have_page) {
    ActiveLabel *label = Gtk::manage(
      new ActiveLabel(title, sigc::hide_return(sigc::bind(sigc::mem_fun(this, &MainForm::close_tab), note, widget))));

    if (title_label_ret)
      *title_label_ret = label;

    mforms::Menu *menu = init_tab_menu(widget);
    label->set_menu(menu, true);
    menu->set_handler(sigc::bind(sigc::mem_fun(this, &MainForm::tab_menu_handler), label, widget, note));

    if (g_object_is_floating(G_OBJECT(widget->gobj())))
      g_object_ref_sink(G_OBJECT(widget->gobj()));
    note->append_page(*widget, *label);

    if (tabState == TabOpenActive) {
      // switch the current page when we're in idle
      _sig_set_current_page = Glib::signal_idle().connect(sigc::bind_return(
        sigc::bind(sigc::mem_fun(note, &Gtk::Notebook::set_current_page), note->get_n_pages() - 1), false));
    }
    if (tabState != TabClosed)
      note->show();
  }
}

//------------------------------------------------------------------------------
mforms::Menu *MainForm::init_tab_menu(Gtk::Widget *widget) {
  {
    mforms::Menu *m = new mforms::Menu();
    m->add_item("Close Tab", "close tab");
    m->add_item("Close Other Tabs", "close other tabs");
    m->add_item("Close Other Tabs of This Type", "close similar");
    return m;
  }
}

//------------------------------------------------------------------------------
void MainForm::tab_menu_handler(const std::string &action, ActiveLabel *label, Gtk::Widget *widget,
                                Gtk::Notebook *note) {
  bec::UIForm *uiform = (bec::UIForm *)widget->get_data("uiform");
  if (uiform) {
    if (action == "close tab") {
      _sig_close_tab = Glib::signal_idle().connect(
        sigc::bind_return(sigc::bind(sigc::mem_fun(this, &MainForm::close_tab), note, widget), false));
    } else if (action == "close other tabs") {
      const int note_size = note->get_n_pages();
      for (int i = note_size - 1; i >= 0; --i) {
        Gtk::Widget *cont = note->get_nth_page(i);
        bec::UIForm *cont_uiform = (bec::UIForm *)cont->get_data("uiform");
        const std::string uiform_name = cont_uiform->get_form_context_name();
        if (uiform_name != "home" && cont_uiform != uiform) {
          close_tab(note, cont);
        }
      }
    } else if (action == "close similar") {
      const int note_size = note->get_n_pages();
      const std::string type = uiform->get_form_context_name();
      for (int i = note_size - 1; i >= 0; --i) {
        Gtk::Widget *cont = note->get_nth_page(i);
        bec::UIForm *cont_uiform = (bec::UIForm *)cont->get_data("uiform");
        const std::string uiform_type = cont_uiform->get_form_context_name();
        if (uiform_type != "home" && cont != widget && type == uiform_type) {
          close_tab(note, cont);
        }
      }
    }
  }
}

FormViewBase *MainForm::get_active_pane() {
  int p = get_upper_note()->get_current_page();
  if (p >= 0) {
    Gtk::Widget *page = get_upper_note()->get_nth_page(p);
    if (page)
      return reinterpret_cast<FormViewBase *>(page->get_data("FormViewBase"));
  }
  return 0;
}

void MainForm::add_form_pane(FormViewBase *panel, TabStateInfo tabState) {
  panel->get_panel()->set_data("FormViewBase", panel);
  panel->get_panel()->set_data("uiform", dynamic_cast<bec::UIForm *>(panel->get_form()));

  ActiveLabel *label = 0;
  append_tab_page(get_upper_note(), panel->get_panel(), panel->get_title(), tabState, &label);
  // Fix gtk3 issue... caused by gtk3: 399de111167c198a7d2ccbd459a2db7c6389181e
  get_upper_note()->set_current_page(get_upper_note()->page_num(*panel->get_panel()));

  if (tabState == TabClosed)
    panel->get_panel()->hide();

  if (label)
    panel->signal_title_changed().connect(sigc::mem_fun(label, &ActiveLabel::set_text));

  mforms::on_add_menubar_to_window(panel->get_form()->get_menubar(), get_mainwindow());

  panel->reset_layout();
}

void MainForm::prepare_close_document() {
  // close all diagram tabs so they stop receiving events (which can lead to a crash)
  Gtk::Notebook *note = get_upper_note();

  for (int i = note->get_n_pages() - 1; i >= 0; --i) {
    Gtk::Widget *panel = note->get_nth_page(i);
    if (dynamic_cast<ModelDiagramPanel *>(panel))
      panel->hide();
  }

  // reset overview
  if (_model_panel != nullptr)
    _model_panel->get_overview()->reset();
}

bool MainForm::fire_timer() {
  bec::GRTManager::get()->flush_timers();

  update_timer();
  return false;
}

void MainForm::update_timer() {
  int delay_ms = (int)(1000 * bec::GRTManager::get()->delay_for_next_timeout());

  if (delay_ms >= 0)
    Glib::signal_timeout().connect(sigc::mem_fun(this, &MainForm::fire_timer), delay_ms);
}

//==============================================================================
// Here go command handlers
//
//------------------------------------------------------------------------------
void MainForm::reset_layout() {
  ///
}

void MainForm::show_diagram_options() {
  DiagramSizeForm *form = DiagramSizeForm::create();

  form->run();

  form->hide();
  delete form;
}

void MainForm::show_page_setup() {
  wb::WBContextUI::get()->get_wb()->execute_plugin("wb.print.setup");
}

#include "mforms/mforms.h"
#include "gtk/lf_view.h"

static std::string get_resource_path(mforms::App *app, const std::string &file) {
  if (file.empty())
    return bec::GRTManager::get()->get_data_file_path("");
  if (file[0] == '/')
    return file;

  if (g_str_has_suffix(file.c_str(), ".png") || g_str_has_suffix(file.c_str(), ".xpm"))
    return bec::IconManager::get_instance()->get_icon_path(file);
  else if (g_str_has_suffix(file.c_str(), ".txt")) { // This is special handling for txt only on Linux.
    auto parts = base::split(bec::GRTManager::get()->get_basedir(), "/");
    std::string last = parts.back() + (wb::WBContextUI::get()->get_wb()->is_commercial() ? "-commercial" : "-community");
    parts.pop_back();
    parts.push_back("doc");
    parts.push_back(last);
    parts.push_back(file);
    return base::join(parts, "/");
  }

  return bec::GRTManager::get()->get_data_file_path(file);
}

static std::string get_executable_path(mforms::App *app, const std::string &file) {
  std::string path = bec::GRTManager::get()->get_data_file_path(file);
  if (!path.empty() && base::file_exists(path))
    return path;

  path = base::dirname(std::string(getenv("MWB_MODULE_DIR"))) + "/" + file;
  if (base::file_exists(path))
    return path;

  const char *basedir = getenv("MWB_BASE_DIR");
  if (basedir) {
    char *p = g_strdup_printf("%s/libexec/mysql-workbench/%s", basedir, file.c_str());
    path = p;
    g_free(p);
    if (g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
      return path;

    p = g_strdup_printf("%s/bin/%s", basedir, file.c_str());
    path = p;
    g_free(p);
    if (g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
      return path;

    p = g_strdup_printf("%s/libexec/%s", basedir, file.c_str());
    path = p;
    g_free(p);
    if (g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
      return path;
  }

  return "";
}

static base::Rect get_main_window_bounds(mforms::App *app) {
  Gtk::Window *w = get_mainwindow();
  int x, y;

  w->get_window()->get_position(x, y);

  return base::Rect(x, y, w->get_width(), w->get_height());
}

void MainForm::set_status_text(mforms::App *app, const std::string &text) {
  MainForm *self = reinterpret_cast<MainForm *>(app->get_data_ptr());

  self->show_status_text_becb(text);
}

struct EventLoopFrame {
  int exit_code;
  bool timedout;
  bool ended;
  runtime::loop loop;
};

static std::list<EventLoopFrame *> event_loop_exit_codes;

static bool event_loop_timeout() {
  if (!event_loop_exit_codes.empty()) {
    EventLoopFrame *frame = event_loop_exit_codes.back();
    if (!frame->timedout && !frame->ended) {
      frame->timedout = true;
      frame->loop.quit();
    }
  }
  return false;
}

static int begin_event_loop(mforms::App *, float timeout) {
  EventLoopFrame frame;

  frame.exit_code = -1;
  frame.timedout = false;
  frame.ended = false;

  sigc::connection timeout_conn;
  if (timeout > 0.0)
    timeout_conn = Glib::signal_timeout().connect(sigc::ptr_fun(event_loop_timeout), (unsigned int)(timeout * 1000));

  event_loop_exit_codes.push_back(&frame);
  frame.loop.run();

  timeout_conn.disconnect();
  if (event_loop_exit_codes.empty() || event_loop_exit_codes.back() != &frame) {
    g_warning("Internal inconsistency in begin_event_loop");
    return -1;
  }

  if (!frame.ended && !frame.timedout) {
    // means something other than end_event_loop() called quit(), could mean the app is being quit..
    runtime::app::get().quit();
    return -1;
  } else {
    int rc = event_loop_exit_codes.back()->exit_code;
    event_loop_exit_codes.pop_back();
    return rc;
  }
}

static void end_event_loop(mforms::App *, int rc) {
  if (event_loop_exit_codes.empty()) {
    g_warning("Attempt to exit unexisting event loop");
    return;
  }

  if (!event_loop_exit_codes.back()->timedout) {
    event_loop_exit_codes.back()->exit_code = rc;
    event_loop_exit_codes.back()->ended = true;
    event_loop_exit_codes.back()->loop.quit();
  }
}

static bool isDarkModeActive(mforms::App *) {
	// On Linux we can't just say if theme is dark or light
	return false;
}

static float backing_scale_factor(mforms::App *) {
	auto window = get_mainwindow();
    double dpi = Gdk::Screen::get_default()->get_resolution();
	int scaleFactor = Gdk::Screen::get_default()->get_monitor_scale_factor(Gdk::Screen::get_default()->get_monitor_at_window(window->get_window()));
	return dpi > 0 ? scaleFactor * dpi / defaultDpi : dpi;
}

void MainForm::setup_mforms_app() {
  mforms::ControlFactory *cf = mforms::ControlFactory::get_instance();
  g_assert(cf);

  mforms::App::instantiate(this, false);
  mforms::App::get()->set_data(this);

  cf->_app_impl.get_resource_path = &get_resource_path;
  cf->_app_impl.get_executable_path = &get_executable_path;
  cf->_app_impl.set_status_text = &set_status_text;
  cf->_app_impl.get_application_bounds = &get_main_window_bounds;
  cf->_app_impl.enter_event_loop = &begin_event_loop;
  cf->_app_impl.exit_event_loop = &end_event_loop;
  cf->_app_impl.backing_scale_factor = &backing_scale_factor;
  cf->_app_impl.isDarkModeActive = &isDarkModeActive;
}

Gtk::Widget *MainForm::decorate_widget(Gtk::Widget *panel, bec::UIForm *form) {
  mforms::MenuBar *menu = form->get_menubar();
  mforms::ToolBar *toolbar = form->get_toolbar();

  Gtk::Box *top_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
  if (menu) {
    Gtk::Widget *w = mforms::widget_for_menubar(menu);
    w->set_name(form->get_form_context_name());
    top_box->pack_start(*w, false, true);
    w->show();
    on_add_menubar_to_window(menu, get_mainwindow());
  }

  if (toolbar) {
    Gtk::Widget *w = mforms::widget_for_toolbar(toolbar);
    top_box->pack_start(*w, false, true);
    w->show();
  }
  top_box->pack_start(*panel, true, true);
  panel->show();

  top_box->show();

  return top_box;
}

static gpointer delete_appview(mforms::AppView *appview) {
  if (appview->is_managed())
    appview->release();
  else
    delete appview;
  return 0;
}

std::pair<int, int> MainForm::get_size() {
  Gtk::Widget *note = get_upper_note();
  int w = note->get_width();
  int h = note->get_height();
  return std::pair<int, int>(w, h);
}

void MainForm::set_name(const std::string &name) {
  if (get_upper_note()) {
   Glib::RefPtr<Atk::Object> acc = get_upper_note()->get_accessible();
   if (acc)
     acc->set_name(name);
  }
}

void MainForm::dock_view(mforms::AppView *view, const std::string &position, int) {
  g_return_if_fail(view != NULL);
  Gtk::Widget *w = mforms::widget_for_view(view);
  g_return_if_fail(w != NULL);

  if (position == "maintab" || position.empty()) {
    Gtk::Notebook *note = get_upper_note();
    // Make a lookup of existing page with the same content
    bool already_have_page = false;
    for (int i = note->get_n_pages() - 1; i >= 0; --i) {
      Gtk::Widget *page;
      if ((page = note->get_nth_page(i)) && page->get_data("uiform") == dynamic_cast<bec::UIForm *>(view)) {
        already_have_page = true;
        break;
      }
    }
    if (already_have_page)
      return;

    if (view->release_on_add())
      view->set_release_on_add(false);
    else
      view->retain();

    if (!view->get_menubar())
      view->set_menubar(mforms::manage(wb::WBContextUI::get()->get_command_ui()->create_menubar_for_context(
        view->is_main_form() ? view->get_form_context_name() : "")));

    Gtk::Widget *decorated = reinterpret_cast<Gtk::Widget *>(w->get_data("DockDecoration"));
    if (!decorated) {
      decorated = decorate_widget(w, view);
      w->set_data("DockDecoration", decorated);

      //     if (g_object_is_floating(decorated->gobj()))
      //       g_object_ref_sink(decorated->gobj()); // turn the floating ref to a normal one
    }

    decorated->set_data("AppView", view);
    decorated->set_data("uiform", dynamic_cast<bec::UIForm *>(view));
    decorated->add_destroy_notify_callback(view, (gpointer(*)(gpointer))delete_appview);

    if (dynamic_cast<bec::UIForm *>(view)->get_form_context_name() == "home") {
      Gtk::Widget *tab = Gtk::manage(new Gtk::Image(bec::IconManager::get_instance()->get_icon_path("WB_Home.png")));
      tab->set_margin_left(10);
      tab->set_margin_right(10);
      note->append_page(*decorated, *tab);
    } else {
      ActiveLabel *label = 0;
      append_tab_page(get_upper_note(), decorated, "", TabOpenActive, &label);
      w->set_data("tablabel", label);
    }
    decorated->set_data("uiform", dynamic_cast<bec::UIForm *>(view));
    w->set_data("containerNote", note);
  }
}

bool MainForm::select_view(mforms::AppView *view) {
  Gtk::Notebook *upper_note = get_upper_note();
  for (int i = 0; i < upper_note->get_n_pages(); i++) {
    Gtk::Widget *page = upper_note->get_nth_page(i);
    if (page && reinterpret_cast<mforms::AppView *>(page->get_data("AppView")) == view) {
      upper_note->set_current_page(i);
      return true;
    }
  }
  return false;
}

void MainForm::undock_view(mforms::AppView *view) {
  g_return_if_fail(view != NULL);
  Gtk::Widget *w = mforms::widget_for_view(view);
  g_return_if_fail(w != NULL);

  Gtk::Notebook *note = reinterpret_cast<Gtk::Notebook *>(w->get_data("containerNote"));
  Gtk::Widget *decorated = reinterpret_cast<Gtk::Widget *>(w->get_data("DockDecoration"));
  if (note && decorated) {
    note->remove_page(*decorated);
  }
  view->release();
}

void MainForm::set_view_title(mforms::AppView *view, const std::string &title) {
  g_return_if_fail(view != NULL);
  Gtk::Widget *w = mforms::widget_for_view(view);
  g_return_if_fail(w != NULL);

  ActiveLabel *label = reinterpret_cast<ActiveLabel *>(w->get_data("tablabel"));
  if (label)
    label->set_text(title);
}

mforms::AppView *MainForm::selected_view() {
  int i = get_upper_note()->get_current_page();
  if (i >= 0)
    return view_at_index(i);
  return NULL;
}

int MainForm::view_count() {
  return get_upper_note()->get_n_pages();
}

mforms::AppView *MainForm::view_at_index(int index) {
  Gtk::Widget *page = get_upper_note()->get_nth_page(index);
  if (page)
    return reinterpret_cast<mforms::AppView *>(page->get_data("AppView"));
  return NULL;
}
