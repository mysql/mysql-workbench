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

#include "routine_group_figure.h"

using namespace wbfig;
using namespace base;

RoutineGroup::RoutineGroup(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self)
  : BaseFigure(layer, hub, self),
    _title(layer, hub, this, true),
    _footer(layer, hub, this, false),
    _content_box(layer, mdc::Box::Vertical) {
  _title.set_icon(mdc::ImageManager::get_instance()->get_image("workbench.physical.RoutineGroupFigure.16x16.png"));

  scoped_connect(_title.signal_expand_toggle(), std::bind(&RoutineGroup::toggle, this, std::placeholders::_1));

  set_allowed_resizing(false, false);
  set_accepts_focus(true);
  set_accepts_selection(true);

  set_border_color(Color(0.5, 0.5, 0.5));
  set_draw_background(true);
  set_background_color(Color::white());
  set_background_corners(mdc::CAll, 8.0);

  _title.set_rounded(mdc::CTop);
  _title.set_draggable(true);
  _title.set_expanded(true);
  _title.set_has_shadow(true);
  _title.set_title("View");
  _title.set_font(mdc::FontSpec("Helvetica", mdc::SNormal, mdc::WBold, 12));
  _title.set_color(Color(0.59, 0.85, 0.59));
  add(&_title, false, false, true);

  _content_box.set_spacing(1);
  add(&_content_box, true, true, true);

  _footer.set_rounded(mdc::CBottom);
  _footer.set_draggable(true);
  _footer.set_expanded(true);
  _footer.set_has_shadow(true);
  _footer.set_title("0 routines");
  _footer.set_font(mdc::FontSpec("Helvetica", mdc::SNormal, mdc::WNormal, 9));
  _footer.set_text_color(Color(0.5, 0.5, 0.5));
  _footer.set_color(Color(0.59, 0.85, 0.59));
  add(&_footer, false, false, true);
}

RoutineGroup::~RoutineGroup() {
  for (ItemList::iterator i = _routines.begin(); i != _routines.end(); ++i)
    delete *i;
}

void RoutineGroup::set_title(const std::string &title, const std::string &subtitle) {
  _title.set_title(title);
  _footer.set_title(subtitle);
}

void RoutineGroup::set_title_font(const mdc::FontSpec &font) {
  _title.set_font(font);
}

void RoutineGroup::set_content_font(const mdc::FontSpec &font) {
  super::set_content_font(font);

  for (ItemList::iterator i = _routines.begin(); i != _routines.end(); ++i)
    (*i)->set_font(font);
}

void RoutineGroup::set_color(const Color &color) {
  _title.set_color(color);
  _footer.set_color(color);
  set_needs_render();
}

void RoutineGroup::toggle(bool flag) {
  _title.set_expanded(flag);
  _content_box.set_visible(flag);
}

RoutineGroup::ItemList::iterator RoutineGroup::begin_routines_sync() {
  return begin_sync(_content_box, _routines);
}

RoutineGroup::ItemList::iterator RoutineGroup::sync_next_routine(ItemList::iterator iter, const std::string &id,
                                                                 const std::string &text) {
  return sync_next(_content_box, _routines, iter, id, 0, text);
}

void RoutineGroup::end_routines_sync(ItemList::iterator iter) {
  end_sync(_content_box, _routines, iter);
}
