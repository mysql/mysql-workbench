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

BEGIN_TEST_DATA_CLASS(canvas_layouting)
END_TEST_DATA_CLASS

TEST_MODULE(canvas_layouting, "Canvas: layouting");

TEST_FUNCTION(1) { // test non-homogeneous box layout
  ImageCanvasView view(1000, 1000);
  view.initialize();
  Layer *layer = view.get_current_layer();

  Box *hbox = new Box(layer, Box::Horizontal, false);
  RectangleFigure r1(layer);
  RectangleFigure r2(layer);
  RectangleFigure r3(layer);
  RectangleFigure r4(layer);

  r1.set_fixed_min_size(Size(20, -1));
  r2.set_fixed_min_size(Size(20, -1));
  r3.set_fixed_min_size(Size(20, -1));
  r4.set_fixed_min_size(Size(20, -1));

  hbox->set_fixed_size(Size(200, 20));

  hbox->add(&r1, true, true, false);   // expand, fill
  hbox->add(&r2, true, false, false);  // expand, dont fill
  hbox->add(&r3, false, true, false);  // dont expand, fill
  hbox->add(&r4, false, false, false); // dont expand, dont fill

  hbox->relayout();

  ensure_equals("box size", hbox->get_size().width, 200);
  ensure_equals("r1 width", r1.get_size().width, 80);
  ensure_equals("r1 height", r1.get_size().height, 20);

  ensure_equals("r2 width", r2.get_size().width, 20);
  ensure_equals("r2 height", r2.get_size().height, 20);

  ensure_equals("r3 width", r3.get_size().width, 20);
  ensure_equals("r3 height", r3.get_size().height, 20);

  ensure_equals("r4 width", r4.get_size().width, 20);
  ensure_equals("r4 height", r4.get_size().height, 20);
}

TEST_FUNCTION(2) { // test homogeneous box layout
}

END_TESTS
