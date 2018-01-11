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

#include "table_figure.h"
#include "connection_figure.h"
/**
 * @file  table_figure.cpp
 * @brief
 */

using namespace wbfig;
using namespace base;

ItemMagnet::ItemMagnet(mdc::CanvasItem *owner) : mdc::Magnet(owner) {
}

double ItemMagnet::constrain_angle(double angle) const {
  if (angle > 90 && angle < 270)
    return 180;
  return 0;
}

void ItemMagnet::owner_parent_bounds_changed(mdc::CanvasItem *item, const Rect &obounds) {
  /*
  if (_owner->get_parents_visible())
  {
    Rect bounds(_owner->get_root_bounds());

    set_position(Point(bounds.xmin()+bounds.width()/2,
                            bounds.ymin()+bounds.height()/2));
  }
  else
  {
    Rect bounds(_owner->get_toplevel()->get_root_bounds());

    set_position(Point(bounds.xmin()+bounds.width()/2,
                            bounds.ymin()+bounds.height()/2));
  }*/
  notify_connected();
}

void ItemMagnet::owner_bounds_changed(
  const Rect &obounds) { /*
                          if (_owner->get_parents_visible())
                          {
                            Rect bounds(_owner->get_root_bounds());

                            set_position(Point(bounds.xmin()+bounds.width()/2,
                                                    bounds.ymin()+bounds.height()/2));
                          }
                          else
                          {
                            Rect bounds(_owner->get_toplevel()->get_root_bounds());

                            set_position(Point(bounds.xmin()+bounds.width()/2,
                                                    bounds.ymin()+bounds.height()/2));
                          }*/
  notify_connected();
}

//--------------------------------------------------------------------------------

TableColumnItem::TableColumnItem(mdc::Layer *layer, FigureEventHub *hub, Table *owner) : FigureItem(layer, hub, owner) {
  _flags = (ColumnFlags)0;
  _magnet = new ItemMagnet(this);
  add_magnet(_magnet);
}

void TableColumnItem::set_column_flags(ColumnFlags flags) {
  if (_flags != flags) {
    _flags = flags;
    set_needs_relayout();
  }
}

Size TableColumnItem::calc_min_size() {
  Size size = FigureItem::calc_min_size();
  cairo_text_extents_t extents;
  mdc::CairoCtx *cr = get_layer()->get_view()->cairoctx();

  std::vector<std::string> flags;
  if (_flags & ColumnUnsigned)
    flags.push_back("UN");
  if (_flags & ColumnNotNull)
    flags.push_back("NN");
  if (_flags & ColumnAutoIncrement)
    flags.push_back("AI");

  mdc::FontSpec font(get_font());
  font.size *= 0.7f;
  for (std::vector<std::string>::const_iterator iter = flags.begin(); iter != flags.end(); ++iter) {
    cr->get_text_extents(font, iter->c_str(), extents);
    size.width += ceil(extents.x_advance) + 3;
  }

  size.width = ceil(size.width);

  return size;
}

void TableColumnItem::draw_contents(mdc::CairoCtx *cr) {
  FigureItem::draw_contents(cr);

  cairo_text_extents_t extents;
  Size text_size(get_text_size());
  Point pos(get_position());
  Size size(get_size());
  float max_x;

  if (text_size.width >= size.width - 2 * _xpadding)
    return;

  pos.x = text_size.width + _xpadding;

  mdc::FontSpec font(get_font());
  font.size *= 0.7f;

  std::vector<std::string> flags;
  if (_flags & ColumnUnsigned)
    flags.push_back("UN");
  if (_flags & ColumnNotNull)
    flags.push_back("NN");
  if (_flags & ColumnAutoIncrement)
    flags.push_back("AI");

  max_x = (float)(size.width - _xpadding - (_icon ? cairo_image_surface_get_width(_icon) + _spacing : 0));

  cr->set_font(font);
  for (std::vector<std::string>::const_iterator iter = flags.begin(); iter != flags.end(); ++iter) {
    cr->get_text_extents(font, *iter, extents);

    pos.x += 3;
    cr->move_to(pos.x, pos.y + (size.height + text_size.height) / 2);

    if (pos.x + ceil(extents.x_advance) > max_x)
      break;

    cr->show_text(*iter);

    pos.x += ceil(extents.x_advance);
  }
  cr->stroke();
}

