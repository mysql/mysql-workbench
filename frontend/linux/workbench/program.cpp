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

#include "grtpp_module_python.h"

#include "gtk/lf_mforms.h"

// linux ui includes
#include "gtk_helpers.h"
#include "program.h"
#include "main_form.h"
#include "wbdbg.h"

#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/builder.h>

// the rest, backend, etc ...
#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_module.h"

#include "grtpp_util.h" // for strfmt

#include "sqlide_main.h"

#include "base/string_utilities.h"
#include "base/log.h"
#include "main_app.h"

using base::strfmt;

#define REFRESH_TIMER 250

#include <gtkmm/messagedialog.h>

Program* Program::_instance = 0;

static void flush_main_thread() {
  while (Gtk::Main::events_pending()) {
    if (Gtk::Main::iteration(false)) {
      // return value of true means quit() was called
      // with gtk3 we're handling this different way
      // no need to call Gtk::quit here
      break;
    }
  }
}

DEFAULT_LOG_DOMAIN("Program")
//------------------------------------------------------------------------------
Program::Program() : _main_form(nullptr), _wbcallbacks(new wb::WBFrontendCallbacks()) {};

void Program::init(wb::WBOptions& wboptions) {
  _instance = this;
// Setup backend stuff
//  _wb_context_ui = new wb::WBContextUI(wboptions.verbose);

#ifdef ENBLE_DEBUG
  if (!getenv("MWB_DATA_DIR")) {
    const char* path = "../share/mysql-workbench";
    g_message("MWB_DATA_DIR is unset! Setting MWB_DATA_DIR to predifined value '%s'", path);
    setenv("MWB_DATA_DIR", path, 1);
  }

  if (!getenv("MWB_MODULE_DIR")) {
    const char* path = "../lib/mysql-workbench/modules";
    g_message("MWB_MODULE_DIR is unset! Setting MWB_MODULE_DIR to predifined value '%s'", path);
    setenv("MWB_MODULE_DIR", path, 1);
  }

  if (!getenv("MWB_LIBRARY_DIR")) {
    const char* path = "../share/mysql-workbench/libraries";
    g_message("MWB_LIBRARY_DIR is unset! Setting MWB_LIBRARY_DIR to predifined value '%s'", path);
    setenv("MWB_LIBRARY_DIR", path, 1);
  }

  if (!getenv("MWB_PLUGIN_DIR")) {
    const char* path = "../lib/mysql-workbench";
    g_message("MWB_PLUGIN_DIR is unset! Setting MWB_PLUGIN_DIR to predifined value '%s'", path);
    setenv("MWB_PLUGIN_DIR", path, 1);
  }
#endif
  if (!getenv("MWB_DATA_DIR") || (!getenv("MWB_MODULE_DIR"))) {
    g_print("Please start Workbench through mysql-workbench instead of calling mysql-workbench-bin directly\n");
    exit(1);
  }

  bec::GRTManager::get()->set_datadir(getenv("MWB_DATA_DIR"));

  // Main form holds UI code, Glade wrapper, etc ...
  _main_form = new MainForm();

  // Assign those callback methods
  _wbcallbacks->show_file_dialog = std::bind(&Program::show_file_dialog_becb, this, std::placeholders::_1,
                                           std::placeholders::_2, std::placeholders::_3);
  _wbcallbacks->show_status_text = std::bind(&MainForm::show_status_text_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->open_editor =
    std::bind(&MainForm::open_plugin_becb, _main_form, std::placeholders::_1, std::placeholders::_2,
              std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
  _wbcallbacks->show_editor = std::bind(&MainForm::show_plugin_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->hide_editor = std::bind(&MainForm::hide_plugin_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->perform_command = std::bind(&MainForm::perform_command_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->create_diagram = std::bind(&MainForm::create_view_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->destroy_view = std::bind(&MainForm::destroy_view_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->switched_view = std::bind(&MainForm::switched_view_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->create_main_form_view =
    std::bind(&MainForm::create_main_form_view_becb, _main_form, std::placeholders::_1, std::placeholders::_2);
  _wbcallbacks->destroy_main_form_view =
    std::bind(&MainForm::destroy_main_form_view_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->tool_changed = std::bind(&MainForm::tool_changed_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->refresh_gui = std::bind(&MainForm::refresh_gui_becb, _main_form, std::placeholders::_1,
                                      std::placeholders::_2, std::placeholders::_3);
  _wbcallbacks->lock_gui = std::bind(&MainForm::lock_gui_becb, _main_form, std::placeholders::_1);
  _wbcallbacks->quit_application = std::bind(&MainForm::quit_app_becb, _main_form);

  wboptions.basedir = getenv("MWB_DATA_DIR");
  wboptions.plugin_search_path = getenv("MWB_PLUGIN_DIR");
  wboptions.struct_search_path = wboptions.basedir + "/grt";
  wboptions.module_search_path = getenv("MWB_MODULE_DIR");
  wboptions.library_search_path = getenv("MWB_LIBRARY_DIR");
  wboptions.cdbc_driver_search_path = getenv("DBC_DRIVER_PATH") ?: "";
  if (wboptions.cdbc_driver_search_path.empty())
    wboptions.cdbc_driver_search_path = wboptions.library_search_path;

  if (!wboptions.user_data_dir.empty()) {
    if (!base::is_directory(wboptions.user_data_dir)) {
      try {
        if (!base::copyDirectoryRecursive(std::string(g_get_home_dir()).append("/.mysql/workbench"),
                                          wboptions.user_data_dir)) {
          logError("Unable to prepare new config directory: %s\n", wboptions.user_data_dir.c_str());
          exit(1);
        }
      } catch (std::exception& exc) {
        logError("There was a problem preparing new config directory. The error was: %s\n", exc.what());
        exit(1);
      }
    }
  } else
    wboptions.user_data_dir = std::string(g_get_home_dir()).append("/.mysql/workbench");

  wb::WBContextUI::get()->init(_wbcallbacks, &wboptions);
  bec::GRTManager::get()->get_dispatcher()->set_main_thread_flush_and_wait(flush_main_thread);

  {
    std::string form_name;
    sigc::slot<FormViewBase*, std::shared_ptr<bec::UIForm> > form_creator;

    setup_sqlide(form_name, form_creator);

    _main_form->register_form_view_factory(form_name, form_creator);
  }

  _main_form->setup_ui();

  // show the window only when everything is done
  _sig_finalize_initialization = Glib::signal_idle().connect(
    sigc::bind_return(sigc::bind(sigc::mem_fun(this, &Program::finalize_initialization), &wboptions), false));
  //  _main_form->show();

  _idle_signal_conn = Glib::signal_timeout().connect(sigc::mem_fun(this, &Program::idle_stuff), REFRESH_TIMER);
}

//------------------------------------------------------------------------------
Program::~Program() {
  delete _wbcallbacks;
}

void Program::finalize_initialization(wb::WBOptions* options) {
  _main_form->show();
  wb::WBContextUI::get()->init_finish(options);
}

bool Program::idle_stuff() {
  // if there are tasks to be executed, schedule it to be done when idle so that the timer
  // doesn't get blocked during its execution
  _idleConnections.push_back(Glib::signal_idle().connect(sigc::bind(
    sigc::bind_return(sigc::mem_fun(wb::WBContextUI::get()->get_wb(), &wb::WBContext::flush_idle_tasks), false), false)));
  return true;
}

void Program::shutdown() {
  if (_instance == nullptr) // there was no initialization
    return;

  _main_form->exiting();

  wb::WBContextUI::get()->finalize();

  // sigc::connection conn(idle_signal_conn);
  _sig_finalize_initialization.disconnect();
  _idle_signal_conn.disconnect();

  bec::GRTManager::get()->terminate();
  bec::GRTManager::get()->get_dispatcher()->shutdown();

  for (std::deque<sigc::connection>::iterator it = _idleConnections.begin(); it != _idleConnections.end(); it++)
    (*it).disconnect();

  _idleConnections.clear();

  delete _main_form;
  _main_form = 0;
}

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
struct GtkAutoLock {
  GtkAutoLock() {
    if (getenv("WB_ADD_LOCKS"))
      gdk_threads_enter();
  }
  ~GtkAutoLock() {
    if (getenv("WB_ADD_LOCKS"))
      gdk_threads_leave();
  }
};
#pragma GCC diagnostic pop

int Program::confirm_action_becb(const std::string& title, const std::string& msg, const std::string& default_btn,
                                 const std::string& alt_btn, const std::string& other_btn) {
  GtkAutoLock lock;

  Gtk::MessageDialog dlg(strfmt("<b>%s</b>\n%s", title.c_str(), msg.c_str()), true, Gtk::MESSAGE_QUESTION,
                         Gtk::BUTTONS_NONE, true);
  dlg.set_title(title);

  dlg.add_button(default_btn, 1);
  if (!other_btn.empty())
    dlg.add_button(other_btn, 3);
  dlg.add_button(alt_btn, 2);

  dlg.set_transient_for(*get_mainwindow());

  int response = dlg.run();

  switch (response) {
    case 1:
      return 1;
    case 2:
      return 0;
    case 3:
      return -1;
    default: // Escape
      if (default_btn == _("Cancel"))
        return 1;
      else if (alt_btn == _("Cancel"))
        return 0;
      else if (other_btn == _("Cancel"))
        return -1;
      return 0;
  }
}

//------------------------------------------------------------------------------
std::string Program::show_file_dialog_becb(const std::string& type, const std::string& title,
                                           const std::string& extensions) {
  Gtk::FileChooserDialog dlg(title, (type == "open" ? Gtk::FILE_CHOOSER_ACTION_OPEN : Gtk::FILE_CHOOSER_ACTION_SAVE));

  dlg.set_transient_for(*(_main_form->get_mainwindow()));

  dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  if (type == "open")
    dlg.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
  else
    dlg.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

  // extensions format:
  // AAA Files (*.aaa)|*.aaa,BBB Files (*.bbb)

  std::vector<std::string> exts(base::split(extensions, ","));
  std::string default_ext;

  for (std::vector<std::string>::const_iterator iter = exts.begin(); iter != exts.end(); ++iter) {
    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();

    if (iter->find('|') != std::string::npos) {
      std::string descr, ext;

      descr = iter->substr(0, iter->find('|'));
      ext = iter->substr(iter->find('|') + 1);

      if (default_ext.empty()) {
        default_ext = ext;
        if (default_ext[0] == '*')
          default_ext = default_ext.substr(1);
        if (default_ext[0] == '.')
          default_ext = default_ext.substr(1);
      }
      filter->add_pattern(ext);
      filter->set_name(descr);
    } else if (*iter == "mwb") {
      if (default_ext.empty())
        default_ext = "mwb";
      filter->add_pattern("*.mwb");
      filter->set_name("MySQL Workbench Models (*.mwb)");
    } else if (*iter == "sql") {
      if (default_ext.empty())
        default_ext = "sql";
      filter->add_pattern("*.sql");
      filter->set_name("SQL Script Files");
    } else {
      if (default_ext.empty())
        default_ext = *iter;
      filter->add_pattern("*." + *iter);
      filter->set_name(strfmt("%s files (*.%s)", iter->c_str(), iter->c_str()));
    }
    dlg.add_filter(filter);
  }

  Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
  filter->add_pattern("*");
  filter->set_name("All Files");
  dlg.add_filter(filter);

  std::string file("");

  if (!default_ext.empty())
    default_ext = "." + default_ext;

  for (;;) {
    int result = dlg.run();

    if (result == Gtk::RESPONSE_OK) {
      file = dlg.get_filename();
      if (!base::hasSuffix(file, default_ext) && type == "save")
        file = file + default_ext;

      if (type == "save" && g_file_test(file.c_str(), G_FILE_TEST_EXISTS)) {
        if (confirm_action_becb(strfmt(_("\"%s\" Already Exists. Do you want to replace it?"), file.c_str()),
                                strfmt(_("Replacing it will overwrite its current contents.")), _("Replace"),
                                _("Cancel"), "") != 1)
          continue;
      }
    }
    break;
  }

  return file;
}

//------------------------------------------------------------------------------
Gtk::Window* Program::get_mainwindow() const {
  return _main_form->get_mainwindow();
}

// get_mainwindow is declared in gtk_helpers.h
//------------------------------------------------------------------------------
void* get_mainwindow_impl() {
  return Program::get_instance()->get_mainwindow();
}
