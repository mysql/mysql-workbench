/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/app.h"

namespace MySQL {
  namespace Forms {

    ref class ManagedDockDelegate;

  public
    enum class AppCommand {
      AppQuit,
      AppGetResourcePath,
      AppSetStatusText,
    };

  public
    delegate String ^ AppCommandDelegate(AppCommand, String ^ str);

    // The creation here is in reverse order compared to most other mforms wrapper classes.
    // The program creates a ManagedApplication instance which then creates an unmanaged wrapper
    // for the backend. Only the docking delegate has a similar approach.
  public
    ref class ManagedApplication {
    private:
      AppCommandDelegate ^ commandDelegate;
      ManagedDockDelegate ^ dockingDelegate;

    public:
      ManagedApplication(AppCommandDelegate ^ app_command, ManagedDockDelegate ^ docking_delegate);
      ~ManagedApplication();

      std::string CallAppDelegate(AppCommand command, const std::string &str);
      String ^ CallAppDelegate(AppCommand command, String ^ str);
    };

  public
    class AppWrapper {
    private:
      gcroot<ManagedApplication ^> application;

    protected:
      static std::string get_resource_path(mforms::App *app, const std::string &file);
      static void set_status_text(mforms::App *app, const std::string &text);
      static base::Rect get_application_bounds(mforms::App *app);
      static int enter_event_loop(mforms::App *app, float max_wait_time);
      static void exit_event_loop(mforms::App *app, int ret_code);
      static bool isDarkModeActive(mforms::App *app);

    public:
      AppWrapper(ManagedApplication ^ managed);

      static String ^ get_image_path(String ^ path);

      static void init();
    };
  };
};
