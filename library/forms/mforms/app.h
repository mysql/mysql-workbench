/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <map>

#include "mforms/dockingpoint.h"
#include "base/geometry.h"
#include "base/drawing.h"

namespace mforms {

  class App;
  class AppView;
  class View;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct AppImplPtrs {
    void (*set_status_text)(App *app, const std::string &title);

    std::string (*get_resource_path)(App *app, const std::string &file);
    std::string (*get_executable_path)(App *app, const std::string &file);
    base::Rect (*get_application_bounds)(App *app);

    int (*enter_event_loop)(App *app, float max_wait_time);
    void (*exit_event_loop)(App *app, int result);

    float (*backing_scale_factor)(App *app);
  };
#endif
#endif

  /** Proxy class for interfacing with the host application window.

   Provides some utility functions to perform certain actions in the main window
   of the host application. This class uses a singleton.
   */
  class MFORMS_EXPORT App : public DockingPoint {
  private:
    App() : DockingPoint() {
    }

    App(DockingPointDelegate *delegate, bool delete_on_destroy);

  public:
#ifndef SWIG
    // for use by host application
    static void instantiate(DockingPointDelegate *delegate, bool delete_on_destroy);
#endif
  public:
    /** Gets the instance of the App class.
     */
    static App *get();

    /** Sets the status text at the bottom of the window.
     */
    void set_status_text(const std::string &text);

    /** Gets the path to a resource file bundled in the application.

     Passing an empty string will return the path to the resource directory. */
    std::string get_resource_path(const std::string &file);

    std::string get_executable_path(const std::string &file);

    std::string get_user_data_folder() {
      return _user_data_folder;
    }

    /** Gets the bounds of the main window. */
    base::Rect get_application_bounds();

    /** Enters the event loop and exit only once exit_event_loop() is called.
     @param timeout Given in seconds. If > 0.0, the function will return -1 after that time.
       */
    int enter_event_loop(float timeout = 0.0);

    /** Exits from enter_event_loop() */
    void exit_event_loop(int retcode);

    float backing_scale_factor();
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
    void set_user_data_folder_path(const std::string &path) {
      _user_data_folder = path;
    }
#endif
#endif
  protected:
    AppImplPtrs *_app_impl;

  private:
    std::string _user_data_folder;
  };
};
