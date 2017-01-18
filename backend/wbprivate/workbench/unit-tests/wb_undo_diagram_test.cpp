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

#include "wb_helpers.h"
#include "grtdb/db_object_helpers.h"
#include "grt_test_utility.h"
#include "model/wb_history_tree.h"

#include "stub/stub_utilities.h"

using namespace bec;
using namespace wb;
using namespace grt;
using namespace base;

BEGIN_TEST_DATA_CLASS(wb_undo_diagram)
public:
WBTester *tester;
UndoManager *um;
OverviewBE *overview;
ModelDiagramForm *diagram_form;
model_DiagramRef diagram;
size_t last_undo_stack_height;
size_t last_redo_stack_height;

TEST_DATA_CONSTRUCTOR(wb_undo_diagram) {
  tester = new WBTester;
  populate_grt(*tester);

  um = grt::GRT::get()->get_undo_manager();
  overview = wb::WBContextUI::get()->get_physical_overview();
  diagram = model_DiagramRef();

  last_undo_stack_height = 0;
  last_redo_stack_height = 0;
  bool flag = tester->wb->open_document("data/workbench/undo_test_model1.mwb");
  ensure("open_document", flag);
  ensure_equals("schemas", tester->get_catalog()->schemata().count(), 1U);

  db_SchemaRef schema(tester->get_catalog()->schemata()[0]);

  // make sure the loaded model contains expected number of things
  ensure_equals("tables", schema->tables().count(), 4U);
  ensure_equals("views", schema->views().count(), 1U);
  ensure_equals("groups", schema->routineGroups().count(), 1U);

  ensure_equals("diagrams", tester->get_pmodel()->diagrams().count(), 1U);
  diagram = tester->get_pmodel()->diagrams()[0];

  ensure_equals("figures", diagram->figures().count(), 5U);
  ensure_equals("layers", diagram->layers().count(), 1U);

  tester->open_all_diagrams();
  tester->sync_view();

  diagram_form = tester->wb->get_model_context()->get_diagram_form_for_diagram_id(tester->get_pview().id());
  ensure("Diagram form is invalid", diagram_form != 0);

  mforms::ToolBar *toolbar = diagram_form->get_tools_toolbar();
  ensure("Toolbar creation failed", toolbar != NULL);

  // Model file not closed by intention. It's used in following test cases.
}

#include "wb_undo_methods.h"

void place_figure_with_tool(const std::string &tool, double x = 10, double y = 10) {
  diagram_form->set_tool(tool);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(x), static_cast<int>(y),
                                    (mdc::EventState)0);
}

END_TEST_DATA_CLASS;

TEST_MODULE(wb_undo_diagram, "undo tests for diagram actions in Workbench");

// setup
TEST_FUNCTION(1) {
  wb::WBContextUI::get()->set_active_form(diagram_form);
  ensure_equals("undo stack is empty", um->get_undo_stack().size(), 0U);
}

// Diagram
//----------------------------------------------------------------------------------------

