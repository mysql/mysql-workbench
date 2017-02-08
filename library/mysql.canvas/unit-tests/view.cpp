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

BEGIN_TEST_DATA_CLASS(canvas_view)

END_TEST_DATA_CLASS

TEST_MODULE(canvas_view, "Canvas: view");

TEST_FUNCTION(1) {
  ImageCanvasView view(500, 400);

  // test viewport

  view.set_page_size(Size(500, 400));

  ensure_equals("viewport for size=viewport", view.get_viewport().str(), Rect(0, 0, 500, 400).str());

  view.set_zoom(2);

  ensure_equals("viewport for zoom 2", view.get_viewport().str(), Rect(0, 0, 250, 200).str());

  view.set_zoom(0.5);

  ensure_equals("viewport for zoom 0.5", view.get_viewport().str(), Rect(0, 0, 500, 400).str());
}

END_TESTS
