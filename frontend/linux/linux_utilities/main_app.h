/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