TEST_FUNCTION(10) //  Place Table
{
  db_SchemaRef schema = tester->get_catalog()->schemata()[0];
  size_t old_figure_count = diagram->figures().count();
  size_t old_root_figure_count = diagram->rootLayer()->figures().count();
  size_t old_object_count = schema->tables().count();

  WBComponentPhysical *compo = wb::WBContextUI::get()->get_wb()->get_component<WBComponentPhysical>();

  compo->place_new_db_object(diagram_form, Point(10, 10), wb::ObjectTable);
  check_only_one_undo_added();

  ensure_equals("figure add", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("table add", schema->tables().count(), old_object_count + 1);
  ensure_equals("figure root add", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
  ensure_equals("figure add undo", diagram->figures().count(), old_figure_count);
  ensure_equals("table add undo", schema->tables().count(), old_object_count);
  ensure_equals("figure root add undo", diagram->rootLayer()->figures().count(), old_root_figure_count);

  check_redo();
  ensure_equals("figure add redo", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("table add redo", schema->tables().count(), old_object_count + 1);
  ensure_equals("figure root add redo", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
}

TEST_FUNCTION(11) //  Place View
{
  db_SchemaRef schema = tester->get_catalog()->schemata()[0];
  size_t old_figure_count = diagram->figures().count();
  size_t old_root_figure_count = diagram->rootLayer()->figures().count();
  size_t old_object_count = schema->views().count();

  WBComponentPhysical *compo = wb::WBContextUI::get()->get_wb()->get_component<WBComponentPhysical>();

  compo->place_new_db_object(diagram_form, Point(10, 10), wb::ObjectView);
  check_only_one_undo_added();

  ensure_equals("figure add", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("diagram_form add", schema->views().count(), old_object_count + 1);
  ensure_equals("figure root add", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
  ensure_equals("figure add undo", diagram->figures().count(), old_figure_count);
  ensure_equals("diagram_form add undo", schema->views().count(), old_object_count);
  ensure_equals("figure root add undo", diagram->rootLayer()->figures().count(), old_root_figure_count);

  check_redo();
  ensure_equals("figure add redo", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("diagram_form add redo", schema->views().count(), old_object_count + 1);
  ensure_equals("figure root add redo", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
}

TEST_FUNCTION(12) //  Place Routine Group
{
  db_SchemaRef schema = tester->get_catalog()->schemata()[0];
  size_t old_figure_count = diagram->figures().count();
  size_t old_root_figure_count = diagram->rootLayer()->figures().count();
  size_t old_object_count = schema->routineGroups().count();

  WBComponentPhysical *compo = wb::WBContextUI::get()->get_wb()->get_component<WBComponentPhysical>();

  compo->place_new_db_object(diagram_form, Point(10, 10), wb::ObjectRoutineGroup);
  check_only_one_undo_added();

  ensure_equals("figure add", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("group add", schema->routineGroups().count(), old_object_count + 1);
  ensure_equals("figure root add", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
  ensure_equals("figure add undo", diagram->figures().count(), old_figure_count);
  ensure_equals("group add undo", schema->routineGroups().count(), old_object_count);
  ensure_equals("figure root add undo", diagram->rootLayer()->figures().count(), old_root_figure_count);

  check_redo();
  ensure_equals("figure add redo", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("group add redo", schema->routineGroups().count(), old_object_count + 1);
  ensure_equals("figure root add redo", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
}

TEST_FUNCTION(13) //  Place Image
{
  size_t old_figure_count = diagram->figures().count();
  size_t old_root_figure_count = diagram->rootLayer()->figures().count();

  // place image will ask for a filename of the image
  tester->add_file_for_file_dialog("data/images/sakila.png");

  place_figure_with_tool(WB_TOOL_IMAGE);
  check_only_one_undo_added();

  ensure_equals("figure add", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("figure root add", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
  ensure_equals("figure add undo", diagram->figures().count(), old_figure_count);
  ensure_equals("figure root add undo", diagram->rootLayer()->figures().count(), old_root_figure_count);

  check_redo();
  ensure_equals("figure add redo", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("figure root add redo", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
}

TEST_FUNCTION(14) //  Place Text
{
  size_t old_figure_count = diagram->figures().count();
  size_t old_root_figure_count = diagram->rootLayer()->figures().count();

  place_figure_with_tool(WB_TOOL_NOTE);
  check_only_one_undo_added();

  ensure_equals("figure add", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("figure root add", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
  ensure_equals("figure add undo", diagram->figures().count(), old_figure_count);
  ensure_equals("figure root add undo", diagram->rootLayer()->figures().count(), old_root_figure_count);

  check_redo();
  ensure_equals("figure add redo", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("figure root add redo", diagram->rootLayer()->figures().count(), old_root_figure_count + 1);

  check_undo();
}

TEST_FUNCTION(15) //  Place Layer
{
  size_t old_layer_count = diagram->layers().count();
  size_t old_root_layer_count = diagram->rootLayer()->subLayers().count();

  diagram_form->set_tool(WB_TOOL_LAYER);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, 10, 10, (mdc::EventState)0);
  diagram_form->handle_mouse_move(50, 50, mdc::SLeftButtonMask);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, 50, 50, (mdc::EventState)0);
  check_only_one_undo_added();

  ensure_equals("layer add", diagram->layers().count(), old_layer_count + 1);
  ensure_equals("layer root add", diagram->rootLayer()->subLayers().count(), old_root_layer_count + 1);

  check_undo();
  ensure_equals("layer add undo", diagram->layers().count(), old_layer_count);
  ensure_equals("layer root add undo", diagram->rootLayer()->subLayers().count(), old_root_layer_count);

  check_redo();
  ensure_equals("layer add redo", diagram->layers().count(), old_layer_count + 1);
  ensure_equals("layer root add redo", diagram->rootLayer()->subLayers().count(), old_root_layer_count + 1);

  check_undo();
}

TEST_FUNCTION(16) // Place something inside a Layer
{
  size_t old_figure_count = diagram->figures().count();
  size_t old_layer_count = diagram->layers().count();
  size_t old_root_layer_count = diagram->rootLayer()->subLayers().count();

  // place layer
  diagram_form->set_tool(WB_TOOL_LAYER);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, 10, 10, (mdc::EventState)0);
  diagram_form->handle_mouse_move(150, 150, (mdc::EventState)0);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, 150, 150, (mdc::EventState)0);
  check_only_one_undo_added();

  ensure_equals("layer add", diagram->layers().count(), old_layer_count + 1);
  ensure_equals("layer root add", diagram->rootLayer()->subLayers().count(), old_root_layer_count + 1);

  model_LayerRef layer(diagram->layers()[old_layer_count]);
  ensure("layer", layer.is_valid());
  ensure_equals("layer empty", layer->figures().count(), 0U);

  // place a note inside the layer
  place_figure_with_tool(WB_TOOL_NOTE, 50, 50);
  check_only_one_undo_added();

  ensure_equals("note add", diagram->figures().count(), old_figure_count + 1);
  model_FigureRef figure(diagram->figures()[old_figure_count]);

  ensure_equals("new note pos", *figure->top(), 40);
  ensure_equals("new note pos", *figure->left(), 40);

  ensure_equals("layer contains figure", layer->figures().count(), 1U);
  ensure("note layer", figure->layer() == layer);

  check_undo();
  ensure_equals("note add undo", diagram->figures().count(), old_figure_count);
  ensure_equals("layer contains figure undo", layer->figures().count(), 0U);

  check_redo();
  ensure_equals("note add undo", diagram->figures().count(), old_figure_count + 1);
  ensure_equals("layer contains figure undo", layer->figures().count(), 1U);

  check_undo();

  check_undo();
}

TEST_FUNCTION(17) // Place Layer around something
{
  size_t old_figure_count = diagram->figures().count();
  size_t old_layer_count = diagram->layers().count();
  size_t old_root_layer_count = diagram->rootLayer()->subLayers().count();

  // place a note
  diagram_form->set_tool(WB_TOOL_NOTE);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, 200, 200, (mdc::EventState)0);
  check_only_one_undo_added();

  model_FigureRef figure(diagram->figures()[old_figure_count]);

  ensure_equals("note add", diagram->figures().count(), old_figure_count + 1);

  // place layer around the note
  diagram_form->set_tool(WB_TOOL_LAYER);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, 190, 190, (mdc::EventState)0);
  diagram_form->handle_mouse_move(300, 300, (mdc::EventState)0);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, 300, 300, (mdc::EventState)0);
  check_only_one_undo_added();

  ensure_equals("layer add", diagram->layers().count(), old_layer_count + 1);
  ensure_equals("layer root add", diagram->rootLayer()->subLayers().count(), old_root_layer_count + 1);

  model_LayerRef layer(diagram->layers()[old_layer_count]);
  ensure("layer", layer.is_valid());
  ensure("note layer", figure->layer() == layer);

  ensure_equals("layer contains note only", layer->figures().count(), 1U);
  ensure("layer contains note only", layer->figures().get_index(figure) != BaseListRef::npos);

  ensure_equals("layer add", diagram->layers().count(), old_layer_count + 1);
  ensure_equals("layer root add", diagram->rootLayer()->subLayers().count(), old_root_layer_count + 1);
  ensure_equals("layer note", layer->figures().count(), 1U);
  ensure_equals("root layer note", layer->figures().count(), 1U);

  check_undo();
  ensure_equals("layer add undo", diagram->layers().count(), old_layer_count);
  ensure_equals("layer root add undo", diagram->rootLayer()->subLayers().count(), old_root_layer_count);
  ensure_equals("layer note", layer->figures().count(), 0U);

  check_redo();
  ensure_equals("layer add redo", diagram->layers().count(), old_layer_count + 1);
  ensure_equals("layer root add redo", diagram->rootLayer()->subLayers().count(), old_root_layer_count + 1);
  ensure_equals("layer note", layer->figures().count(), 1U);

  check_undo(); // undo the layer

  check_undo(); // undo the note place
}

TEST_FUNCTION(20) // Move Object
{
  double x, y;
  model_FigureRef figure(diagram->figures()[0]);

  x = figure->left();
  y = figure->top();

  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(x + 5), static_cast<int>(y + 5),
                                    (mdc::EventState)0);
  diagram_form->handle_mouse_move(50, 50, mdc::SLeftButtonMask);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, 50, 50, mdc::SLeftButtonMask);
  check_only_one_undo_added();

  ensure("object moved", *figure->left() != x);

  check_undo();
  ensure_equals("move undo", *figure->left(), x);

  check_redo();
  ensure("move redo", *figure->left() != x);

  check_undo();
}

TEST_FUNCTION(22) // Move into and out of Layer
{
  double x, y;
  model_FigureRef figure(diagram->figures()[0]);
  model_LayerRef layer(diagram->layers()[0]);

  ensure("layer is not root", layer != diagram->rootLayer());

  x = figure->left();
  y = figure->top();

  ensure("object is in root", figure->layer() == diagram->rootLayer());
  ensure_equals("layer begins empty", layer->figures().count(), 0U);

  // move object into layer
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(x + 5), static_cast<int>(y + 5),
                                    (mdc::EventState)0);
  diagram_form->handle_mouse_move(150, 400, mdc::SLeftButtonMask);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, 150, 400, mdc::SLeftButtonMask);
  check_only_one_undo_added();

  ensure("object moved", *figure->left() != x);
  ensure("object moved into layer", figure->layer() == layer);
  ensure("layer contains object", layer->figures().get_index(figure) != BaseListRef::npos);
  ensure("object not in root", diagram->rootLayer()->figures()->get_index(figure) == BaseListRef::npos);

  check_undo();
  ensure_equals("move undo", *figure->left(), x);
  ensure("object moved into layer undo", figure->layer() == diagram->rootLayer());
  ensure_equals("layer empty on undo", layer->figures().count(), 0U);
  ensure("layer not contains object", layer->figures().get_index(figure) == BaseListRef::npos);
  ensure("object in root", diagram->rootLayer()->figures()->get_index(figure) != BaseListRef::npos);

  check_redo();
  ensure("move redo", *figure->left() != x);
  ensure("object moved into layer redo", figure->layer() == layer);
  ensure_equals("layer contains stuff after redo", layer->figures().count(), 1U);
  ensure("layer contains object", layer->figures().get_index(figure) != BaseListRef::npos);
  ensure("object not in root", diagram->rootLayer()->figures()->get_index(figure) == BaseListRef::npos);

  // Move object out of layer.
  double new_x = figure->left();

  double mouse_x = figure->left() + layer->left();
  double mouse_y = figure->top() + layer->top();

  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, (int)(mouse_x + 5), (int)(mouse_y + 5), (mdc::EventState)0);
  diagram_form->handle_mouse_move(10, 10, mdc::SLeftButtonMask);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, 10, 10, mdc::SLeftButtonMask);
  check_only_one_undo_added();

  ensure("object moved", *figure->left() != new_x);
  ensure("object moved out of layer", figure->layer() == diagram->rootLayer());
  ensure("layer is empty", layer->figures().get_index(figure) == BaseListRef::npos);
  ensure("object in root layer", diagram->rootLayer()->figures().get_index(figure) != BaseListRef::npos);

  check_undo();

  ensure("object moved undo", *figure->left() == new_x);
  ensure("object moved out of layer undo", figure->layer() == layer);
  ensure("layer is not empty", layer->figures().get_index(figure) != BaseListRef::npos);
  ensure("object not in root layer", diagram->rootLayer()->figures().get_index(figure) == BaseListRef::npos);

  check_redo();
  ensure("object moved", *figure->left() != new_x);
  ensure("object moved out of layer", figure->layer() == diagram->rootLayer());
  ensure("layer is empty", layer->figures().get_index(figure) == BaseListRef::npos);
  ensure("object in root layer", diagram->rootLayer()->figures().get_index(figure) != BaseListRef::npos);

  // undo both operations
  check_undo();
  check_undo();
}

template <class C>
static void resize_object(ModelDiagramForm *diagram_form, C obj, double w, double h) {
  double px, py;

  px = obj->left() + obj->width() + 1;
  py = obj->top() + obj->height() + 1;

  // Click once to select the layer.
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, (int)px - 20, (int)py - 20, mdc::SNone);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, (int)px - 20, (int)py - 20, mdc::SNone);

  // Resize it by dragging the lower right resize handle.
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, (int)px, (int)py, mdc::SNone);
  diagram_form->handle_mouse_move((int)(obj->left() + w), (int)(obj->top() + h), mdc::SLeftButtonMask);

  // We cannot switch auto scrolling off in the diagram so we revert its effect when we performed mouse dragging.
  diagram_form->get_view()->set_offset(Point(0, 0));
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, (int)(obj->left() + w), (int)(obj->top() + h),
                                    mdc::SLeftButtonMask);
}

template <class C>
static void drag_object(ModelDiagramForm *diagram_form, C object, double deltaX, double deltaY) {
  double x = object->left();
  double y = object->top();

  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, (int)(x + 20), (int)(y + 13), mdc::SNone);
  diagram_form->handle_mouse_move((int)(x + deltaX + 20), (int)(y + deltaY + 13), mdc::SLeftButtonMask);
  diagram_form->handle_mouse_button(mdc::ButtonLeft, false, (int)(x + deltaX + 20), (int)(y + deltaY + 13),
                                    mdc::SLeftButtonMask);
}

TEST_FUNCTION(24) // Move Layer
{
  // Not implemented.
}

TEST_FUNCTION(26) // Move Layer under object to capture it
{
  // Not implemented.
}

TEST_FUNCTION(28) // Resize Layer to eat a figure
{
  model_LayerRef layer(diagram->layers()[0]);

  place_figure_with_tool(WB_TOOL_NOTE, 580, 480);
  check_only_one_undo_added();

  diagram->unselectAll();

  model_FigureRef figure(find_named_object_in_list(diagram->figures(), "text1"));

  // resize the layer to cover figure
  resize_object(diagram_form, layer, 800, 800);

  check_only_one_undo_added();

  ensure_equals("layer resized properly", *layer->width(), 800);
  ensure_equals("layer resized properly", *layer->height(), 800);

  // At this point the layer should have captured the text figure, but there's a long term bug
  // pending where figure <-> layer relationship is only updated when a figure was dragged
  // (regardless which of both was dragged, layer or child figure).
  // TODO: For now we drag the layer a bit to make this work. Needs to be addressed sooner or later.
  drag_object(diagram_form, layer, 10, 10);
  check_only_one_undo_added();

  ensure("layer contains object", layer->figures().get_index(figure) != BaseListRef::npos);
  ensure("object not in root", diagram->rootLayer()->figures().get_index(figure) == BaseListRef::npos);
  ensure("object layer changed", figure->layer() == layer);

  check_undo();
  ensure("layer not contains object", layer->figures().get_index(figure) == BaseListRef::npos);
  ensure("object in root", diagram->rootLayer()->figures().get_index(figure) != BaseListRef::npos);
  ensure("object layer changed", figure->layer() != layer);

  check_redo();
  ensure("layer contains object", layer->figures().get_index(figure) != BaseListRef::npos);
  ensure("object not in root", diagram->rootLayer()->figures().get_index(figure) == BaseListRef::npos);
  ensure("object layer changed", figure->layer() == layer);

  check_undo(); // Layer dragging.
  check_undo(); // Layer resize.
  check_undo(); // Note add.
}

//  XXX: for now disabled as there's a bug which must be fixed first (but cannot right now).
//       Internal bug number #268.
// TEST_FUNCTION(30) // Resize Table
// {
//   model_FigureRef figure(find_named_object_in_list(diagram->figures(), "table1"));
//
//   ensure("table found", figure.is_valid());
//
//   double w,h;
//   w= figure->width();
//   h= figure->height();
//
//   // resize the figure
//   resize_object(diagram_form, figure, 150, 200);
//   check_only_one_undo_added();
//
//   ensure_equals("Table width is wrong", *figure->width(), 150);
//   ensure_equals("Table height is wrong", *figure->height(), 200);
//
//   check_undo();
//   ensure_equals("Table width is wrong", *figure->width(), w);
//   ensure_equals("Table height is wrong", *figure->height(), h);
//
//   check_redo();
//   ensure_equals("Table width is wrong", *figure->width(), 150);
//   ensure_equals("Table height is wrong", *figure->height(), 200);
//   check_undo();
// }

static mforms::DialogResult message_ok_callback() {
  return mforms::ResultOk;
}

TEST_FUNCTION(32) // Delete Table
{
  model_FigureRef figure(find_named_object_in_list(diagram->figures(), "table1"));

  ensure("table found", figure.is_valid());

  // delete the figure
  mforms::stub::UtilitiesWrapper::set_message_callback(message_ok_callback);
  wb::WBContextUI::get()->get_wb()->get_model_context()->delete_object(figure);
  check_only_one_undo_added();

  ensure("table delete", !find_named_object_in_list(diagram->figures(), "table1").is_valid());
  ensure("table delete", !find_named_object_in_list(diagram->rootLayer()->figures(), "table1").is_valid());

  check_undo();
  ensure("table delete undo", find_named_object_in_list(diagram->figures(), "table1").is_valid());
  ensure("table delete undo", find_named_object_in_list(diagram->rootLayer()->figures(), "table1").is_valid());

  check_redo();
  ensure("table delete redo", !find_named_object_in_list(diagram->figures(), "table1").is_valid());
  ensure("table delete redo", !find_named_object_in_list(diagram->rootLayer()->figures(), "table1").is_valid());

  check_undo();
}

TEST_FUNCTION(34) // Delete layer with stuff inside
{
  // Not implemented.
}

TEST_FUNCTION(36) // Place with Drag/Drop
{
  db_TableRef table(find_named_object_in_list(tester->get_pmodel()->catalog()->schemata()[0]->tables(), "table3"));

  ensure("table found", table.is_valid());

  std::list<GrtObjectRef> list;
  list.push_back(table);
  diagram_form->perform_drop(10, 10, WB_DBOBJECT_DRAG_TYPE, list);
  check_only_one_undo_added();

  ensure("table figure added", find_named_object_in_list(diagram->figures(), "table3").is_valid());
  model_FigureRef figure(find_named_object_in_list(diagram->figures(), "table3"));
  ensure("figure has proper layer set", figure->layer().is_valid());
  ensure("figure has proper layer set", figure->layer() == diagram->rootLayer());
  ensure("figure has proper owner set", figure->owner() == diagram);

  check_undo();
  ensure("table figure added undo", !find_named_object_in_list(diagram->figures(), "table3").is_valid());

  check_redo();
  ensure("table figure added", find_named_object_in_list(diagram->figures(), "table3").is_valid());

  check_undo();
}

TEST_FUNCTION(38) // Place with Drag/Drop on a layer
{
  db_TableRef table(find_named_object_in_list(tester->get_pmodel()->catalog()->schemata()[0]->tables(), "table3"));

  ensure("table found", table.is_valid());

  std::list<GrtObjectRef> list;
  list.push_back(table);
  diagram_form->perform_drop(200, 500, WB_DBOBJECT_DRAG_TYPE, list);
  check_only_one_undo_added();

  model_LayerRef layer(diagram->layers()[0]);
  model_FigureRef figure;

  figure = find_named_object_in_list(diagram->figures(), "table3");

  ensure("table figure added", figure.is_valid());
  ensure("table in layer", figure->layer() == layer);
  ensure("layer contains table", layer->figures().get_index(figure) != BaseListRef::npos);

  check_undo();
  ensure("table figure added undo", !find_named_object_in_list(diagram->figures(), "table3").is_valid());
  // not needed ensure("table not in layer", !figure->layer().is_valid());
  ensure("layer not contains table", layer->figures().get_index(figure) == BaseListRef::npos);

  check_redo();
  ensure("table figure added", find_named_object_in_list(diagram->figures(), "table3").is_valid());
  ensure("table in layer", figure->layer() == layer);
  ensure("layer contains table", layer->figures().get_index(figure) != BaseListRef::npos);

  check_undo();
}

TEST_FUNCTION(40) // Create Relationship (in diagram)
{
  diagram_form->set_tool(WB_TOOL_PREL1n);

  model_FigureRef table1(find_named_object_in_list(diagram->figures(), "table1"));
  model_FigureRef table2(find_named_object_in_list(diagram->figures(), "table2"));

  ensure("found tables", table1.is_valid() && table2.is_valid());
  ensure_equals("rel count", diagram->connections().count(), 1U);

  // click table1
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(*table1->left() + 20),
                                    static_cast<int>(*table1->top() + 20), (mdc::EventState)0);
  // click table2
  diagram_form->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(*table2->left() + 20),
                                    static_cast<int>(*table2->top() + 20), (mdc::EventState)0);

  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 2U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count after redo", diagram->connections().count(), 2U);

  check_undo();
}

