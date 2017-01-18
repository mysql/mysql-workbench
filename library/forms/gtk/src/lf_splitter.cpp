/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "../lf_splitter.h"

mforms::gtk::SplitterImpl::SplitterImpl(::mforms::Splitter *self, bool horiz) : ViewImpl(self) {
  _paned = new Gtk::Paned(horiz ? Gtk::ORIENTATION_HORIZONTAL : Gtk::ORIENTATION_VERTICAL);
  _paned->property_position().signal_changed().connect(sigc::mem_fun(self, &mforms::Splitter::position_changed));
  _paned->show();
}

bool mforms::gtk::SplitterImpl::create(::mforms::Splitter *self, bool horiz) {
  return new SplitterImpl(self, horiz);
}

void mforms::gtk::SplitterImpl::add(Splitter *self, View *child, int minwidth, bool fixed) {
  SplitterImpl *splitview = self->get_data<SplitterImpl>();

  if (!splitview->_paned->get_child1())
    splitview->_paned->pack1(*child->get_data<ViewImpl>()->get_outer(), !fixed, true);
  else
    splitview->_paned->pack2(*child->get_data<ViewImpl>()->get_outer(), !fixed, true);
}

void mforms::gtk::SplitterImpl::remove(Splitter *self, View *child) {
  SplitterImpl *splitview = self->get_data<SplitterImpl>();

  splitview->_paned->remove(*child->get_data<ViewImpl>()->get_outer());
}

void mforms::gtk::SplitterImpl::set_divider_position(Splitter *self, int pos) {
  SplitterImpl *splitview = self->get_data<SplitterImpl>();

  splitview->_paned->set_position(pos);
}

int mforms::gtk::SplitterImpl::get_divider_position(Splitter *self) {
  SplitterImpl *splitview = self->get_data<SplitterImpl>();

  return splitview->_paned->get_position();
}

void mforms::gtk::SplitterImpl::set_expanded(Splitter *self, bool first, bool expand) {
  SplitterImpl *sv = self->get_data<SplitterImpl>();

  if (sv && sv->_paned) {
    Gtk::Widget *child = first ? sv->_paned->get_child1() : sv->_paned->get_child2();
    if (child) {
      if (expand)
        child->show();
      else
        child->hide();
    }
  }
}

void mforms::gtk::SplitterImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_splitter_impl.create = &SplitterImpl::create;
  f->_splitter_impl.add = &SplitterImpl::add;
  f->_splitter_impl.remove = &SplitterImpl::remove;
  f->_splitter_impl.set_divider_position = &SplitterImpl::set_divider_position;
  f->_splitter_impl.get_divider_position = &SplitterImpl::get_divider_position;
  f->_splitter_impl.set_expanded = &SplitterImpl::set_expanded;
}

mforms::gtk::SplitterImpl::~SplitterImpl() {
  delete _paned;
}
