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

using namespace mforms;

static Form *current_active_form = NULL;

//--------------------------------------------------------------------------------------------------

Form::Form(Form *owner, FormFlag flag) {
  _form_impl = &ControlFactory::get_instance()->_form_impl;

  _menu = NULL;
  _content = NULL;
  _fixed_size = false;
  _release_on_close = false;
  _active = true;
  _form_impl->create(this, owner, flag);
}

//--------------------------------------------------------------------------------------------------

Form::Form() {
  _form_impl = &ControlFactory::get_instance()->_form_impl;
  _menu = NULL;
  _content = NULL;
  _fixed_size = false;
  _release_on_close = false;
  _active = true;
}

//--------------------------------------------------------------------------------------------------

Form *Form::main_form() {
  static Form *main_form = new Form();
  // the platform specific code should initialize the main_form stub with whatever it wants (namely,
  // call set_data() with a pointer to the real main window
  return main_form;
}

//--------------------------------------------------------------------------------------------------

Form::~Form() {
  if (_menu)
    _menu->release();
  if (current_active_form == this)
    current_active_form = NULL;
  if (_content != NULL)
    _content->release();
}

//--------------------------------------------------------------------------------------------------

void Form::set_menubar(MenuBar *menu) {
  if (!_content || !dynamic_cast<Box *>(_content))
    throw std::logic_error("set_menubar() must be called on a window with a Box as it's toplevel content");

  if (menu != _menu) {
    if (_menu)
      _menu->release();
    _menu = menu;
    _menu->retain();

    _form_impl->set_menubar(this, menu);
  }
}

//--------------------------------------------------------------------------------------------------

void Form::set_title(const std::string &title) {
  if (_form_impl)
    _form_impl->set_title(this, title);
}

//--------------------------------------------------------------------------------------------------

void Form::set_release_on_close(bool flag) {
  if (_form_impl)
    _release_on_close = flag;
}

//--------------------------------------------------------------------------------------------------

bool Form::run_modal(Button *accept, Button *cancel) {
  if (_form_impl)
    return _form_impl->run_modal(this, accept, cancel);
  return false;
}

//--------------------------------------------------------------------------------------------------

void Form::show_modal(Button *accept, Button *cancel) {
  if (_form_impl)
    _form_impl->show_modal(this, accept, cancel);
}

//--------------------------------------------------------------------------------------------------

void Form::end_modal(bool result) {
  if (_form_impl)
    _form_impl->end_modal(this, result);
}

//--------------------------------------------------------------------------------------------------

void Form::close() {
  if (_form_impl)
    _form_impl->close(this);
}

//--------------------------------------------------------------------------------------------------

void Form::center() {
  if (_form_impl)
    _form_impl->center(this);
}

//--------------------------------------------------------------------------------------------------

void Form::set_content(View *view) {
  if (_content != view && _form_impl) {
    if (_content)
      _content->release();
    _content = view;
    if (!_content->release_on_add())
      _content->retain();
    _form_impl->set_content(this, view);
    _content->set_parent(this);

    // Note: we are not adding the content view to the underlying view's child list and hence
    //       have to take care for ref counting here.
  }
}

//--------------------------------------------------------------------------------------------------

void Form::flush_events() {
  if (_form_impl)
    _form_impl->flush_events(this);
}

//--------------------------------------------------------------------------------------------------

void Form::activated() {
  current_active_form = this;
  _active = true;
  _activated_signal();
}

//--------------------------------------------------------------------------------------------------

void Form::deactivated() {
  _active = false;
  _deactivated_signal();
}

//--------------------------------------------------------------------------------------------------

bool Form::is_active() {
  return _active;
}

//--------------------------------------------------------------------------------------------------

bool Form::can_close() {
  if (_can_close_slot)
    return _can_close_slot();
  return true;
}

//--------------------------------------------------------------------------------------------------

Form *Form::active_form() {
  return current_active_form;
}