TEST_FUNCTION(42) // Create Relationship (indirectly with FK)
{
  db_TableRef table1(find_named_object_in_list(tester->get_pmodel()->catalog()->schemata()[0]->tables(), "table1"));
  db_TableRef table2(find_named_object_in_list(tester->get_pmodel()->catalog()->schemata()[0]->tables(), "table2"));

  ensure("table valid", table1.is_valid() && table2.is_valid());
  ensure_equals("rel count", diagram->connections().count(), 1U);

  db_ForeignKeyRef fk;

  fk = bec::TableHelper::create_foreign_key_to_table(table1, table2, true, true, true, false, tester->get_rdbms(),
                                                     DictRef(true), DictRef(true));
  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 2U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count after redo", diagram->connections().count(), 2U);

  check_undo();
}

TEST_FUNCTION(44) // Create Relationship (dropping table with FK)
{
  db_TableRef table(
    find_named_object_in_list(tester->get_pmodel()->catalog()->schemata()[0]->tables(), "table_with_fk"));

  ensure("table found", table.is_valid());
  ensure_equals("rel count", diagram->connections().count(), 1U);

  std::list<GrtObjectRef> list;
  list.push_back(table);
  diagram_form->perform_drop(200, 500, WB_DBOBJECT_DRAG_TYPE, list);
  check_only_one_undo_added();

  model_FigureRef figure;

  figure = find_named_object_in_list(diagram->figures(), "table_with_fk");

  ensure("table figure added", figure.is_valid());
  ensure_equals("rel count", diagram->connections().count(), 2U);

  check_undo();
  ensure("table figure added undo", !find_named_object_in_list(diagram->figures(), "table_with_fk").is_valid());
  ensure_equals("rel count", diagram->connections().count(), 1U);

  check_redo();
  ensure("table figure added redo", find_named_object_in_list(diagram->figures(), "table_with_fk").is_valid());
  ensure_equals("rel count", diagram->connections().count(), 2U);

  check_undo();
}

