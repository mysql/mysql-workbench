/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc.h"
#include "mdc_canvas_view_image.h"

#include "casmine.h"

using namespace casmine;

namespace {

struct CairoSurface {
  std::string path;
  unsigned char *data;
  cairo_surface_t *surface;
  base::Size size;
  unsigned int stride;
  bool owner;

  CairoSurface() {
    data = nullptr;
    surface = nullptr;
    stride = 0;
    owner = false;
  }
  CairoSurface(const CairoSurface &other) {
    surface = other.surface;

    size.height = other.size.height;
    size.width = other.size.width;
    stride = other.stride;
    data = other.data;
    owner = false;
  }
  CairoSurface(const std::string &path) {
    surface = cairo_image_surface_create_from_png(path.c_str());

    size.height = cairo_image_surface_get_height(surface);
    size.width = cairo_image_surface_get_width(surface);
    stride = cairo_image_surface_get_stride(surface);
    data = cairo_image_surface_get_data(surface);
    owner = true;
  }

  ~CairoSurface() {
    if (surface && owner)
      cairo_surface_destroy(surface);
  }

  bool operator == (const CairoSurface &other) const {
    if (cairo_surface_status(surface) != cairo_surface_status(other.surface))
      return false;
    if (size != other.size)
      return false;
    if (stride != other.stride)
      return false;
    if (memcmp(data, other.data, size.height * stride) != 0)
      return false;
    return true;
  }

  std::string toString() const {
    return base::strfmt("{ width: %f, height: %f, stride: %d }", size.width, size.height, stride);
  }
};

$ModuleEnvironment() {};

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

    // TODO: Verify these urls
    column_icon =
      cairo_image_surface_create_from_png("/Users/kojima/Development/mysql-workbench-pro/images/grt/column.png");
    key_icon =
      cairo_image_surface_create_from_png("/Users/kojima/Development/mysql-workbench-pro/images/grt/column_pk.png");

    set_accepts_focus(true);
    set_accepts_selection(true);

    set_background_color(base::Color(1, 1, 1));
    set_border_color(base::Color(0.5, 0.5, 0.5));
    set_draw_background(true);

    add(&title_bar, false, false);
    title_bar.set_padding(4, 4);
    title_bar.add(&title, true, true);
    title_bar.set_background_color(base::Color(0.5, 0.7, 0.83));
    title_bar.set_border_color(base::Color(0.5, 0.5, 0.5));
    title_bar.set_draw_background(true);

    title.set_icon(cairo_image_surface_create_from_png(
      "/Users/kojima/Development/mysql-workbench-pro/images/grt/db.Table.12x12.png"));
    title.set_font(mdc::FontSpec("Lucida Grande", mdc::SNormal, mdc::WBold, 10));
    title.set_text("Hello World");

    title_expander.set_fixed_size(base::Size(10, -1));
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
    tf->set_font(mdc::FontSpec("Lucida Grande", mdc::SNormal, mdc::WNormal, 10));
    tf->set_text(text);

    column_box.add(tf, false, true);
  }
};

$TestData {
  std::unique_ptr<mdc::CanvasView> view;
  std::unique_ptr<mdc::AreaGroup> group;

  std::unique_ptr<Thing> item0, item1, item2;

  std::string outputDir = CasmineContext::get()->outputDir();
  std::string dataDir = CasmineContext::get()->tmpDataDir();
};

