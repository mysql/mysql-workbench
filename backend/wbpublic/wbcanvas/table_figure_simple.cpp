/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "table_figure_simple.h"

using namespace wbfig;
using namespace mdc;
using namespace base;

SimpleTable::SimpleTable(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self)
  : Table(layer, hub, self, false), _column_box(layer, mdc::Box::Vertical) {
  set_allowed_resizing(true, true);
  set_accepts_focus(true);
  set_accepts_selection(true);
  magnetize_bounds();

  add(&_title, false, true, true);
  _title.set_border_color(Color::black());
  _title.set_font(mdc::FontSpec(_title.get_font().family, mdc::SNormal, mdc::WNormal, 12));

  _column_box.set_spacing(1);

  //_column_box.set_fixed_min_size(mdc::Size(-1, 20));

  set_border_color(Color::black());
  set_background_color(Color::white());
  set_draw_background(true);

  _barker = false;

  add(&_column_box, false, true, true);
}

void SimpleTable::set_color(const Color &color) {
  set_background_color(color);
  set_needs_render();
}

void SimpleTable::set_barker_notation(bool flag) {
  _barker = true;
}

void SimpleTable::set_dependant(bool flag) {
  if (flag)
    set_background_corners(mdc::CAll, 8.0);
  else
    set_background_corners(mdc::CNone, 0.0);
}

Table::ItemList::iterator SimpleTable::begin_columns_sync() {
  return begin_sync(_column_box, _columns);
}

Table::ItemList::iterator SimpleTable::sync_next_column(ItemList::iterator iter, const std::string &id,
                                                        ColumnFlags flags, const std::string &text) {
  std::string pref;

  if (_barker) {
    if (flags & wbfig::ColumnNotNull)
      pref = "\xe2\x97\x8f";
    else
      pref = "\xe2\x97\x8b";
  }

  if (flags & wbfig::ColumnPK) {
    if (flags & wbfig::ColumnFK)
      return sync_next(_column_box, _columns, iter, id, NULL, _barker ? "# " + text : text + " (FK)",
                       std::bind(&SimpleTable::create_column_item, this, std::placeholders::_1, std::placeholders::_2),
                       std::bind(&SimpleTable::update_column_item, this, std::placeholders::_1, flags));
    else
      return sync_next(_column_box, _columns, iter, id, NULL, _barker ? "# " + text : text,
                       std::bind(&SimpleTable::create_column_item, this, std::placeholders::_1, std::placeholders::_2),
                       std::bind(&SimpleTable::update_column_item, this, std::placeholders::_1, flags));
  } else if (flags & wbfig::ColumnFK)
    return sync_next(_column_box, _columns, iter, id, NULL, pref + text + " (FK)",
                     std::bind(&SimpleTable::create_column_item, this, std::placeholders::_1, std::placeholders::_2),
                     std::bind(&SimpleTable::update_column_item, this, std::placeholders::_1, flags));
  else
    return sync_next(_column_box, _columns, iter, id, NULL, pref + text,
                     std::bind(&SimpleTable::create_column_item, this, std::placeholders::_1, std::placeholders::_2),
                     std::bind(&SimpleTable::update_column_item, this, std::placeholders::_1, flags));
}

void SimpleTable::end_columns_sync(ItemList::iterator iter) {
  end_sync(_column_box, _columns, iter);
}