TEST_FUNCTION(46) // Create Relationship (dropping table referenced by FK)
{
  ensure_equals("rel count", diagram->connections().count(), 1U);
  ensure("Table already in diagram", !find_named_object_in_list(diagram->figures(), "table_with_fk").is_valid());

  ensure_equals("Wrong figure count", diagram->figures().count(), 5U);
  ensure_equals("Wrong relationship count", diagram->connections().count(), 1U);

  // Now drag/drop the table to the diagram.
  std::list<GrtObjectRef> list;
  list.push_back(find_named_object_in_list(tester->get_pmodel()->catalog()->schemata()[0]->tables(), "table_with_fk"));
  diagram_form->perform_drop(10, 10, WB_DBOBJECT_DRAG_TYPE, list);
  check_only_one_undo_added();

  ensure("Table not found", find_named_object_in_list(diagram->figures(), "table_with_fk").is_valid());
  ensure_equals("Wrong relationship count", diagram->connections().count(), 2U);

  check_undo();
  ensure_equals("Wrong figure count", diagram->figures().count(), 5U);
  ensure_equals("Wrong relationship count", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("Wrong figure count", diagram->figures().count(), 6U);
  ensure_equals("Wrong relationship count", diagram->connections().count(), 2U);

  check_undo();
}

TEST_FUNCTION(48) // Delete Relationship (in diagram)
{
  ensure_equals("rel count", diagram->connections().count(), 1U);

  model_ConnectionRef conn(diagram->connections()[0]);

  diagram->unselectAll();

  diagram->selectObject(conn);
  ensure_equals("selection", diagram->selection().count(), 1U);

  // Message callback set in case 32.
  diagram_form->delete_selection();
  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
}

TEST_FUNCTION(50) // Delete Relationship (indirectly with FK)
{
  db_TableRef table2(find_named_object_in_list(tester->get_pmodel()->catalog()->schemata()[0]->tables(), "table2"));

  ensure("table valid", table2.is_valid());
  ensure_equals("rel count", diagram->connections().count(), 1U);
  ensure_equals("fk count", table2->foreignKeys().count(), 1U);

  db_ColumnRef column(table2->columns()[1]);

  table2->removeColumn(column);
  check_only_one_undo_added();

  ensure_equals("fk count", table2->foreignKeys().count(), 0U);
  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);
  ensure_equals("fk count", table2->foreignKeys().count(), 1U);

  check_redo();
  ensure_equals("rel count after redo", diagram->connections().count(), 0U);

  check_undo();
}

