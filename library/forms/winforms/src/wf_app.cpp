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

/**
 * Contains the implementation of the .NET wrapper for the mforms application class.
 */

#include "base/log.h"

#include "wf_base.h"
#include "wf_utilities.h"
#include "wf_dockingpoint.h"
#include "wf_app.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::IO;
using namespace System::Threading;

using namespace MySQL;
using namespace MySQL::Forms;

//----------------- ManagedApplication -------------------------------------------------------------

ManagedApplication::ManagedApplication(AppCommandDelegate ^ app_command, ManagedDockDelegate ^ docking_delegate) {
  logDebug("Creating application wrapper\n");

  dockingDelegate = docking_delegate;
  mforms::App::instantiate(dockingDelegate->get_unmanaged_delegate(), false);

  AppWrapper *wrapper = new AppWrapper(this);
  mforms::App::get()->set_data(wrapper);
  commandDelegate = app_command;
}

//--------------------------------------------------------------------------------------------------

ManagedApplication::~ManagedApplication() {
  logDebug("Destroyed backend application wrapper\n");
}

//--------------------------------------------------------------------------------------------------

std::string ManagedApplication::CallAppDelegate(AppCommand command, const std::string &str) {
  String ^ result = commandDelegate(command, CppStringToNative(str));
  return NativeToCppString(result);
}

//--------------------------------------------------------------------------------------------------

String ^ ManagedApplication::CallAppDelegate(AppCommand command, String ^ str) {
  return commandDelegate(command, str);
}

//----------------- AppWrapper ---------------------------------------------------------------------

AppWrapper::AppWrapper(ManagedApplication ^ managed) {
  application = managed;
}

//--------------------------------------------------------------------------------------------------

/**
 * Attempts to find the full path to the given image file. If the given path is already an absolute
 * one then it is returned as is. Otherwise the C# frontend is queried to find the file in any of the
 * (image) resource paths and if found a full path is constructed.
 * Note: this is a pure platform function and hence requires no string conversion like the (similar)
 *       get_resource_path function, which is used by the backend.
 */
String ^ AppWrapper::get_image_path(String ^ path) {
  if (Path::IsPathRooted(path))
    return path;

  AppWrapper *wrapper = mforms::App::get()->get_data<AppWrapper>();
  return wrapper->application->CallAppDelegate(AppCommand::AppGetResourcePath, path);
}

//--------------------------------------------------------------------------------------------------

std::string AppWrapper::get_resource_path(mforms::App *app, const std::string &file) {
  AppWrapper *wrapper = mforms::App::get()->get_data<AppWrapper>();
  return wrapper->application->CallAppDelegate(AppCommand::AppGetResourcePath, file);
}

//--------------------------------------------------------------------------------------------------

void AppWrapper::set_status_text(mforms::App *app, const std::string &text) {
  AppWrapper *wrapper = mforms::App::get()->get_data<AppWrapper>();
  wrapper->application->CallAppDelegate(AppCommand::AppSetStatusText, text);
}

//--------------------------------------------------------------------------------------------------

base::Rect AppWrapper::get_application_bounds(mforms::App *app) {
  System::Windows::Forms::Form ^ form = UtilitiesWrapper::get_mainform();
  Drawing::Rectangle bounds = form->Bounds;
  return base::Rect(bounds.Left, bounds.Top, bounds.Width, bounds.Height);
}

//--------------------------------------------------------------------------------------------------

bool AppWrapper::isDarkModeActive(mforms::App *app) {
  return false;
}

//--------------------------------------------------------------------------------------------------

static int message_loop_exit_code = MININT; // Can stay unguarded. We only use it from the main thread.

/**
 * Establishes a local event loop which is needed by the python debugger. This is potentially a very
 * dangerous and hard-to-debug thing so use it carefully.
 * Additionally, if you fail to call exit_event_loop the call will never return and block closing
 * the application.
 */
int AppWrapper::enter_event_loop(mforms::App *app, float max_wait_time) {
  message_loop_exit_code = -MININT;
  int remaining_milliseconds;
  if (max_wait_time <= 0)
    remaining_milliseconds = MAXINT;
  else
    remaining_milliseconds = (int)(1000 * max_wait_time);
  while (message_loop_exit_code == -MININT && remaining_milliseconds > 0) {
    Application::DoEvents();
    Thread::Sleep(100);
    remaining_milliseconds -= 100;
  }
  if (remaining_milliseconds == 0 || message_loop_exit_code == -MININT)
    return -1;
  return message_loop_exit_code;
}

//--------------------------------------------------------------------------------------------------

void AppWrapper::exit_event_loop(mforms::App *app, int ret_code) {
  message_loop_exit_code = ret_code;
}

//--------------------------------------------------------------------------------------------------

void AppWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_app_impl.get_resource_path = &get_resource_path;
  f->_app_impl.set_status_text = &set_status_text;
  f->_app_impl.get_application_bounds = &get_application_bounds;
  f->_app_impl.enter_event_loop = &enter_event_loop;
  f->_app_impl.exit_event_loop = &exit_event_loop;
  f->_app_impl.isDarkModeActive = &isDarkModeActive;

}

//--------------------------------------------------------------------------------------------------
