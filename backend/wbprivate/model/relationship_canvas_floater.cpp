/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "relationship_canvas_floater.h"
#include "model/wb_model_diagram_form.h"
#include "base/string_utilities.h"

using namespace wb;
using namespace base;

RelationshipFloater::RelationshipFloater(ModelDiagramForm *view)
  : Floater(view->get_floater_layer(), _("Foreign Key Columns")),
    _columns_box(view->get_floater_layer(), mdc::Box::Vertical, true),
    _text(view->get_floater_layer()),
    _button(view->get_floater_layer()) {
  _text.set_multi_line(true);
  _text.set_pen_color(Color(0.8, 0.8, 0.8));
  _text.set_font(mdc::FontSpec("Helvetica", mdc::SNormal, mdc::WNormal, 11));
  _columns_box.set_spacing(4);
  _content_box.set_spacing(8);
  _content_box.add(&_columns_box, false, false);
  _content_box.add(&_text, false, false);
  _content_box.add(&_button, false, false);

  setup_pick_source();

  _content_box.set_needs_relayout();
}

RelationshipFloater::~RelationshipFloater() {
  for (std::vector<mdc::TextFigure *>::iterator iter = _columns.begin(); iter != _columns.end(); ++iter)
    delete *iter;
  _columns.clear();
}

void RelationshipFloater::setup_pick_source() {
  set_title(_("Foreign Key Columns"));
  _text.set_text(_("Pick one or more columns\nfor the foreign key."));
  //_text.set_needs_relayout();
  // set_needs_relayout();

  _button.set_text(_("Pick Referenced Columns"));
}

void RelationshipFloater::add_column(const std::string &name) {
  mdc::TextFigure *text = new mdc::TextFigure(get_layer());
  text->set_text(name);
  text->set_pen_color(Color::White());

  _columns.push_back(text);

  _columns_box.add(text, false, false);
}

void RelationshipFloater::setup_pick_target() {
  set_title(_("Referenced Columns"));
  _text.set_text(_("Pick matching columns for\nthe referenced table."));

  _button.set_visible(false);

  _current_column = 0;

  mdc::FontSpec font(_columns[_current_column]->get_font());
  font.toggle_bold(true);
  _columns[_current_column]->set_font(font);
}

void RelationshipFloater::pick_next_target() {
  mdc::FontSpec font(_columns[_current_column]->get_font());

  font.toggle_bold(true);
  _columns[_current_column]->set_font(font);

  _current_column++;
  if (_current_column < _columns.size()) {
    font.toggle_bold(false);
    _columns[_current_column]->set_font(font);
  }
}
