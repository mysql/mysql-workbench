#include "stdafx.h"
#include "test.h"
#include "mdc.h"
#include <mdc_canvas_view_image.h>
#include "wbcanvas/table_figure.h"
#include "wbcanvas/view_figure.h"
#include "wbcanvas/routine_group_figure.h"

using namespace mdc;
using namespace wbfig;

BEGIN_TEST_DATA_CLASS(wbcanvas_elements)
public:
ImageCanvasView view;
Layer *layer;

TEST_DATA_CONSTRUCTOR(wbcanvas_elements) : view(0, 500, 400) {
  view.initialize();
  layer = view.get_current_layer();

  IconManager::get_instance()->add_search_path("../../images/grt/structs");
}

END_TEST_DATA_CLASS

TEST_MODULE(wbcanvas_elements, "wbcanvas elements");

TEST_FUNCTION(1) { // test table

  Table table(layer);

  table.get_title()->set_title("Table áéíõãä");
  layer->add_item(&table);

  table.move_to(Point(150, 150));

  view.save_to("canvas_table.png");

  //  ensure_png_equals("table", "canvas_table.png");
}

TEST_FUNCTION(2) { // test view
  ensure("view is empty", layer->get_root_area_group()->get_contents().size() == 0);

  View v(layer);

  layer->add_item(&v);

  v.move_to(Point(150, 150));

  view.save_to("canvas_view.png");

  //  ensure_png_equals("view", "canvas_view.png");
}

TEST_FUNCTION(3) { // test routnegroup

  RoutineGroup routinegroup(layer);

  layer->add_item(&routinegroup);

  routinegroup.move_to(Point(150, 150));

  view.save_to("canvas_routines.png");

  //  ensure_png_equals("table", "canvas_routines.png");
}

END_TESTS