TEST_FUNCTION(52) // Delete Relationship (deleted table)
{
  ensure_equals("rel count", diagram->connections().count(), 1U);

  model_ConnectionRef conn(diagram->connections()[0]);

  diagram->unselectAll();

  diagram->selectObject(find_named_object_in_list(diagram->figures(), "table2"));
  ensure_equals("selection", diagram->selection().count(), 1U);

  // Message callback set in case 32.
  diagram_form->delete_selection();
  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
}

static mforms::DialogResult message_cancel_callback() {
  return mforms::ResultCancel;
}

TEST_FUNCTION(54) // Delete Relationship (deleted table figure only)
{
  ensure_equals("rel count", diagram->connections().count(), 1U);

  model_ConnectionRef conn(diagram->connections()[0]);

  diagram->unselectAll();

  diagram->selectObject(find_named_object_in_list(diagram->figures(), "table2"));
  ensure_equals("selection", diagram->selection().count(), 1U);

  // Keep db objects
  mforms::stub::UtilitiesWrapper::set_message_callback(message_cancel_callback);
  diagram_form->delete_selection();
  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
}

TEST_FUNCTION(56) // Delete Relationship (deleted referenced table)
{
  ensure_equals("rel count", diagram->connections().count(), 1U);

  model_ConnectionRef conn(diagram->connections()[0]);

  diagram->unselectAll();

  diagram->selectObject(find_named_object_in_list(diagram->figures(), "table1"));
  ensure_equals("selection", diagram->selection().count(), 1U);

  diagram_form->delete_selection();
  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
}

