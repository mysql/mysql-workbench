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

//!
//! \addtogroup linuxui Linux UI
//! @{
//!

#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include <string>
#include <sigc++/sigc++.h>

#include "workbench/wb_context.h"

namespace bec {
  class GRTManager;
}

namespace Gtk {
  class Window;
}

//==============================================================================

class MainForm;

//==============================================================================
//
//==============================================================================
//! Class Program mostly gathers together top level UI classes and top level
//! backend classes like wb::WBContextUI under one hood, plus it
//! defines some callbacks-methods to pass to backend's wb::WBContext.
//! Also it has a timer-triggered idle tasks runner, see Program::idle_stuff()
class Program {
public:
  Program();
  void init(wb::WBOptions& options);
  ~Program();

  void shutdown();

  static Program* get_instance() {
    return _instance;
  }
  Gtk::Window* get_mainwindow() const;

private: // Callbacks for backend
  int confirm_action_becb(const std::string& title, const std::string& msg, const std::string& default_btn,
                          const std::string& alt_btn, const std::string& other_btn);
  std::string show_file_dialog_becb(const std::string& type, const std::string& title, const std::string& extensions);

  void finalize_initialization(wb::WBOptions* options);

  bool idle_stuff();

private:
  std::deque<sigc::connection> _idleConnections;
  MainForm* _main_form;

  // sigc::signal<void>::iterator idle_signal_conn;
  sigc::connection _sig_finalize_initialization;
  sigc::connection _idle_signal_conn;
  Program(const Program&) = delete;
  Program& operator=(const Program&) = delete;
  static Program* _instance;
  wb::WBFrontendCallbacks *_wbcallbacks;
};

#endif

//!
//! @}
//!
