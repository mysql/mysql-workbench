/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_radiobutton.h"

static std::map<int, Gtk::RadioButton *> groups;

Gtk::Widget *mforms::gtk::RadioButtonImpl::get_outer() const {
  return _radio;
}

mforms::gtk::RadioButtonImpl::RadioButtonImpl(::mforms::RadioButton *self, int group_id)
  : mforms::gtk::ButtonImpl(self), _group_id(group_id) {
  _radio = Gtk::manage(new Gtk::RadioButton());
  _radio->set_use_underline(false);
  _button = _radio;

  if (groups.find(group_id) != groups.end()) {
    Gtk::RadioButton::Group group(groups[group_id]->get_group());
    _radio->set_group(group);
  } else {
    groups[group_id] = _radio;
  }

  self->add_destroy_notify_callback(reinterpret_cast<void *>(group_id), &RadioButtonImpl::unregister_group);
  _radio->add_destroy_notify_callback(reinterpret_cast<void *>(group_id), &RadioButtonImpl::unregister_group);

  _radio->signal_toggled().connect(sigc::bind(sigc::ptr_fun(&RadioButtonImpl::toggled), self));
  _radio->show();
}

void *mforms::gtk::RadioButtonImpl::unregister_group(void *data) {
  int group_id = reinterpret_cast<intptr_t>(data);

  std::map<int, Gtk::RadioButton *>::iterator iter;

  if ((iter = groups.find(group_id)) != groups.end())
    groups.erase(iter);
  return NULL;
}

void mforms::gtk::RadioButtonImpl::toggled(::mforms::RadioButton *self) {
  if (!self->is_updating() && self->get_data<RadioButtonImpl>()->_radio->get_active())
    self->callback();
}

bool mforms::gtk::RadioButtonImpl::create(::mforms::RadioButton *self, int group_id) {
  return new RadioButtonImpl(self, group_id);
}

bool mforms::gtk::RadioButtonImpl::get_active(::mforms::RadioButton *self) {
  RadioButtonImpl *button = self->get_data<RadioButtonImpl>();

  if (button) {
    return button->_radio->get_active();
  }
  return false;
}

void mforms::gtk::RadioButtonImpl::set_active(::mforms::RadioButton *self, bool flag) {
  RadioButtonImpl *button = self->get_data<RadioButtonImpl>();

  if (button) {
    button->_radio->set_active(flag);
  }
}

void mforms::gtk::RadioButtonImpl::set_text(const std::string &text) {
  if (_label)
    _label->set_label(text);
  else
    _button->set_label(text);
}

void mforms::gtk::RadioButtonImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_radio_impl.create = &RadioButtonImpl::create;
  f->_radio_impl.get_active = &RadioButtonImpl::get_active;
  f->_radio_impl.set_active = &RadioButtonImpl::set_active;
}
