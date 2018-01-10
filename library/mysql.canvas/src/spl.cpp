/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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


#include <gtkmm.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <gdk/gdkx.h>

#include "mdc.h"
#include "mdc_canvas_view_x11.h"
#include "gtk/mdc_gtk_canvas_view.h"
#include "gtk/mdc_gtk_canvas_scroller.h"
#include <sys/time.h>

using namespace mdc;

class Thing : public mdc::Box {
  mdc::RectangleFigure rect;
  mdc::Box title_bar;
  mdc::RectangleFigure title_back;
  mdc::IconTextFigure title;
  mdc::Button title_expander;

  mdc::Box column_box;

  cairo_surface_t *column_icon;
  cairo_surface_t *key_icon;

  std::vector<mdc::IconTextFigure *> columns;

public:
  Thing(mdc::Layer *layer)
    : mdc::Box(layer, Box::Vertical),
      rect(layer),
      title_bar(layer, Box::Horizontal),
      title_back(layer),
      title(layer),
      title_expander(layer, ExpanderButton),
      column_box(layer, Box::Vertical, true) {
    ImageManager *im = ImageManager::get_instance();

    column_icon = im->get_image("column.png");
    key_icon = im->get_image("column_pk.png");

    set_allowed_resizing(false, false);

    set_background(&rect);
    set_accepts_focus(true);
    set_accepts_selection(true);

    rect.set_fill_color(Color(1, 1, 1));
    rect.set_pen_color(Color(0.5, 0.5, 0.5));
    rect.set_filled(true);
    rect.set_rounded_corners(8, CTopLeft | CTopRight);
    rect.set_has_shadow(true);

    add(&title_bar, false, false, true);
    title_bar.set_padding(6, 4);
    title_bar.add(&title, true, true);
    title_bar.set_background(&title_back);
    title_back.set_fill_color(Color(0.5, 0.7, 0.83));
    title_back.set_pen_color(Color(0.5, 0.5, 0.5));
    title_back.set_filled(true);
    title_back.set_rounded_corners(8, CTopLeft | CTopRight);

    title_bar.set_draggable(true);

    title.set_icon(im->get_image("db.Table.16x16.png"));
    title.set_font(mdc::FontSpec("helvetica", SNormal, WBold, 11));
    title.set_text("Hello World");

    //    title_expander.set_back_color(Color(0.4,0.4,0.4));
    title_expander.signal_activate().connect(sigc::mem_fun(this, &Thing::toggle_expander));
    title_bar.add(&title_expander, false, true);
    title_expander.set_active(true);

    add(&column_box, false, true, true);
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

  void toggle_expander() {
    if (title_expander.get_active())
      column_box.set_visible(true);
    else
      column_box.set_visible(false);
  }

  void add_column(const std::string &text, cairo_surface_t *icon) {
    mdc::IconTextFigure *tf;

    tf = new mdc::IconTextFigure(_layer);
    tf->set_icon(icon);
    tf->set_spacing(1);
    tf->set_font(mdc::FontSpec("helvetica", SNormal, WNormal, 10));
    tf->set_text(text);

    column_box.add(tf, false, true);
  }
};

#include "wbcanvas/table_figure.h"

int main(int argc, char **argv) {
  Gtk::Main main(argc, argv);
  Gtk::Window window(Gtk::WINDOW_TOPLEVEL);
  Gtk::Notebook note;
  GtkCanvas *canvas;
  GtkCanvasScroller *scroller;
  mdc::CanvasView *cv;

  window.add(note);
  window.show_all();

  for (int i = 0; i < 1; i++) {
    ImageManager *im = ImageManager::get_instance();
    im->add_search_path("../../../images/grt/structs");
    im->add_search_path("../../../images/icons");

    canvas = new GtkCanvas(false);
    scroller = new GtkCanvasScroller();

    window.set_default_size(1024, 768);
    scroller->add(canvas);

    note.add(*scroller);

    canvas->set_scroll_adjustments(scroller->get_hadjustment(), scroller->get_vadjustment());

    cv = canvas->get_canvas();
    cv->set_page_size(Size(210 * 5, 297 * 5));
    // cv->set_page_size(Size(600, 400));
    cv->set_page_layout(1, 1);

#if 0
  for (int i=0; i < 10; i++)
{
  wbfig::Table *thing= new wbfig::Table(cv->get_current_layer());
  cv->get_current_layer()->add_item(thing);
  thing->move_to(Point(100, 100));
}
#endif

    RectangleFigure r(cv->get_current_layer());

    cv->get_current_layer()->add_item(&r);
    r.move_to(Point(100, 200));
    r.resize_to(Size(50, 100));
    r.set_pen_color(Color(1, 0, 0));
    r.set_line_width(2);
    r.set_filled(true);
    r.set_fill_color(Color(1, 1, 0));
  }
  /*
  for (int i= 0; i < 100; i++)
  {
    RectangleFigure *r= new RectangleFigure(cv->get_current_layer(), 0);
    r->set_fixed_size(Size(rand()%100+1, rand()%200+1));

    ToplevelItem *item= cv->get_current_layer()->add_wrap_item(r);

    item->set_pos(Point(rand()%800, rand()%600));
  }
*/

  main.run();
}
