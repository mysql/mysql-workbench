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

#include "mforms/mforms.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("mforms")

using namespace mforms;
using namespace base;

// Implementation of _app_impl for WB is done directly in main_window.cpp in Linux
// and WBMainWindow.mm in Mac
// In Windows, it's done in wf_app.h/wf_app.cpp wrapper

static App *singleton = 0;

App::App(DockingPointDelegate *delegate, bool delete_on_destroy)
  : DockingPoint(delegate, delete_on_destroy), _app_impl(nullptr) {
}

//--------------------------------------------------------------------------------------------------

void App::instantiate(DockingPointDelegate *delegate, bool delete_on_destroy) {
  singleton = new App(delegate, delete_on_destroy);
  singleton->_app_impl = &ControlFactory::get_instance()->_app_impl;
}

//--------------------------------------------------------------------------------------------------

App *App::get() {
  return singleton;
}

//--------------------------------------------------------------------------------------------------

std::string App::get_resource_path(const std::string &file) {
  std::string ret;
  if (_app_impl->get_resource_path)
    ret = _app_impl->get_resource_path(this, file);
  if (ret == "")
      logWarning("Resource file not found: %s\n", file.c_str());
  return ret;
}

//--------------------------------------------------------------------------------------------------

std::string App::get_executable_path(const std::string &file) {
  std::string ret;
  if (_app_impl->get_executable_path)
    ret = _app_impl->get_executable_path(this, file);
  else
    ret = get_resource_path(file);
  return ret;
}

//--------------------------------------------------------------------------------------------------

void App::set_status_text(const std::string &text) {
  if (_app_impl->set_status_text)
    _app_impl->set_status_text(this, text);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the bounds of the main application window.
 */
base::Rect App::get_application_bounds() {
  return _app_impl->get_application_bounds(this);
}

//--------------------------------------------------------------------------------------------------

int App::enter_event_loop(float timeout) {
  return _app_impl->enter_event_loop(this, timeout);
}

//--------------------------------------------------------------------------------------------------

void App::exit_event_loop(int retcode) {
  _app_impl->exit_event_loop(this, retcode);
}

//--------------------------------------------------------------------------------------------------

float App::backing_scale_factor() {
  if (_app_impl->backing_scale_factor != nullptr)
    return _app_impl->backing_scale_factor(this);
  return 1.0;
}

//--------------------------------------------------------------------------------------------------

bool App::isDarkModeActive() {
  if (_app_impl->isDarkModeActive != nullptr)
    return _app_impl->isDarkModeActive(this);

  return false;
}

//--------------------------------------------------------------------------------------------------
