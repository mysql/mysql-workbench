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

#include "base/string_utilities.h"
#include "base/log.h"
#include "mforms/mforms.h"

// DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE);

using namespace mforms;

//----------------------------------------------------------------------------------------------------------------------

static int _serial = 0;

#ifdef _MSC_VER

AppView::AppView(bool horiz, const std::string &accessibilityName, const std::string &context_name, bool is_main)
  : Box(horiz), _context_name(context_name), _menubar(nullptr), _toolbar(nullptr), _is_main(is_main) {

  _app_view_impl = &ControlFactory::get_instance()->_app_view_impl;
  if (_app_view_impl && _app_view_impl->create)
    _app_view_impl->create(this, horiz);

  _identifier = base::strfmt("avid%i", ++_serial);
  _dpoint = NULL;
  set_name(accessibilityName);
  setInternalName(context_name);
}

#else

AppView::AppView(bool horiz, const std::string &accessibilityName, const std::string &context_name, bool is_main)
  : Box(horiz), _context_name(context_name), _menubar(nullptr), _toolbar(nullptr), _is_main(is_main) {
  set_name(accessibilityName);
  setInternalName(context_name);

  _identifier = base::strfmt("avid%i", ++_serial);
  _dpoint = 0;
}

#endif

//----------------------------------------------------------------------------------------------------------------------

AppView::~AppView() {
  if (_menubar)
    _menubar->release();
  if (_toolbar)
    _toolbar->release();
}

//----------------------------------------------------------------------------------------------------------------------

void AppView::set_containing_docking_point(mforms::DockingPoint *dpoint) {
  _dpoint = dpoint;
}

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

void AppView::set_title(const std::string &title) {
  _title = title;
  if (_dpoint)
    _dpoint->set_view_title(this, title);
}

//----------------------------------------------------------------------------------------------------------------------

std::string AppView::get_title() {
  return _title;
}

//----------------------------------------------------------------------------------------------------------------------

bool AppView::on_close() {
  if (_on_close_slot)
    return _on_close_slot();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void AppView::close() {
  if (_dpoint)
    _dpoint->undock_view(this);
}

//----------------------------------------------------------------------------------------------------------------------