$xdescribe("mdc canvas") {

  $it("View creation + zoom", []() {
    mdc::ImageCanvasView view(500, 400);

    view.set_page_size(base::Size(500, 400));

    $expect(view.get_viewport().str()).toBe(base::Rect(0, 0, 500, 400).str());

    view.set_zoom(2);

    $expect(view.get_viewport().str()).toBe(base::Rect(0, 0, 250, 200).str());

    view.set_zoom(0.5);

    $expect(view.get_viewport().str()).toBe(base::Rect(0, 0, 500, 400).str());
  });

  $it("View hierarchy creation", [this]() {
    data->view = std::make_unique<mdc::ImageCanvasView>(1000, 1000);
    data->view->initialize();

    mdc::Layer *layer = data->view->get_current_layer();
    $expect(layer).Not.toBe(nullptr);

    data->item0 = std::make_unique<Thing>(layer);
    layer->add_item(data->item0.get());
    data->item0->move_to(base::Point(100, 100));

    data->item1 = std::make_unique<Thing>(layer);
    layer->add_item(data->item1.get());

    data->item2 = std::make_unique<Thing>(layer);
    layer->add_item(data->item2.get());

    data->item1->move_to(base::Point(100, 100));

    data->item2->move_to(base::Point(200, 50));

    data->view->get_selection()->add(data->item1.get());
    data->view->get_selection()->add(data->item2.get());

    $expect(data->view->get_selected_items().size()).toBe(2U);

    std::list<mdc::CanvasItem *> items;
    mdc::Selection::ContentType selection(data->view->get_selected_items());

    for (mdc::Selection::ContentType::iterator iter = selection.begin(); iter != selection.end(); ++iter)
      items.push_back(*iter);

    data->group.reset(layer->create_area_group_with(items));

    data->group->set_draw_background(true);
    data->group->set_background_color(base::Color(1.0, 1.0, 0.6));

    $expect(data->group != nullptr).toBeTrue();
  });

  $it("Test get_common_ancestor", [this]() {
    mdc::CanvasItem *ancestor;

    ancestor = data->item1->get_common_ancestor(data->item2.get());
    $expect(ancestor).toBe(data->item1->get_parent());

    ancestor = data->item2->get_common_ancestor(data->item1.get());
    $expect(ancestor).toBe(data->item1->get_parent());

    $expect(ancestor).Not.toBe(nullptr);
  });

  $it("Coordinate conversion", [this]() {
    base::Point p;

    $expect(data->item0->get_position().x).toBe(100);
    $expect(data->item0->get_position().y).toBe(100);

    // convert point from item0's coordinate system to the global coord system
    p = data->item0->convert_point_to(base::Point(4, 5), 0);
    $expect(p.x).toBe(104);
    $expect(p.y).toBe(105);

    // convert point from global to local
    p = data->item0->convert_point_from(base::Point(304, 305), 0);
    $expect(p.x).toBe(204);
    $expect(p.y).toBe(205);
  });

  $it("Non-homogeneous box layout", []() {
    mdc::ImageCanvasView view(1000, 1000);
    view.initialize();
    mdc::Layer *layer = view.get_current_layer();

    mdc::Box *hbox = new mdc::Box(layer, mdc::Box::Horizontal, false);
    mdc::RectangleFigure r1(layer);
    mdc::RectangleFigure r2(layer);
    mdc::RectangleFigure r3(layer);
    mdc::RectangleFigure r4(layer);

    r1.set_fixed_min_size(base::Size(20, -1));
    r2.set_fixed_min_size(base::Size(20, -1));
    r3.set_fixed_min_size(base::Size(20, -1));
    r4.set_fixed_min_size(base::Size(20, -1));

    hbox->set_fixed_size(base::Size(200, 20));

    hbox->add(&r1, true, true, false);   // expand, fill
    hbox->add(&r2, true, false, false);  // expand, dont fill
    hbox->add(&r3, false, true, false);  // dont expand, fill
    hbox->add(&r4, false, false, false); // dont expand, dont fill

    hbox->relayout();

    $expect(hbox->get_size().width).toBe(200);
    $expect(r1.get_size().width).toBe(80);
    $expect(r1.get_size().height).toBe(20);

    $expect(r2.get_size().width).toBe(20);
    $expect(r2.get_size().height).toBe(20);

    $expect(r3.get_size().width).toBe(20);
    $expect(r3.get_size().height).toBe(20);

    $expect(r4.get_size().width).toBe(20);
    $expect(r4.get_size().height).toBe(20);
  });

  $it("Rectangle rendering", [this]() {
    mdc::ImageCanvasView imageView(500, 400);
    mdc::Layer *layer;

    imageView.initialize();
    layer = imageView.get_current_layer();

    mdc::RectangleFigure r[17] = {
      layer, layer, layer, layer, layer, layer, layer, layer, layer, layer, layer, layer, layer, layer, layer, layer, layer
    };

    for (int i = 0; i < 17; i++)
      layer->add_item(&r[i]);

    r[0].move_to(base::Point(10, 10));
    r[0].set_fixed_size(base::Size(50, 100));
    r[0].set_pen_color(base::Color(1, 0, 0));

    r[1].move_to(base::Point(25, 50));
    r[1].set_fixed_size(base::Size(50, 50));
    r[1].set_pen_color(base::Color(0, 1, 0));

    // XXX TODO: alpha is being ignored, not sure if its a cairo bug or what
    r[2].move_to(base::Point(40, 10));
    r[2].set_fixed_size(base::Size(50, 50));
    r[2].set_pen_color(base::Color(0, 0, 0));
    r[2].set_filled(true);
    r[2].set_fill_color(base::Color(0, 1, 0, 0.5));

    r[3].move_to(base::Point(100, 10));
    r[3].set_fixed_size(base::Size(80, 80));
    r[3].set_pen_color(base::Color::black());
    r[3].set_filled(true);
    r[3].set_fill_color(base::Color(1, 0.5, 0.8));
    r[3].set_rounded_corners(10, mdc::CTopLeft | mdc::CBottomRight);

    r[4].move_to(base::Point(200, 10));
    r[4].set_fixed_size(base::Size(80, 80));
    r[4].set_pen_color(base::Color::black());
    r[4].set_filled(true);
    r[4].set_fill_color(base::Color(1, 0.5, 0.8));
    r[4].set_rounded_corners(10, mdc::CTopRight | mdc::CBottomLeft);

    r[5].move_to(base::Point(300, 10));
    r[5].set_fixed_size(base::Size(80, 80));
    r[5].set_pen_color(base::Color::black());
    r[5].set_filled(true);
    r[5].set_fill_color(base::Color(1, 0.5, 0.8));
    r[5].set_rounded_corners(10, mdc::CTop);

    r[6].move_to(base::Point(400, 10));
    r[6].set_fixed_size(base::Size(80, 80));
    r[6].set_pen_color(base::Color::black());
    r[6].set_filled(true);
    r[6].set_fill_color(base::Color(1, 0.5, 0.8));
    r[6].set_rounded_corners(10, mdc::CBottom);

    r[7].move_to(base::Point(100, 300));
    r[7].set_fixed_size(base::Size(80, 80));
    r[7].set_pen_color(base::Color::black());
    r[7].set_filled(true);
    r[7].set_fill_color(base::Color(1, 1, 0.8));
    r[7].set_rounded_corners(10, mdc::CAll);

    r[8].move_to(base::Point(100, 100));
    r[8].set_fixed_size(base::Size(80, 80));
    r[8].set_pen_color(base::Color::black());
    r[8].set_filled(true);
    r[8].set_fill_color(base::Color(0.5, 0.5, 0.8));
    r[8].set_rounded_corners(10, mdc::CTopLeft);

    r[9].move_to(base::Point(200, 100));
    r[9].set_fixed_size(base::Size(80, 80));
    r[9].set_pen_color(base::Color::black());
    r[9].set_filled(true);
    r[9].set_fill_color(base::Color(0.5, 0.5, 0.8));
    r[9].set_rounded_corners(10, mdc::CTopRight);

    r[10].move_to(base::Point(300, 100));
    r[10].set_fixed_size(base::Size(80, 80));
    r[10].set_pen_color(base::Color::black());
    r[10].set_filled(true);
    r[10].set_fill_color(base::Color(0.5, 0.5, 0.8));
    r[10].set_rounded_corners(10, mdc::CBottomLeft);

    r[11].move_to(base::Point(400, 100));
    r[11].set_fixed_size(base::Size(80, 80));
    r[11].set_pen_color(base::Color::black());
    r[11].set_filled(true);
    r[11].set_fill_color(base::Color(0.5, 0.5, 0.8));
    r[11].set_rounded_corners(10, mdc::CBottomRight);

    r[12].move_to(base::Point(100, 200));
    r[12].set_fixed_size(base::Size(80, 80));
    r[12].set_pen_color(base::Color::black());
    r[12].set_filled(true);
    r[12].set_fill_color(base::Color(0.5, 0.8, 0.8));
    r[12].set_rounded_corners(10, (mdc::CornerMask)~mdc::CTopLeft);

    r[13].move_to(base::Point(200, 200));
    r[13].set_fixed_size(base::Size(80, 80));
    r[13].set_pen_color(base::Color::black());
    r[13].set_filled(true);
    r[13].set_fill_color(base::Color(0.5, 0.8, 0.8));
    r[13].set_rounded_corners(10, (mdc::CornerMask)~mdc::CTopRight);

    r[14].move_to(base::Point(300, 200));
    r[14].set_fixed_size(base::Size(80, 80));
    r[14].set_pen_color(base::Color::black());
    r[14].set_filled(true);
    r[14].set_fill_color(base::Color(0.5, 0.8, 0.8));
    r[14].set_rounded_corners(10, (mdc::CornerMask)~mdc::CBottomLeft);

    r[15].move_to(base::Point(400, 200));
    r[15].set_fixed_size(base::Size(80, 80));
    r[15].set_pen_color(base::Color::black());
    r[15].set_filled(true);
    r[15].set_fill_color(base::Color(0.5, 0.8, 0.8));
    r[15].set_rounded_corners(10, (mdc::CornerMask)~mdc::CBottomRight);

    r[16].move_to(base::Point(200, 300));
    r[16].set_fixed_size(base::Size(80, 80));
    r[16].set_pen_color(base::Color::black());
    r[16].set_filled(true);
    r[16].set_fill_color(base::Color(1, 1, 0.8));
    r[16].set_rounded_corners(10, mdc::CNone);

    std::string target = data->outputDir + "/shapes_rects_test.png";
    imageView.save_to(target);

    CairoSurface targetSurface(target);
    $expect((int)cairo_surface_status(targetSurface.surface)).toBe(CAIRO_STATUS_SUCCESS, "Target surface invalid");
    CairoSurface sourceSurface(data->dataDir + "/images/shapes_rects.png");
    $expect((int)cairo_surface_status(sourceSurface.surface)).toBe(CAIRO_STATUS_SUCCESS, "Source surface invalid");

    $expect(targetSurface == sourceSurface).toBe(true);
  });

}

}
