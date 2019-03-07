/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

//!
//!
//! \addtogroup linuxui Linux UI
//! @{
//!

#ifndef __MAIN_FORM_H__
#define __MAIN_FORM_H__

#include <gtkmm/window.h>
#include <gtkmm/notebook.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/builder.h>

#include "mforms/mforms.h"
#include "mforms/dockingpoint.h"
#include "base/notifications.h"

#include "workbench/wb_context.h"

namespace bec {
  class GRTManager;
}

namespace Gtk {
  class Window;
}

class ActiveLabel;
class FormViewBase;
class ModelPanel;
class OverviewPanel;
class ModelDiagramPanel;
class OutputBox;

class FormViewBase;
class PluginEditorBase;

//==============================================================================
//
//==============================================================================
class MainForm : public sigc::trackable, base::Observer, public mforms::DockingPointDelegate {
public:
  enum TabStateInfo { TabClosed, TabOpen, TabOpenActive };

  MainForm();
  ~MainForm();
  void setup_ui();
  Gtk::Window *get_mainwindow() const;

  void show();

public:
  typedef sigc::slot<FormViewBase *, std::shared_ptr<bec::UIForm> > FormViewFactory;
  void register_form_view_factory(const std::string &name, FormViewFactory factory);

  void show_status_text_becb(const std::string &text);
  bool show_progress_becb(const std::string &title, const std::string &status, float pct);
  NativeHandle open_plugin_becb(grt::Module *module, const std::string &shlib, const std::string &editor_class,
                                grt::BaseListRef args, bec::GUIPluginFlags flags);
  void show_plugin_becb(NativeHandle handle);
  void hide_plugin_becb(NativeHandle handle);
  void perform_command_becb(const std::string &command);
  // Creates diagram view
  mdc::CanvasView *create_view_becb(const model_DiagramRef &);
  void destroy_view_becb(mdc::CanvasView *view);
  void switched_view_becb(mdc::CanvasView *view);
  void tool_changed_becb(mdc::CanvasView *view);
  void refresh_gui_becb(wb::RefreshType type, const std::string &arg_id, NativeHandle arg_ptr);
  void lock_gui_becb(bool lock);
  void create_main_form_view_becb(const std::string &name, std::shared_ptr<bec::UIForm> form);
  void destroy_main_form_view_becb(bec::UIForm *form);
  bool quit_app_becb();

  void exiting() {
    _exiting = true;
  }

private:
  std::map<mdc::CanvasView *, ModelDiagramPanel *> _diagram_panel_list;

  virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);

  void register_commands();
  ModelDiagramPanel *get_panel_for_view(mdc::CanvasView *view);

  FormViewBase *get_active_pane();

  //  void add_model_pane(ModelDiagramPanel *pane);
  //
  void add_form_pane(FormViewBase *pane, TabStateInfo tabState);

  void add_plugin_form(PluginEditorBase *frame);

  void add_plugin_pane(PluginEditorBase *editor);
  void bring_plugin_pane(PluginEditorBase *editor);
  //    void remove_plugin_pane(PluginEditorBase *editor);
  //

  void close_active_tab();
  void close_main_tab();
  void close_inner_tab();

  bool close_tab(Gtk::Notebook *note, Gtk::Widget *widget);
  void append_tab_page(Gtk::Notebook *note, Gtk::Widget *widget, const std::string &title, TabStateInfo tabState,
                       ActiveLabel **title_label_ret = 0);

  //  void init_tab_menu(mforms::Menu* m);
  mforms::Menu *init_tab_menu(Gtk::Widget *widget);
  void tab_menu_handler(const std::string &action, ActiveLabel *label, Gtk::Widget *widget, Gtk::Notebook *note);

  void show_output();
  void show_diagram_options();
  void show_page_setup();

  void handle_model_created();
  void handle_model_closed();

  bool close_window(GdkEventAny *ev);
  void on_focus_widget(Gtk::Widget *focus);
  void on_configure_window(GdkEventConfigure *conf);
  void on_window_state(GdkEventWindowState *conf);
  void is_active_changed();

  void prepare_close_document();

  void update_timer();
  bool fire_timer();

  // command handlers
  void reset_layout();

  void switch_page(Gtk::Widget *page, guint pagenum);

  Gtk::Notebook *get_upper_note() const;

  void call_find_replace();
  void call_find();
  void call_undo();
  void call_redo();
  void call_paste();
  void call_cut();
  void call_copy();
  void call_select_all();
  void call_delete();
  void call_search();

  bool validate_find_replace();
  bool validate_find();
  bool validate_undo();
  bool validate_redo();
  bool validate_copy();
  bool validate_cut();
  bool validate_paste();
  bool validate_select_all();
  bool validate_delete();
  bool validate_search();

private:
  // mforms integration
  void setup_mforms_app();

  virtual std::string get_type() {
    return "MainWindow";
  }
  virtual void set_name(const std::string &name);
  virtual void dock_view(mforms::AppView *view, const std::string &position, int arg);
  virtual bool select_view(mforms::AppView *view);
  virtual void undock_view(mforms::AppView *view);
  virtual std::pair<int, int> get_size();
  virtual void set_view_title(mforms::AppView *view, const std::string &title);
  virtual mforms::AppView *selected_view();
  virtual int view_count();
  virtual mforms::AppView *view_at_index(int index);

  static void set_status_text(mforms::App *app, const std::string &text);

  //@@@  bool find_callback(const std::string &search, const std::string &replace,
  //                     mforms::SearchFlags flags, SqlEditorFE *editor);

  Gtk::Widget *decorate_widget(Gtk::Widget *panel, bec::UIForm *form);

private:
  std::map<std::string, FormViewFactory> _form_view_factories;

  ModelPanel *_model_panel;
  OverviewPanel *_model_overview; //!< Overview of the model, see overview_panel.h

  OutputBox *_output_box;

  Glib::RefPtr<Gtk::Builder> _ui; //!< Glade model wrapper of the main window
  const char *_db_glade_file;     //!< File name of the glade model of the model overview part

  bool _gui_locked;
  bool _exiting;
  Gtk::ProgressBar _progress_bar;

  sigc::signal<void, std::string> _signal_close_editor;

  sigc::connection _sig_change_status;
  sigc::connection _sig_flush_idle;
  sigc::connection _sig_set_current_page;
  sigc::connection _sig_close_tab;

  typedef std::vector<sigc::slot_base> Slots;
  Slots _slots;

  template <typename R, typename T>
  sigc::slot<R> make_slot(R (T::*t)()) {
    const sigc::slot<R> slot = sigc::mem_fun(this, t);
    _slots.push_back(slot);
    return slot;
  }
};

#endif