//-------------------------------------------------------------------------------------------

Table::Table(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self, bool collapsible)
  : BaseFigure(layer, hub, self), _background(layer), _title(layer, hub, this, collapsible) {
  _original_column_box_height = 0.0;

  _hide_columns = false;
  _hide_indexes = false;
  _hide_triggers = false;

  _show_flags = false;

  add_magnet(_sides_magnet = new mdc::BoxSideMagnet(this));
  _sides_magnet->set_compare_slot(std::bind(&Table::compare_connection_position, this, std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3));
}

void Table::set_title_font(const mdc::FontSpec &font) {
  _title.set_font(font);
}

void Table::set_section_font(const mdc::FontSpec &font) {
  if (get_index_title())
    get_index_title()->set_font(font);
  if (get_trigger_title())
    get_trigger_title()->set_font(font);
}

void Table::set_content_font(const mdc::FontSpec &font) {
  super::set_content_font(font);
}

void Table::set_show_flags(bool flag) {
  _show_flags = flag;
}

wbfig::FigureItem *Table::create_column_item(mdc::Layer *layer, FigureEventHub *hub) {
  return new TableColumnItem(layer, hub, this);
}

void Table::update_column_item(wbfig::FigureItem *item, ColumnFlags flags) {
  if (_show_flags)
    dynamic_cast<TableColumnItem *>(item)->set_column_flags(flags);
}

wbfig::FigureItem *Table::create_index_item(mdc::Layer *layer, FigureEventHub *hub) {
  return new FigureItem(layer, hub, this);
}

bool Table::compare_connection_position(mdc::Connector *a, mdc::Connector *b, mdc::BoxSideMagnet::Side side) {
  wbfig::ConnectionLineLayouter *layouter;
  Point a_pos, b_pos;
  mdc::CanvasItem *item;
  mdc::Connector *a_conn, *b_conn;

  layouter = dynamic_cast<wbfig::ConnectionLineLayouter *>(dynamic_cast<mdc::Line *>(a->get_owner())->get_layouter());
  if (!layouter)
    return false;
  if (layouter->get_start_connector() == a) {
    a_conn = layouter->get_end_connector();
    item = a_conn->get_connected_item();
  } else {
    a_conn = layouter->get_start_connector();
    item = a_conn->get_connected_item();
  }
  if (item)
    a_pos = item->get_root_bounds().center();
  else
    a_pos = a_conn->get_position();

  layouter = dynamic_cast<wbfig::ConnectionLineLayouter *>(dynamic_cast<mdc::Line *>(b->get_owner())->get_layouter());
  if (!layouter)
    return false;
  if (layouter->get_start_connector() == b) {
    b_conn = layouter->get_end_connector();
    item = b_conn->get_connected_item();
  } else {
    b_conn = layouter->get_start_connector();
    item = b_conn->get_connected_item();
  }
  if (item)
    b_pos = item->get_root_bounds().center();
  else
    b_pos = b_conn->get_position();

  // the strict ordering between 2 connectors is important
  // ie, compare(a,b) and compare(b,a) must never be the same
  // otherwise we get an infinite loop when a_pos == b_pos
  if (side == mdc::BoxSideMagnet::Top || side == mdc::BoxSideMagnet::Bottom)
    return a_pos.x < b_pos.x || (a_pos.x == b_pos.x && a_conn < b_conn);
  else
    return a_pos.y < b_pos.y || (a_pos.y == b_pos.y && a_conn < b_conn);
}
