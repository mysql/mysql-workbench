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

#include "base/string_utilities.h"
#include "base/log.h"
#include "mforms/mforms.h"

// DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE);

using namespace mforms;

//--------------------------------------------------------------------------------------------------

static int _serial = 0;

#ifdef _WIN32

AppView::AppView(bool horiz, const std::string &context_name, bool is_main)
  : Box(horiz), _context_name(context_name), _menubar(0), _toolbar(0), _is_main(is_main) {
  _app_view_impl = &ControlFactory::get_instance()->_app_view_impl;
  if (_app_view_impl && _app_view_impl->create)
    _app_view_impl->create(this, horiz);

  _identifier = base::strfmt("avid%i", ++_serial);
  _dpoint = NULL;
  set_name(context_name);
}

#else

AppView::AppView(bool horiz, const std::string &context_name, bool is_main)
  : Box(horiz), _context_name(context_name), _menubar(0), _toolbar(0), _is_main(is_main) {
  set_name(context_name);
#ifdef __APPLE__
  // default, empty toolbar for mac, to show the 3px bar under the top tabs
  // TODO: move this to the platform layer. It doesn't belong here.
  _toolbar = new mforms::ToolBar(mforms::MainToolBar);
  set_back_color("#e8e8e8");
#endif
  _identifier = base::strfmt("avid%i", ++_serial);
  _dpoint = 0;
}

#endif

//--------------------------------------------------------------------------------------------------

AppView::~AppView() {
  if (_menubar)
    _menubar->release();
  if (_toolbar)
    _toolbar->release();
}

//--------------------------------------------------------------------------------------------------

void AppView::set_containing_docking_point(mforms::DockingPoint *dpoint) {
  _dpoint = dpoint;
}

//--------------------------------------------------------------------------------------------------

void AppView::set_menubar(mforms::MenuBar *menu) {
  if (_menubar != menu) {
    if (_menubar != NULL)
      _menubar->release();
    _menubar = menu;

    if (menu != NULL) {
      if (!menu->release_on_add())
        _menubar->retain();
      else
        _menubar->set_release_on_add(false);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void AppView::set_toolbar(mforms::ToolBar *toolbar) {
  if (_toolbar != toolbar) {
    if (_toolbar)
      _toolbar->release();
    _toolbar = toolbar;

    if (toolbar != NULL) {
      if (!_toolbar->release_on_add())
        _toolbar->retain();
      else
        _toolbar->set_release_on_add(false);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void AppView::set_title(const std::string &title) {
  _title = title;
  if (_dpoint)
    _dpoint->set_view_title(this, title);
}

//--------------------------------------------------------------------------------------------------

std::string AppView::get_title() {
  return _title;
}

//--------------------------------------------------------------------------------------------------

bool AppView::on_close() {
  if (_on_close_slot)
    return _on_close_slot();
  return true;
}

//--------------------------------------------------------------------------------------------------

void AppView::close() {
  if (_dpoint)
    _dpoint->undock_view(this);
}

//--------------------------------------------------------------------------------------------------
