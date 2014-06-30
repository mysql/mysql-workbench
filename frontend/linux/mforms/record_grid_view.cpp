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

#include "record_grid_view.h"
#include "../sqlide/recordset_view.h"
#include "sqlide/recordset_be.h"

using namespace mforms;


static RecordGrid* create_record_grid(boost::shared_ptr<Recordset> rset)
{
  return new RecordGridView(rset);
}


void lf_record_grid_init()
{
  mforms::RecordGrid::register_factory(create_record_grid);
}



RecordGridView::RecordGridView(Recordset::Ref rset)
{
  viewer = RecordsetView::create(rset);
  viewer->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  set_data(viewer);
  viewer->show_all();
}

RecordGridView::~RecordGridView()
{
  delete viewer;
}

int RecordGridView::get_column_count()
{
  return viewer->grid_view()->get_columns().size()-1;
}

int RecordGridView::get_column_width(int column)
{
  Gtk::TreeViewColumn *tc = viewer->grid_view()->get_column(column);
  if (tc)
    return tc->get_width();
  return 0;
}


void RecordGridView::set_column_width(int column, int width)
{
  Gtk::TreeViewColumn *tc = viewer->grid_view()->get_column(column);
  if (tc)
    tc->set_fixed_width(width);
}


bool RecordGridView::current_cell(size_t &row, int &column)
{
  int r, c;
  if (viewer->grid_view()->current_cell(r, c).is_valid())
    return false;
  row = r;
  column = c;
  return true;
}


void RecordGridView::set_current_cell(size_t row, int column)
{
  viewer->grid_view()->select_cell(row, column);
}

