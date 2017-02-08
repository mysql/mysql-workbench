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

#include "mforms/mforms.h"

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
  if (_app_impl->backing_scale_factor != NULL)
    return _app_impl->backing_scale_factor(this);
  return 1.0;
}

//--------------------------------------------------------------------------------------------------
