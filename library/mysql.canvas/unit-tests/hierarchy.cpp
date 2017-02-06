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
#include "test_figure.h"
#include "mdc_canvas_view_image.h"
#include "wb_helpers.h"

using namespace mdc;
using namespace base;

BEGIN_TEST_DATA_CLASS(canvas_hierarchy)
public:
CanvasView *view;

AreaGroup *group;
Thing *item0, *item1, *item2;
END_TEST_DATA_CLASS

TEST_MODULE(canvas_hierarchy, "Canvas: hierarchy");

TEST_FUNCTION(1) {
  // create a test hierarchy

  view = new ImageCanvasView(1000, 1000);
  view->initialize();

  Layer *layer = view->get_current_layer();
  ensure("layer", layer != 0);

  item0 = new Thing(layer);
  layer->add_item(item0);
  item0->move_to(Point(100, 100));

  item1 = new Thing(layer);
  layer->add_item(item1);

  item2 = new Thing(layer);
  layer->add_item(item2);

  item1->move_to(Point(100, 100));

  item2->move_to(Point(200, 50));

  view->get_selection()->add(item1);
  view->get_selection()->add(item2);

  ensure_equals("selection", view->get_selected_items().size(), 2U);

  std::list<mdc::CanvasItem *> items;
  Selection::ContentType selection(view->get_selected_items());

  for (Selection::ContentType::iterator iter = selection.begin(); iter != selection.end(); ++iter)
    items.push_back(*iter);

  group = layer->create_area_group_with(items);

  group->set_draw_background(true);
  group->set_background_color(Color(1.0, 1.0, 0.6));

  ensure("area group", group != 0);
}

TEST_FUNCTION(2) { // test get_common_ancestor

  CanvasItem *ancestor;

  ancestor = item1->get_common_ancestor(item2);
  ensure("common ancestor", ancestor == item1->get_parent());

  ancestor = item2->get_common_ancestor(item1);
  ensure("common ancestor", ancestor == item1->get_parent());

  ensure("common ancestor", ancestor != NULL);
}

TEST_FUNCTION(3) {
  Point p;

  ensure_equals("item0 x", item0->get_position().x, 100);
  ensure_equals("item0 y", item0->get_position().y, 100);

  // convert point from item0's coordinate system to the global coord system
  p = item0->convert_point_to(Point(4, 5), 0);
  ensure_equals("item0 to root x", p.x, 104);
  ensure_equals("item0 to root y", p.y, 105);

  // convert point from global to local
  p = item0->convert_point_from(Point(304, 305), 0);
  ensure_equals("item0 from root x", p.x, 204);
  ensure_equals("item0 from root y", p.y, 205);
}

END_TESTS