TEST_FUNCTION(58) // Delete Relationship and Ref Table
{
  ensure_equals("rel count", diagram->connections().count(), 1U);

  model_ConnectionRef conn(diagram->connections()[0]);

  diagram->unselectAll();

  diagram->selectObject(conn->endFigure());
  diagram->selectObject(conn);
  ensure_equals("selection", diagram->selection().count(), 2U);

  diagram_form->delete_selection();
  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
}

TEST_FUNCTION(60) // Delete Relationship and Table
{
  ensure_equals("rel count", diagram->connections().count(), 1U);

  model_ConnectionRef conn(diagram->connections()[0]);

  diagram->unselectAll();

  diagram->selectObject(conn);
  diagram->selectObject(conn->startFigure());
  ensure_equals("selection", diagram->selection().count(), 2U);

  mforms::stub::UtilitiesWrapper::set_message_callback(message_ok_callback);
  diagram_form->delete_selection();
  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
}

TEST_FUNCTION(62) // Delete Relationship and both Tables
{
  ensure_equals("rel count", diagram->connections().count(), 1U);

  model_ConnectionRef conn(diagram->connections()[0]);

  diagram->unselectAll();

  diagram->selectObject(conn->startFigure());
  diagram->selectObject(conn);
  diagram->selectObject(conn->endFigure());
  ensure_equals("selection", diagram->selection().count(), 3U);

  mforms::stub::UtilitiesWrapper::set_message_callback(message_ok_callback);
  diagram_form->delete_selection();
  check_only_one_undo_added();

  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();
  ensure_equals("rel count after undo", diagram->connections().count(), 1U);

  check_redo();
  ensure_equals("rel count", diagram->connections().count(), 0U);

  check_undo();

  ensure("Could not close document", tester->close_document());
  tester->wb->close_document_finish();
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
