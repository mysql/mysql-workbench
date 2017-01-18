/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mdc.h"
#include "mdc_canvas_view_image.h"
#include "wb_helpers.h"

using namespace mdc;
using namespace base;

BEGIN_TEST_DATA_CLASS(canvas_shapes)
public:
ImageCanvasView view;
Layer *layer;

TEST_DATA_CONSTRUCTOR(canvas_shapes) : view(500, 400) {
  view.initialize();
  layer = view.get_current_layer();
}

END_TEST_DATA_CLASS

TEST_MODULE(canvas_shapes, "Canvas: shapes");

TEST_FUNCTION(1) { // test rectangle rendering
#ifndef DISABLE_IMAGE_TESTS
  RectangleFigure r[17] = {layer, layer, layer, layer, layer, layer, layer, layer, layer,
                           layer, layer, layer, layer, layer, layer, layer, layer};

  for (int i = 0; i < 17; i++)
    layer->add_item(&r[i]);

  r[0].move_to(Point(10, 10));
  r[0].set_fixed_size(Size(50, 100));
  r[0].set_pen_color(Color(1, 0, 0));

  r[1].move_to(Point(25, 50));
  r[1].set_fixed_size(Size(50, 50));
  r[1].set_pen_color(Color(0, 1, 0));

  // XXX TODO: alpha is being ignored, not sure if its a cairo bug or what
  r[2].move_to(Point(40, 10));
  r[2].set_fixed_size(Size(50, 50));
  r[2].set_pen_color(Color(0, 0, 0));
  r[2].set_filled(true);
  r[2].set_fill_color(Color(0, 1, 0, 0.5));

  r[3].move_to(Point(100, 10));
  r[3].set_fixed_size(Size(80, 80));
  r[3].set_pen_color(Color::Black());
  r[3].set_filled(true);
  r[3].set_fill_color(Color(1, 0.5, 0.8));
  r[3].set_rounded_corners(10, CTopLeft | CBottomRight);

  r[4].move_to(Point(200, 10));
  r[4].set_fixed_size(Size(80, 80));
  r[4].set_pen_color(Color::Black());
  r[4].set_filled(true);
  r[4].set_fill_color(Color(1, 0.5, 0.8));
  r[4].set_rounded_corners(10, CTopRight | CBottomLeft);

  r[5].move_to(Point(300, 10));
  r[5].set_fixed_size(Size(80, 80));
  r[5].set_pen_color(Color::Black());
  r[5].set_filled(true);
  r[5].set_fill_color(Color(1, 0.5, 0.8));
  r[5].set_rounded_corners(10, CTop);

  r[6].move_to(Point(400, 10));
  r[6].set_fixed_size(Size(80, 80));
  r[6].set_pen_color(Color::Black());
  r[6].set_filled(true);
  r[6].set_fill_color(Color(1, 0.5, 0.8));
  r[6].set_rounded_corners(10, CBottom);

  r[7].move_to(Point(100, 300));
  r[7].set_fixed_size(Size(80, 80));
  r[7].set_pen_color(Color::Black());
  r[7].set_filled(true);
  r[7].set_fill_color(Color(1, 1, 0.8));
  r[7].set_rounded_corners(10, CAll);

  r[8].move_to(Point(100, 100));
  r[8].set_fixed_size(Size(80, 80));
  r[8].set_pen_color(Color::Black());
  r[8].set_filled(true);
  r[8].set_fill_color(Color(0.5, 0.5, 0.8));
  r[8].set_rounded_corners(10, CTopLeft);

  r[9].move_to(Point(200, 100));
  r[9].set_fixed_size(Size(80, 80));
  r[9].set_pen_color(Color::Black());
  r[9].set_filled(true);
  r[9].set_fill_color(Color(0.5, 0.5, 0.8));
  r[9].set_rounded_corners(10, CTopRight);

  r[10].move_to(Point(300, 100));
  r[10].set_fixed_size(Size(80, 80));
  r[10].set_pen_color(Color::Black());
  r[10].set_filled(true);
  r[10].set_fill_color(Color(0.5, 0.5, 0.8));
  r[10].set_rounded_corners(10, CBottomLeft);

  r[11].move_to(Point(400, 100));
  r[11].set_fixed_size(Size(80, 80));
  r[11].set_pen_color(Color::Black());
  r[11].set_filled(true);
  r[11].set_fill_color(Color(0.5, 0.5, 0.8));
  r[11].set_rounded_corners(10, CBottomRight);

  r[12].move_to(Point(100, 200));
  r[12].set_fixed_size(Size(80, 80));
  r[12].set_pen_color(Color::Black());
  r[12].set_filled(true);
  r[12].set_fill_color(Color(0.5, 0.8, 0.8));
  r[12].set_rounded_corners(10, (CornerMask)~CTopLeft);

  r[13].move_to(Point(200, 200));
  r[13].set_fixed_size(Size(80, 80));
  r[13].set_pen_color(Color::Black());
  r[13].set_filled(true);
  r[13].set_fill_color(Color(0.5, 0.8, 0.8));
  r[13].set_rounded_corners(10, (CornerMask)~CTopRight);

  r[14].move_to(Point(300, 200));
  r[14].set_fixed_size(Size(80, 80));
  r[14].set_pen_color(Color::Black());
  r[14].set_filled(true);
  r[14].set_fill_color(Color(0.5, 0.8, 0.8));
  r[14].set_rounded_corners(10, (CornerMask)~CBottomLeft);

  r[15].move_to(Point(400, 200));
  r[15].set_fixed_size(Size(80, 80));
  r[15].set_pen_color(Color::Black());
  r[15].set_filled(true);
  r[15].set_fill_color(Color(0.5, 0.8, 0.8));
  r[15].set_rounded_corners(10, (CornerMask)~CBottomRight);

  r[16].move_to(Point(200, 300));
  r[16].set_fixed_size(Size(80, 80));
  r[16].set_pen_color(Color::Black());
  r[16].set_filled(true);
  r[16].set_fill_color(Color(1, 1, 0.8));
  r[16].set_rounded_corners(10, CNone);

  view.save_to("shapes_rects.png");

  ensure_png_equals("rectangles", "shapes_rects.png");
#endif
}

TEST_FUNCTION(2) { // test line connection
}

TEST_FUNCTION(3) { // test line ends
}

TEST_FUNCTION(4) { // test text rendering
}

END_TESTS
