/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "main_app.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <gtkmm.h>
#pragma GCC diagnostic pop
#include <iostream>
#include "base/log.h"

runtime::loop::loop() : _loop(nullptr) {
}

runtime::loop::~loop() {
  if (_loop != nullptr) {
    static const char *const default_log_domain = "runtime";
    logError("loop d-tor: loop is still active calling loop::quit ");
    quit();
  }
}

void runtime::loop::run() {
  if (!runtime::app::get().isMainThread())
    throw std::runtime_error("Loops are only supported from inside of main thread");

  if (_loop != nullptr)
    throw std::runtime_error("Loop already started");

  _loop = g_main_loop_new(NULL, FALSE);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gdk_threads_leave();
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_main_loop_run(_loop);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gdk_threads_enter();
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_main_loop_unref(_loop);
  _loop = nullptr;
}

void runtime::loop::quit() {
  if (_loop != nullptr && g_main_loop_is_running(_loop))
    g_main_loop_quit(_loop);
}

bool runtime::loop::isRunning() const {
  return _loop != nullptr && g_main_loop_is_running(_loop);
}

runtime::app::app() {
  _mainThread = g_thread_self();
}

runtime::app &runtime::app::get() {
  static app app;
  return app;
}

runtime::app::~app() {
}

static GOptionArg convertOptionType(dataTypes::OptionArgumentType type) {
  switch (type) {
    case dataTypes::OptionArgumentNumeric:
      return G_OPTION_ARG_INT;
    case dataTypes::OptionArgumentText:
      return G_OPTION_ARG_STRING;
    case dataTypes::OptionArgumentFilename:
      return G_OPTION_ARG_FILENAME;
    case dataTypes::OptionArgumentLogical:
    default:
      return G_OPTION_ARG_NONE;
  }
}

void runtime::app::init(const std::string &name, int argc, char **argv) {
  auto cmdOptions = getCmdOptions();
  std::vector<GOptionEntry> entries;
  for (auto &o : *(cmdOptions->getEntries())) {
    entries.push_back({o.second.longName.c_str(), o.second.shortName, G_OPTION_FLAG_IN_MAIN,
                       convertOptionType(o.second.value.type), nullptr, o.second.description.c_str(),
                       o.second.argName.empty() ? nullptr : o.second.argName.c_str()});
    if (!o.second.callback) {
      switch (o.second.value.type) {
        case dataTypes::OptionArgumentNumeric:
          entries.back().arg_data = &o.second.value.numericValue;
          break;
        case dataTypes::OptionArgumentLogical:
          entries.back().arg_data = &o.second.value.logicalValue;
          break;
        case dataTypes::OptionArgumentFilename:
          entries.back().arg_data = &o.second.value.textValue;
          break;
        default:
          break;
      }
    }
  }

  entries.push_back({G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, nullptr, nullptr,"[PATH]"});

  entries.push_back({NULL, 0, 0, G_OPTION_ARG_NONE, nullptr, NULL, nullptr});
  _app = Gtk::Application::create(argc, argv, name, Gio::APPLICATION_HANDLES_COMMAND_LINE|Gio::APPLICATION_NON_UNIQUE);

  g_application_add_main_option_entries((GApplication *)_app->gobj(), entries.data());
  _app->signal_command_line().connect(sigc::mem_fun(this, &app::onCommand), false);

  _app->signal_activate().connect([&]() {
    bool activate = true;
    if (onBeforeActivate)
      activate = onBeforeActivate();

    if (!activate)
      return;

    _app->hold(); // we need to add additional app reference
                  // without it, the window will not be shown
    if (onActivate)
      onActivate();
  });
}

int runtime::app::onCommand(const Glib::RefPtr<Gio::ApplicationCommandLine> &appCmdLine) {
  auto optDict = appCmdLine->get_options_dict();
  auto cmdOptions = getCmdOptions();
  for (auto &o : *(cmdOptions->getEntries())) {
    if (o.second.value.type == dataTypes::OptionArgumentText ||
        o.second.value.type == dataTypes::OptionArgumentFilename) {
      Glib::ustring tmp;
      if (!optDict->lookup_value(o.second.longName.c_str(), tmp))
        continue;
      o.second.value.textValue = tmp.c_str();
    } else if (o.second.value.type == dataTypes::OptionArgumentLogical) {
      bool tmp;
      if (!optDict->lookup_value(o.second.longName.c_str(), tmp))
        continue;
      o.second.value.logicalValue = tmp;
    } else if (o.second.value.type == dataTypes::OptionArgumentNumeric) {
      int tmp;
      if (!optDict->lookup_value(o.second.longName.c_str(), tmp))
        continue;
      o.second.value.numericValue = tmp;
    }

    if (o.second.callback) {
      int retval = -1; // continue
      if (!o.second.callback(o.second, &retval))
        return retval;
    }
  }

  optDict->lookup_value(G_OPTION_REMAINING, cmdOptions->pathArgs);

  _app->activate();
  return EXIT_SUCCESS;
}

int runtime::app::run() {
  if (_app)
    return _app->run();
  return EXIT_FAILURE;
}

void runtime::app::quit() {
  if (_app)
    _app->quit();
}

bool runtime::app::isMainThread() {
  return _mainThread == g_thread_self();
}
