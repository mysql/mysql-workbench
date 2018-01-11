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

#include "../lf_mforms.h"
#include "../lf_box.h"
#include "base/util_functions.h"

//------------------------------------------------------------------------------
mforms::gtk::BoxImpl::BoxImpl(::mforms::Box *self, bool horiz) : ViewImpl(self) {
  _innerBox = new Gtk::Box(horiz ? Gtk::ORIENTATION_HORIZONTAL : Gtk::ORIENTATION_VERTICAL);

  _innerBox->signal_draw().connect(sigc::bind(sigc::ptr_fun(mforms::gtk::draw_event_slot), _innerBox), false);
  setup();
}

//------------------------------------------------------------------------------
bool mforms::gtk::BoxImpl::create(::mforms::Box *self, bool horiz) {
  return new BoxImpl(self, horiz);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::add(Box *self, View *child, bool expand, bool fill) {
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_innerBox->pack_start(*child->get_data<ViewImpl>()->get_outer(), expand, fill);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::add_end(Box *self, View *child, bool expand, bool fill) {
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_innerBox->pack_end(*child->get_data<ViewImpl>()->get_outer(), expand, fill);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::remove(Box *self, View *child) {
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_innerBox->remove(*child->get_data<ViewImpl>()->get_outer());
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::set_homogeneous(Box *self, bool flag) {
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_innerBox->set_homogeneous(flag);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::set_spacing(Box *self, int spc) {
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_innerBox->set_spacing(spc);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::set_padding_impl(int left, int top, int right, int bottom) {
  _innerBox->set_margin_bottom(bottom);
  _innerBox->set_margin_top(top);
  _innerBox->set_margin_left(left);
  _innerBox->set_margin_right(right);
}

//------------------------------------------------------------------------------

void mforms::gtk::BoxImpl::set_size(int width, int height) {
  get_outer()->set_size_request(width, height);
  get_inner()->set_size_request(width, height);
}

//------------------------------------------------------------------------------
mforms::gtk::BoxImpl::~BoxImpl() {
  delete _innerBox;
}
//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_box_impl.create = &BoxImpl::create;
  f->_box_impl.add = &BoxImpl::add;
  f->_box_impl.add_end = &BoxImpl::add_end;
  f->_box_impl.remove = &BoxImpl::remove;
  f->_box_impl.set_homogeneous = &BoxImpl::set_homogeneous;
  f->_box_impl.set_spacing = &BoxImpl::set_spacing;
  // f->_box_impl.set_padding = &BoxImpl::set_padding;
}
