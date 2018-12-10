/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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
    bool (*isDarkModeActive)(App *app);
  };
#endif
#endif

  /** Proxy class for interfacing with the host application window.

   Provides some utility functions to perform certain actions in the main window
   of the host application. This class uses a singleton.
   */
  class MFORMS_EXPORT App : public DockingPoint {
  private:
    App() : DockingPoint(), _app_impl(nullptr) {
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

    std::string baseDir() {
      return _baseDir;
    }

    /** Gets the bounds of the main window. */
    base::Rect get_application_bounds();

    /** Enters the event loop and exit only once exit_event_loop() is called.
     @param timeout Given in seconds. If > 0.0, the function will return -1 after that time.
       */
    int enter_event_loop(float timeout = 0.0);

    /** Exits from enter_event_loop() */
    void exit_event_loop(int retcode);

    /** The scale factor for the current screen. Usually 1, but e.g. on macOS retina screens can be 2. */
    float backing_scale_factor();

    /** Returns true if the OS currently shows in dark mode (Windows + macOS). */
    bool isDarkModeActive();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
    void set_user_data_folder_path(const std::string &path) {
      _user_data_folder = path;
    }

    void setBaseDir(const std::string &path) {
      _baseDir = path;
    }
#endif
#endif
  protected:
    AppImplPtrs *_app_impl;

  private:
    std::string _user_data_folder;
    std::string _baseDir;
  };
};
