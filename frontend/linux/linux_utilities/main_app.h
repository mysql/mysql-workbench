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

#pragma once
#include <gtkmm/application.h>
#include <stack>
#include "base/threading.h"
#include "base/data_types.h"

namespace runtime {
  class loop {
    GMainLoop *_loop;

  public:
    loop();
    virtual ~loop();
    void run();
    void quit();
    bool isRunning() const;
  };

  class app {
  protected:
    GThread *_mainThread;
    Glib::RefPtr<Gtk::Application> _app;
    app();
    app(const app &) = delete;
    app &operator=(app &) = delete;

  public:
    static app &get();
    virtual ~app();
    void init(const std::string &name, int argc, char **argv);
    int onCommand(const Glib::RefPtr<Gio::ApplicationCommandLine> &appCmdLine);
    int run();
    void quit();
    bool isMainThread();

    std::function<void()> onActivate;

    /**
     * if this function is not empty, it should return true if application should be started
     */
    std::function<bool()> onBeforeActivate;
    std::function<dataTypes::OptionsList *()> getCmdOptions;
  };
}
