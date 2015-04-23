/*
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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
#include "../lf_mforms.h"
#include "../lf_box.h"
#include "base/util_functions.h"

//------------------------------------------------------------------------------
mforms::gtk::BoxImpl::BoxImpl(::mforms::Box *self, bool horiz)
  : ViewImpl(self)
{
  if (horiz)
    _box = new Gtk::HBox();
  else
    _box = new Gtk::VBox();

  _alignment = new Gtk::Alignment();
  _alignment->add(*_box);
  _alignment->show_all();

  _box->signal_draw().connect(sigc::bind(sigc::ptr_fun(mforms::gtk::draw_event_slot), _box), false);
}

//------------------------------------------------------------------------------
bool mforms::gtk::BoxImpl::create(::mforms::Box *self, bool horiz)
{
  return new BoxImpl(self, horiz);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::add(Box *self, View *child, bool expand, bool fill)
{
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_box->pack_start(*child->get_data<ViewImpl>()->get_outer(), expand, fill);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::add_end(Box *self, View *child, bool expand, bool fill)
{
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_box->pack_end(*child->get_data<ViewImpl>()->get_outer(), expand, fill);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::remove(Box *self, View *child)
{
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_box->remove(*child->get_data<ViewImpl>()->get_outer());
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::set_homogeneous(Box *self, bool flag)
{
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_box->set_homogeneous(flag);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::set_spacing(Box *self, int spc)
{
  BoxImpl *box = self->get_data<BoxImpl>();

  box->_box->set_spacing(spc);
}

//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::set_padding_impl(int left, int top, int right, int bottom)
{
  _alignment->set_padding(top, bottom, left, right);
}

//------------------------------------------------------------------------------
mforms::gtk::BoxImpl::~BoxImpl()
{
  delete _box;
  delete _alignment;
}
//------------------------------------------------------------------------------
void mforms::gtk::BoxImpl::init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_box_impl.create = &BoxImpl::create;
  f->_box_impl.add = &BoxImpl::add;
  f->_box_impl.add_end = &BoxImpl::add_end;
  f->_box_impl.remove = &BoxImpl::remove;
  f->_box_impl.set_homogeneous = &BoxImpl::set_homogeneous;
  f->_box_impl.set_spacing = &BoxImpl::set_spacing;
  //f->_box_impl.set_padding = &BoxImpl::set_padding;
}
