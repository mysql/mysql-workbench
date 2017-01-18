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

#ifndef _TEST_FIGURE_H_
#define _TEST_FIGURE_H_

using namespace mdc;
using namespace base;

class Thing : public mdc::Box {
  mdc::Box title_bar;
  mdc::IconTextFigure title;
  mdc::TextFigure title_expander;

  mdc::Box column_box;

  cairo_surface_t *column_icon;
  cairo_surface_t *key_icon;

  std::vector<mdc::IconTextFigure *> columns;

public:
  Thing(mdc::Layer *layer)
    : mdc::Box(layer, Box::Vertical),
      title_bar(layer, Box::Horizontal),
      title(layer),
      title_expander(layer),
      column_box(layer, Box::Vertical, true) {
    column_icon =
      cairo_image_surface_create_from_png("/Users/kojima/Development/mysql-workbench-pro/images/grt/column.png");
    key_icon =
      cairo_image_surface_create_from_png("/Users/kojima/Development/mysql-workbench-pro/images/grt/column_pk.png");

    set_accepts_focus(true);
    set_accepts_selection(true);

    set_background_color(Color(1, 1, 1));
    set_border_color(Color(0.5, 0.5, 0.5));
    set_draw_background(true);

    add(&title_bar, false, false);
    title_bar.set_padding(4, 4);
    title_bar.add(&title, true, true);
    title_bar.set_background_color(Color(0.5, 0.7, 0.83));
    title_bar.set_border_color(Color(0.5, 0.5, 0.5));
    title_bar.set_draw_background(true);

    title.set_icon(cairo_image_surface_create_from_png(
      "/Users/kojima/Development/mysql-workbench-pro/images/grt/db.Table.12x12.png"));
    title.set_font(mdc::FontSpec("Lucida Grande", SNormal, WBold, 10));
    title.set_text("Hello World");

    title_expander.set_fixed_size(Size(10, -1));
    title_expander.set_text(">");
    title_bar.add(&title_expander, false, true);

    add(&column_box, false, true);
    column_box.set_spacing(2);
    column_box.set_padding(3, 3);

    add_column("id int primary key", key_icon);
    add_column("name varchar(32)", column_icon);
    add_column("address varchar(200)", column_icon);
    add_column("city int", column_icon);
    add_column("country int", column_icon);
    add_column("phone varchar(40)", column_icon);
    add_column("email varchar(80)", column_icon);
  }

  void add_column(const std::string &text, cairo_surface_t *icon) {
    mdc::IconTextFigure *tf;

    tf = new mdc::IconTextFigure(_layer);
    tf->set_icon(icon);
    tf->set_spacing(1);
    tf->set_font(mdc::FontSpec("Lucida Grande", SNormal, WNormal, 10));
    tf->set_text(text);

    column_box.add(tf, false, true);
  }
};
#endif /* _TEST_FIGURE_H_ */
