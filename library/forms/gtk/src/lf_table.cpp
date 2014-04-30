/*
 * Copyright (c) 2014 Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_table.h"
#include "base/string_utilities.h"

Gtk::Widget *mforms::gtk::TableImpl::get_outer() const
{
  return _alignment;
}
//------------------------------------------------------------------------------
Gtk::Widget *mforms::gtk::TableImpl::get_inner() const
{
  return _table;
}
//------------------------------------------------------------------------------
mforms::gtk::TableImpl::TableImpl(::mforms::Table *self) : ViewImpl(self)
{
  _alignment= new Gtk::Alignment();
  _table= new Gtk::Table();
  _table->show();
  _alignment->add(*_table);
  _alignment->show();
  _alignment->set(0.5, 0.5, 1.0, 1.0);
}

mforms::gtk::TableImpl::~TableImpl()
{
  delete _table;
  delete _alignment;
}
//------------------------------------------------------------------------------
bool mforms::gtk::TableImpl::create(::mforms::Table *self)
{
  return new TableImpl(self);
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_row_count(Table *self, int count)
{
  TableImpl *table= self->get_data<TableImpl>();

  table->_table->property_n_rows()= count;
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_col_count(Table *self, int count)
{
  TableImpl *table= self->get_data<TableImpl>();

  table->_table->property_n_columns()= count;
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::add(Table *self, View *child, int left, int right,
                int top, int bottom, int flags)
{
  TableImpl *table= self->get_data<TableImpl>();
  if (table)
  {
    if ((int)table->_table->property_n_columns() < right || (int)table->_table->property_n_rows() < bottom)
      throw std::logic_error(base::strfmt("Tried to use col %d, row %d, but the table has got only %d cols, %d rows.",
                              right, bottom, (int)table->_table->property_n_columns(), (int)table->_table->property_n_rows()));

    Gtk::AttachOptions roptions= (Gtk::AttachOptions)0, coptions= (Gtk::AttachOptions)0;

    roptions= Gtk::SHRINK;
    coptions= Gtk::SHRINK;

    if (flags & mforms::VExpandFlag)
      roptions|= Gtk::EXPAND;
    if (flags & mforms::VFillFlag)
      roptions|= Gtk::FILL;

    if (flags & mforms::HExpandFlag)
      coptions|= Gtk::EXPAND;
    if (flags & mforms::HFillFlag)
      coptions|= Gtk::FILL;

    table->_table->attach(*child->get_data<ViewImpl>()->get_outer(),
                                           left, right, top, bottom,
                                           coptions, roptions);
    child->show();
  }
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::remove(Table *self, View *child)
{
  TableImpl *table= self->get_data<TableImpl>();

  table->_table->remove(*child->get_data<ViewImpl>()->get_outer());
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_row_spacing(Table *self, int space)
{
  TableImpl *table= self->get_data<TableImpl>();

  table->_table->set_row_spacings(space);
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_col_spacing(Table *self, int space)
{
  TableImpl *table= self->get_data<TableImpl>();

  table->_table->set_col_spacings(space);
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_homogeneous(Table *self, bool flag)
{
  TableImpl *table= self->get_data<TableImpl>();

  table->_table->set_homogeneous(flag);
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::set_padding_impl(int left, int top, int right, int bottom)
{
  if (left < 0 && top < 0 && right < 0 && bottom < 0)
  {
    _alignment->set(0.5, 0.5, 0.0, 0.0);
    _table->set_border_width(0);
  }
  else
  {
    _alignment->set(0.5, 0.5, 1.0, 1.0);
    _table->set_border_width(left);
  }
}
//------------------------------------------------------------------------------
void mforms::gtk::TableImpl::init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_table_impl.create= &TableImpl::create;
  f->_table_impl.set_row_count= &TableImpl::set_row_count;
  f->_table_impl.set_column_count= &TableImpl::set_col_count;
  f->_table_impl.add= &TableImpl::add;
  f->_table_impl.remove= &TableImpl::remove;
  f->_table_impl.set_row_spacing= &TableImpl::set_row_spacing;
  f->_table_impl.set_column_spacing= &TableImpl::set_col_spacing;
  f->_table_impl.set_homogeneous= &TableImpl::set_homogeneous;
}
//------------------------------------------------------------------------------
