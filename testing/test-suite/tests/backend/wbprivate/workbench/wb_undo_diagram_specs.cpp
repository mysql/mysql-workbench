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

#include "grtdb/db_object_helpers.h"
#include "model/wb_history_tree.h"
#include "wb_overview.h"
#include "wb_model_diagram_form.h"
#include "wb_component_basic.h"

#include "stub/stub_utilities.h"

#include "casmine.h"
#include "grt_test_helpers.h"
#include "wb_test_helpers.h"

using namespace bec;
using namespace wb;
using namespace grt;
using namespace casmine;

namespace {

//----------------------------------------------------------------------------------------------------------------------

static mforms::DialogResult message_ok_callback() {
  return mforms::ResultOk;
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::DialogResult message_cancel_callback() {
  return mforms::ResultCancel;
}

//----------------------------------------------------------------------------------------------------------------------

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  UndoManager *um = nullptr;
  OverviewBE *overview = nullptr;
  ModelDiagramForm *diagramForm = nullptr;
  model_DiagramRef diagram;
  size_t lastUndoStackSize;
  size_t lastRedoStackSize;

  std::string dataDir;

  #include "wb_undo_helpers.h"

  void placeFigureWithTool(const std::string &tool, double x = 10, double y = 10) {
    diagramForm->set_tool(tool);
    diagramForm->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(x), static_cast<int>(y),
                                    (mdc::EventState)0);
  }

  //--------------------------------------------------------------------------------------------------------------------

  template <class C>
  void resizeObject(ModelDiagramForm *diagramForm, C obj, double w, double h) {
    double px, py;

    px = obj->left() + obj->width() + 1;
    py = obj->top() + obj->height() + 1;

    // Click once to select the layer.
    diagramForm->handle_mouse_button(mdc::ButtonLeft, true, (int)px - 20, (int)py - 20, mdc::SNone);
    diagramForm->handle_mouse_button(mdc::ButtonLeft, false, (int)px - 20, (int)py - 20, mdc::SNone);

    // Resize it by dragging the lower right resize handle.
    diagramForm->handle_mouse_button(mdc::ButtonLeft, true, (int)px, (int)py, mdc::SNone);
    diagramForm->handle_mouse_move((int)(obj->left() + w), (int)(obj->top() + h), mdc::SLeftButtonMask);

    // We cannot switch auto scrolling off in the diagram so we revert its effect when we performed mouse dragging.
    diagramForm->get_view()->set_offset(base::Point(0, 0));
    diagramForm->handle_mouse_button(mdc::ButtonLeft, false, (int)(obj->left() + w), (int)(obj->top() + h),
                                     mdc::SLeftButtonMask);
  }

  //--------------------------------------------------------------------------------------------------------------------

  template <class C>
  void dragObject(ModelDiagramForm *diagramForm, C object, double deltaX, double deltaY) {
    double x = object->left();
    double y = object->top();

    diagramForm->handle_mouse_button(mdc::ButtonLeft, true, (int)(x + 20), (int)(y + 13), mdc::SNone);
    diagramForm->handle_mouse_move((int)(x + deltaX + 20), (int)(y + deltaY + 13), mdc::SLeftButtonMask);
    diagramForm->handle_mouse_button(mdc::ButtonLeft, false, (int)(x + deltaX + 20), (int)(y + deltaY + 13),
                                     mdc::SLeftButtonMask);
  }

  //--------------------------------------------------------------------------------------------------------------------

};

$describe("Undo/Redo for Diagram Actions in Workbench") {
  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
    data->tester->initializeRuntime();
    data->dataDir = casmine::CasmineContext::get()->tmpDataDir();

    data->um = grt::GRT::get()->get_undo_manager();
    data->overview = wb::WBContextUI::get()->get_physical_overview();

    data->lastUndoStackSize = 0;
    data->lastRedoStackSize = 0;
    bool flag = data->tester->wb->open_document(data->dataDir + "/workbench/undo_test_model1.mwb");
    $expect(flag).toBeTrue("open_document");
    $expect(data->tester->getCatalog()->schemata().count()).toEqual(1U, "schemas");

    db_SchemaRef schema(data->tester->getCatalog()->schemata()[0]);

    // Make sure the loaded model contains expected number of things.
    $expect(schema->tables().count()).toEqual(4U, "tables");
    $expect(schema->views().count()).toEqual(1U, "views");
    $expect(schema->routineGroups().count()).toEqual(1U, "groups");

    $expect(data->tester->getPmodel()->diagrams().count()).toEqual(1U, "diagrams");
    data->diagram = data->tester->getPmodel()->diagrams()[0];

    $expect(data->diagram->figures().count()).toEqual(5U, "figures");
    $expect(data->diagram->layers().count()).toEqual(1U, "layers");

    data->tester->openAllDiagrams();
    data->tester->syncView();

    data->diagramForm = data->tester->wb->get_model_context()->get_diagram_form_for_diagram_id(data->tester->getPview().id());
    $expect(data->diagramForm).Not.toBeNull("Diagram form is invalid");

    mforms::ToolBar *toolbar = data->diagramForm->get_tools_toolbar();
    $expect(toolbar).Not.toBeNull("Toolbar creation failed");

    wb::WBContextUI::get()->set_active_form(data->diagramForm);
    $expect(data->um->get_undo_stack().size()).toEqual(0U, "undo stack is empty");

    // Model file not closed by intention. It's used in following test cases.
  });

  $afterAll([this]() {
    $expect(data->tester->closeDocument()).toBeTrue("Could not close document");
    data->tester->wb->close_document_finish();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place table", [this]() {
    db_SchemaRef schema = data->tester->getCatalog()->schemata()[0];
    size_t old_figure_count = data->diagram->figures().count();
    size_t old_root_figure_count = data->diagram->rootLayer()->figures().count();
    size_t old_object_count = schema->tables().count();

    WBComponentPhysical *compo = wb::WBContextUI::get()->get_wb()->get_component<WBComponentPhysical>();

    compo->place_new_db_object(data->diagramForm, base::Point(10, 10), wb::ObjectTable);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add");
    $expect(schema->tables().count()).toEqual(old_object_count + 1, "table add");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add");

    data->checkUndo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count, "figure add undo");
    $expect(schema->tables().count()).toEqual(old_object_count, "table add undo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count, "figure root add undo");

    data->checkRedo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add redo");
    $expect(schema->tables().count()).toEqual(old_object_count + 1, "table add redo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place view", [this]() {
    db_SchemaRef schema = data->tester->getCatalog()->schemata()[0];
    size_t old_figure_count = data->diagram->figures().count();
    size_t old_root_figure_count = data->diagram->rootLayer()->figures().count();
    size_t old_object_count = schema->views().count();

    WBComponentPhysical *compo = wb::WBContextUI::get()->get_wb()->get_component<WBComponentPhysical>();

    compo->place_new_db_object(data->diagramForm, base::Point(10, 10), wb::ObjectView);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add");
    $expect(schema->views().count()).toEqual(old_object_count + 1, "data->diagramForm add");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add");

    data->checkUndo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count, "figure add undo");
    $expect(schema->views().count()).toEqual(old_object_count, "data->diagramForm add undo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count, "figure root add undo");

    data->checkRedo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add redo");
    $expect(schema->views().count()).toEqual(old_object_count + 1, "data->diagramForm add redo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place Routine Group", [this]() {
    db_SchemaRef schema = data->tester->getCatalog()->schemata()[0];
    size_t old_figure_count = data->diagram->figures().count();
    size_t old_root_figure_count = data->diagram->rootLayer()->figures().count();
    size_t old_object_count = schema->routineGroups().count();

    WBComponentPhysical *compo = wb::WBContextUI::get()->get_wb()->get_component<WBComponentPhysical>();

    compo->place_new_db_object(data->diagramForm, base::Point(10, 10), wb::ObjectRoutineGroup);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add");
    $expect(schema->routineGroups().count()).toEqual(old_object_count + 1, "group add");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add");

    data->checkUndo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count, "figure add undo");
    $expect(schema->routineGroups().count()).toEqual(old_object_count, "group add undo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count, "figure root add undo");

    data->checkRedo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add redo");
    $expect(schema->routineGroups().count()).toEqual(old_object_count + 1, "group add redo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place Image", [this]() {
    size_t old_figure_count = data->diagram->figures().count();
    size_t old_root_figure_count = data->diagram->rootLayer()->figures().count();

    // Place image will ask for a filename of the image.
    data->tester->addFileForFileDialog(data->dataDir + "/images/sakila.png");

    data->placeFigureWithTool(WB_TOOL_IMAGE);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add");

    data->checkUndo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count, "figure add undo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count, "figure root add undo");

    data->checkRedo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add redo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place text", [this]() {
    size_t old_figure_count = data->diagram->figures().count();
    size_t old_root_figure_count = data->diagram->rootLayer()->figures().count();

    data->placeFigureWithTool(WB_TOOL_NOTE);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add");

    data->checkUndo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count, "figure add undo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count, "figure root add undo");

    data->checkRedo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "figure add redo");
    $expect(data->diagram->rootLayer()->figures().count()).toEqual(old_root_figure_count + 1, "figure root add redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place layer", [this]() {
    size_t old_layer_count = data->diagram->layers().count();
    size_t old_root_layer_count = data->diagram->rootLayer()->subLayers().count();

    data->diagramForm->set_tool(WB_TOOL_LAYER);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, 10, 10, (mdc::EventState)0);
    data->diagramForm->handle_mouse_move(50, 50, mdc::SLeftButtonMask);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, false, 50, 50, (mdc::EventState)0);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->layers().count()).toEqual(old_layer_count + 1, "layer add");
    $expect(data->diagram->rootLayer()->subLayers().count()).toEqual(old_root_layer_count + 1, "layer root add");

    data->checkUndo();
    $expect(data->diagram->layers().count()).toEqual(old_layer_count, "layer add undo");
    $expect(data->diagram->rootLayer()->subLayers().count()).toEqual(old_root_layer_count, "layer root add undo");

    data->checkRedo();
    $expect(data->diagram->layers().count()).toEqual(old_layer_count + 1, "layer add redo");
    $expect(data->diagram->rootLayer()->subLayers().count()).toEqual(old_root_layer_count + 1, "layer root add redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place something inside a layer", [this]() {
    size_t old_figure_count = data->diagram->figures().count();
    size_t old_layer_count = data->diagram->layers().count();
    size_t old_root_layer_count = data->diagram->rootLayer()->subLayers().count();

    // place layer
    data->diagramForm->set_tool(WB_TOOL_LAYER);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, 10, 10, (mdc::EventState)0);
    data->diagramForm->handle_mouse_move(150, 150, (mdc::EventState)0);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, false, 150, 150, (mdc::EventState)0);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->layers().count()).toEqual(old_layer_count + 1, "layer add");
    $expect(data->diagram->rootLayer()->subLayers().count()).toEqual(old_root_layer_count + 1, "layer root add");

    model_LayerRef layer(data->diagram->layers()[old_layer_count]);
    $expect(layer.is_valid()).toBeTrue("layer");
    $expect(layer->figures().count()).toEqual(0U, "layer empty");

    // place a note inside the layer
    data->placeFigureWithTool(WB_TOOL_NOTE, 50, 50);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "note add");
    model_FigureRef figure(data->diagram->figures()[old_figure_count]);

    $expect(*figure->top()).toEqual(40, "new note pos");
    $expect(*figure->left()).toEqual(40, "new note pos");

    $expect(layer->figures().count()).toEqual(1U, "layer contains figure");
    $expect(figure->layer()).toEqual(layer, "note layer");

    data->checkUndo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count, "note add undo");
    $expect(layer->figures().count()).toEqual(0U, "layer contains figure undo");

    data->checkRedo();
    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "note add undo");
    $expect(layer->figures().count()).toEqual(1U, "layer contains figure undo");

    data->checkUndo();
    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place layer around something", [this]() {
    size_t old_figure_count = data->diagram->figures().count();
    size_t old_layer_count = data->diagram->layers().count();
    size_t old_root_layer_count = data->diagram->rootLayer()->subLayers().count();

    // place a note
    data->diagramForm->set_tool(WB_TOOL_NOTE);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, 200, 200, (mdc::EventState)0);
    data->checkOnlyOneUndoAdded();

    model_FigureRef figure(data->diagram->figures()[old_figure_count]);

    $expect(data->diagram->figures().count()).toEqual(old_figure_count + 1, "note add");

    // place layer around the note
    data->diagramForm->set_tool(WB_TOOL_LAYER);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, 190, 190, (mdc::EventState)0);
    data->diagramForm->handle_mouse_move(300, 300, (mdc::EventState)0);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, false, 300, 300, (mdc::EventState)0);
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->layers().count()).toEqual(old_layer_count + 1, "layer add");
    $expect(data->diagram->rootLayer()->subLayers().count()).toEqual(old_root_layer_count + 1, "layer root add");

    model_LayerRef layer(data->diagram->layers()[old_layer_count]);
    $expect(layer.is_valid()).toBeTrue("layer");
    $expect(figure->layer()).toEqual(layer, "note layer");

    $expect(layer->figures().count()).toEqual(1U, "layer contains note only");
    $expect(layer->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "layer contains note only");

    $expect(data->diagram->layers().count()).toEqual(old_layer_count + 1, "layer add");
    $expect(data->diagram->rootLayer()->subLayers().count()).toEqual(old_root_layer_count + 1, "layer root add");
    $expect(layer->figures().count()).toEqual(1U, "layer note");
    $expect(layer->figures().count()).toEqual(1U, "root layer note");

    data->checkUndo();
    $expect(data->diagram->layers().count()).toEqual(old_layer_count, "layer add undo");
    $expect(data->diagram->rootLayer()->subLayers().count()).toEqual(old_root_layer_count, "layer root add undo");
    $expect(layer->figures().count()).toEqual(0U, "layer note");

    data->checkRedo();
    $expect(data->diagram->layers().count()).toEqual(old_layer_count + 1, "layer add redo");
    $expect(data->diagram->rootLayer()->subLayers().count()).toEqual(old_root_layer_count + 1, "layer root add redo");
    $expect(layer->figures().count()).toEqual(1U, "layer note");

    data->checkUndo(); // undo the layer
    data->checkUndo(); // undo the note place
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Move object", [this]() {
    double x, y;
    model_FigureRef figure(data->diagram->figures()[0]);

    x = figure->left();
    y = figure->top();

    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(x + 5), static_cast<int>(y + 5),
                                           (mdc::EventState)0);
    data->diagramForm->handle_mouse_move(50, 50, mdc::SLeftButtonMask);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, false, 50, 50, mdc::SLeftButtonMask);
    data->checkOnlyOneUndoAdded();

    $expect(*figure->left()).Not.toEqual(x, "object moved");

    data->checkUndo();
    $expect(*figure->left()).toEqual(x, "move undo");

    data->checkRedo();
    $expect(*figure->left()).Not.toEqual(x, "move redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Move into and out of layer", [this]() {
    double x, y;
    model_FigureRef figure(data->diagram->figures()[0]);
    model_LayerRef layer(data->diagram->layers()[0]);

    $expect(layer).Not.toEqual(data->diagram->rootLayer(), "layer is not root");

    x = figure->left();
    y = figure->top();

    $expect(figure->layer()).toEqual(data->diagram->rootLayer(), "object is in root");
    $expect(layer->figures().count()).toEqual(0U, "layer begins empty");

    // move object into layer
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(x + 5), static_cast<int>(y + 5),
                                           (mdc::EventState)0);
    data->diagramForm->handle_mouse_move(150, 400, mdc::SLeftButtonMask);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, false, 150, 400, mdc::SLeftButtonMask);
    data->checkOnlyOneUndoAdded();

    $expect(*figure->left()).Not.toEqual(x, "object moved");
    $expect(figure->layer()).toEqual(layer, "object moved into layer");
    $expect(layer->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "layer contains object");
    $expect(data->diagram->rootLayer()->figures()->get_index(figure)).toEqual(BaseListRef::npos, "object not in root");

    data->checkUndo();
    $expect(*figure->left()).toEqual(x, "move undo");
    $expect(figure->layer()).toEqual(data->diagram->rootLayer(), "object moved into layer undo");
    $expect(layer->figures().count()).toEqual(0U, "layer empty on undo");
    $expect(layer->figures().get_index(figure)).toEqual(BaseListRef::npos, "layer does not contain object");
    $expect(data->diagram->rootLayer()->figures()->get_index(figure)).Not.toEqual(BaseListRef::npos, "object in root");

    data->checkRedo();
    $expect(*figure->left()).Not.toEqual(x, "move redo");
    $expect(figure->layer()).toEqual(layer, "object moved into layer redo");
    $expect(layer->figures().count()).toEqual(1U, "layer contains stuff after redo");
    $expect(layer->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "layer contains object");
    $expect(data->diagram->rootLayer()->figures()->get_index(figure)).toEqual(BaseListRef::npos, "object not in root");

    // Move object out of layer.
    double new_x = figure->left();

    double mouse_x = figure->left() + layer->left();
    double mouse_y = figure->top() + layer->top();

    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, (int)(mouse_x + 5), (int)(mouse_y + 5), (mdc::EventState)0);
    data->diagramForm->handle_mouse_move(10, 10, mdc::SLeftButtonMask);
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, false, 10, 10, mdc::SLeftButtonMask);
    data->checkOnlyOneUndoAdded();

    $expect(*figure->left()).Not.toEqual(new_x, "object moved");
    $expect(figure->layer()).toEqual(data->diagram->rootLayer(), "object moved out of layer");
    $expect(layer->figures().get_index(figure)).toEqual(BaseListRef::npos, "layer is empty");
    $expect(data->diagram->rootLayer()->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "object in root layer");

    data->checkUndo();

    $expect(*figure->left()).toEqual(new_x, "object moved undo");
    $expect(figure->layer()).toEqual(layer, "object moved out of layer undo");
    $expect(layer->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "layer is not empty");
    $expect(data->diagram->rootLayer()->figures().get_index(figure)).toEqual(BaseListRef::npos, "object not in root layer");

    data->checkRedo();
    $expect(*figure->left()).Not.toEqual(new_x, "object moved");
    $expect(figure->layer()).toEqual(data->diagram->rootLayer(), "object moved out of layer");
    $expect(layer->figures().get_index(figure)).toEqual(BaseListRef::npos, "layer is empty");
    $expect(data->diagram->rootLayer()->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "object in root layer");

    // undo both operations
    data->checkUndo();
    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Move layer", []() {
    $pending("not implemented");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Move layer under object to capture it", []() {
    $pending("not implemented");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Resize layer to eat a figure", [this]() {
    model_LayerRef layer(data->diagram->layers()[0]);

    data->placeFigureWithTool(WB_TOOL_NOTE, 580, 480);
    data->checkOnlyOneUndoAdded();

    data->diagram->unselectAll();

    model_FigureRef figure(find_named_object_in_list(data->diagram->figures(), "text1"));

    // resize the layer to cover figure
    data->resizeObject(data->diagramForm, layer, 800, 800);

    data->checkOnlyOneUndoAdded();

    $expect(*layer->width()).toEqual(800, "layer resized properly");
    $expect(*layer->height()).toEqual(800, "layer resized properly");

    // At this point the layer should have captured the text figure, but there's a long term bug
    // pending where figure <-> layer relationship is only updated when a figure was dragged
    // (regardless which of both was dragged, layer or child figure).
    // TODO: For now we drag the layer a bit to make this work. Needs to be addressed sooner or later.
    data->dragObject(data->diagramForm, layer, 10, 10);
    data->checkOnlyOneUndoAdded();

    $expect(layer->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "layer contains object");
    $expect(data->diagram->rootLayer()->figures().get_index(figure)).toEqual(BaseListRef::npos, "object not in root");
    $expect(figure->layer()).toEqual(layer, "object layer changed");

    data->checkUndo();
    $expect(layer->figures().get_index(figure)).toEqual(BaseListRef::npos, "layer not contains object");
    $expect(data->diagram->rootLayer()->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "object in root");
    $expect(figure->layer()).Not.toEqual(layer, "object layer changed");

    data->checkRedo();
    $expect(layer->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "layer contains object");
    $expect(data->diagram->rootLayer()->figures().get_index(figure)).toEqual(BaseListRef::npos, "object not in root");
    $expect(figure->layer()).toEqual(layer, "object layer changed");

    data->checkUndo(); // Layer dragging.
    data->checkUndo(); // Layer resize.
    data->checkUndo(); // Note add.
  });

  //--------------------------------------------------------------------------------------------------------------------

  //  XXX: for now disabled as there's a bug which must be fixed first (but cannot right now).
  //       Internal bug number #268.
  // $it(30) // Resize Table
  // {
  //   model_FigureRef figure(find_named_object_in_list(data->diagram->figures(), "table1"));
  //
  //   $expect("table found", figure.is_valid());
  //
  //   double w,h;
  //   w= figure->width();
  //   h= figure->height();
  //
  //   // resize the figure
  //   resize_object(data->diagramForm, figure, 150, 200);
  //   data->checkOnlyOneUndoAdded();
  //
  //   $expect("Table width is wrong", *figure->width(), 150);
  //   $expect("Table height is wrong", *figure->height(), 200);
  //
  //   data->checkUndo();
  //   $expect("Table width is wrong", *figure->width(), w);
  //   $expect("Table height is wrong", *figure->height(), h);
  //
  //   data->checkUndo();
  //   $expect("Table width is wrong", *figure->width(), 150);
  //   $expect("Table height is wrong", *figure->height(), 200);
  //   data->checkUndo();
  // });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete table", [this]() {
    model_FigureRef figure(find_named_object_in_list(data->diagram->figures(), "table1"));

    $expect(figure.is_valid()).toBeTrue("table found");

    // delete the figure
    mforms::stub::UtilitiesWrapper::set_message_callback(message_ok_callback);
    wb::WBContextUI::get()->get_wb()->get_model_context()->delete_object(figure);
    data->checkOnlyOneUndoAdded();

    $expect(find_named_object_in_list(data->diagram->figures(), "table1").is_valid()).toBeFalse("table delete");
    $expect(find_named_object_in_list(data->diagram->rootLayer()->figures(), "table1").is_valid()).toBeFalse("table delete");

    data->checkUndo();
    $expect(find_named_object_in_list(data->diagram->figures(), "table1").is_valid()).toBeTrue("table delete undo");
    $expect(find_named_object_in_list(data->diagram->rootLayer()->figures(), "table1").is_valid()).toBeTrue("table delete undo");

    data->checkRedo();
    $expect(find_named_object_in_list(data->diagram->figures(), "table1").is_valid()).toBeFalse("table delete redo");
    $expect(find_named_object_in_list(data->diagram->rootLayer()->figures(), "table1").is_valid()).toBeFalse("table delete redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete layer with stuff inside", []() {
    $pending("not implemented");
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place with drag/drop", [this]() {
    db_TableRef table(find_named_object_in_list(data->tester->getPmodel()->catalog()->schemata()[0]->tables(), "table3"));

    $expect(table.is_valid()).toBeTrue("table found");

    std::list<GrtObjectRef> list;
    list.push_back(table);
    data->diagramForm->perform_drop(10, 10, WB_DBOBJECT_DRAG_TYPE, list);
    data->checkOnlyOneUndoAdded();

    $expect(find_named_object_in_list(data->diagram->figures(), "table3").is_valid()).toBeTrue("table figure added");
    model_FigureRef figure(find_named_object_in_list(data->diagram->figures(), "table3"));
    $expect(figure->layer().is_valid()).toBeTrue("figure has proper layer set");
    $expect(figure->layer()).toEqual(data->diagram->rootLayer(), "figure has proper layer set");
    $expect(figure->owner()).toEqual(data->diagram, "figure has proper owner set");

    data->checkUndo();
    $expect(find_named_object_in_list(data->diagram->figures(), "table3").is_valid()).toBeFalse("table figure added undo");

    data->checkRedo();
    $expect(find_named_object_in_list(data->diagram->figures(), "table3").is_valid()).toBeTrue("table figure added");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Place with drag/drop on a layer", [this]() {
    db_TableRef table(find_named_object_in_list(data->tester->getPmodel()->catalog()->schemata()[0]->tables(), "table3"));

    $expect(table.is_valid()).toBeTrue("table found");

    std::list<GrtObjectRef> list;
    list.push_back(table);
    data->diagramForm->perform_drop(200, 500, WB_DBOBJECT_DRAG_TYPE, list);
    data->checkOnlyOneUndoAdded();

    model_LayerRef layer(data->diagram->layers()[0]);
    model_FigureRef figure;

    figure = find_named_object_in_list(data->diagram->figures(), "table3");

    $expect(figure.is_valid()).toBeTrue("table figure added");
    $expect(figure->layer()).toEqual(layer, "table in layer");
    $expect(layer->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "layer contains table");

    data->checkUndo();
    $expect(find_named_object_in_list(data->diagram->figures(), "table3").is_valid()).toBeFalse("table figure added undo");
    $expect(layer->figures().get_index(figure)).toEqual(BaseListRef::npos, "layer not contains table");

    data->checkRedo();
    $expect(find_named_object_in_list(data->diagram->figures(), "table3").is_valid()).toBeTrue("table figure added");
    $expect(figure->layer()).toEqual(layer, "table in layer");
    $expect(layer->figures().get_index(figure)).Not.toEqual(BaseListRef::npos, "layer contains table");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Create relationship (in diagram)", [this]() {
    data->diagramForm->set_tool(WB_TOOL_PREL1n);

    model_FigureRef table1(find_named_object_in_list(data->diagram->figures(), "table1"));
    model_FigureRef table2(find_named_object_in_list(data->diagram->figures(), "table2"));

    $expect(table1.is_valid() && table2.is_valid()).toBeTrue("found tables");
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    // click table1
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(*table1->left() + 20),
                                           static_cast<int>(*table1->top() + 20), (mdc::EventState)0);
    // click table2
    data->diagramForm->handle_mouse_button(mdc::ButtonLeft, true, static_cast<int>(*table2->left() + 20),
                                           static_cast<int>(*table2->top() + 20), (mdc::EventState)0);

    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(2U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(2U, "rel count after redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Create relationship (indirectly with FK)", [this]() {
    db_TableRef table1(find_named_object_in_list(data->tester->getPmodel()->catalog()->schemata()[0]->tables(), "table1"));
    db_TableRef table2(find_named_object_in_list(data->tester->getPmodel()->catalog()->schemata()[0]->tables(), "table2"));

    $expect(table1.is_valid() && table2.is_valid()).toBeTrue("table valid");
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    db_ForeignKeyRef fk;

    fk = bec::TableHelper::create_foreign_key_to_table(table1, table2, true, true, true, false,
      data->tester->getRdbms(), DictRef(true), DictRef(true));
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(2U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(2U, "rel count after redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Create relationship (dropping table with FK)", [this]() {
    db_TableRef table(
      find_named_object_in_list(data->tester->getPmodel()->catalog()->schemata()[0]->tables(), "table_with_fk")
    );

    $expect(table.is_valid()).toBeTrue("table found");
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    std::list<GrtObjectRef> list;
    list.push_back(table);
    data->diagramForm->perform_drop(200, 500, WB_DBOBJECT_DRAG_TYPE, list);
    data->checkOnlyOneUndoAdded();

    model_FigureRef figure;

    figure = find_named_object_in_list(data->diagram->figures(), "table_with_fk");

    $expect(figure.is_valid()).toBeTrue("table figure added");
    $expect(data->diagram->connections().count()).toEqual(2U, "rel count");

    data->checkUndo();
    $expect(find_named_object_in_list(data->diagram->figures(), "table_with_fk").is_valid()).toBeFalse("table figure added undo");
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    data->checkRedo();
    $expect(find_named_object_in_list(data->diagram->figures(), "table_with_fk").is_valid()).toBeTrue("table figure added redo");
    $expect(data->diagram->connections().count()).toEqual(2U, "rel count");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Create relationship (dropping table referenced by FK)", [this]() {
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");
    $expect(find_named_object_in_list(data->diagram->figures(), "table_with_fk").is_valid()).toBeFalse("Table already in diagram");

    $expect(data->diagram->figures().count()).toEqual(5U, "Wrong figure count");
    $expect(data->diagram->connections().count()).toEqual(1U, "Wrong relationship count");

    // Now drag/drop the table to the diagram.
    std::list<GrtObjectRef> list;
    list.push_back(find_named_object_in_list(data->tester->getPmodel()->catalog()->schemata()[0]->tables(), "table_with_fk"));
    data->diagramForm->perform_drop(10, 10, WB_DBOBJECT_DRAG_TYPE, list);
    data->checkOnlyOneUndoAdded();

    $expect(find_named_object_in_list(data->diagram->figures(), "table_with_fk").is_valid()).toBeTrue("Table not found");
    $expect(data->diagram->connections().count()).toEqual(2U, "Wrong relationship count");

    data->checkUndo();
    $expect(data->diagram->figures().count()).toEqual(5U, "Wrong figure count");
    $expect(data->diagram->connections().count()).toEqual(1U, "Wrong relationship count");

    data->checkRedo();
    $expect(data->diagram->figures().count()).toEqual(6U, "Wrong figure count");
    $expect(data->diagram->connections().count()).toEqual(2U, "Wrong relationship count");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete relationship (in diagram)", [this]() {
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    model_ConnectionRef conn(data->diagram->connections()[0]);

    data->diagram->unselectAll();

    data->diagram->selectObject(conn);
    $expect(data->diagram->selection().count()).toEqual(1U, "selection");

    // Message callback set in case 32.
    data->diagramForm->delete_selection();
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete relationship (indirectly with FK)", [this]() {
    db_TableRef table2(find_named_object_in_list(data->tester->getPmodel()->catalog()->schemata()[0]->tables(), "table2"));

    $expect(table2.is_valid()).toBeTrue("table valid");
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");
    $expect(table2->foreignKeys().count()).toEqual(1U, "fk count");

    db_ColumnRef column(table2->columns()[1]);

    table2->removeColumn(column);
    data->checkOnlyOneUndoAdded();

    $expect(table2->foreignKeys().count()).toEqual(0U, "fk count");
    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");
    $expect(table2->foreignKeys().count()).toEqual(1U, "fk count");

    data->checkRedo();
    $expect( data->diagram->connections().count()).toEqual(0U, "rel count after redo");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete relationship (deleted table)", [this]() {
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    model_ConnectionRef conn(data->diagram->connections()[0]);

    data->diagram->unselectAll();

    data->diagram->selectObject(find_named_object_in_list(data->diagram->figures(), "table2"));
    $expect(data->diagram->selection().count()).toEqual(1U, "selection");

    // Message callback set in case 32.
    data->diagramForm->delete_selection();
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete relationship (deleted table figure only)", [this]() {
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    model_ConnectionRef conn(data->diagram->connections()[0]);

    data->diagram->unselectAll();

    data->diagram->selectObject(find_named_object_in_list(data->diagram->figures(), "table2"));
    $expect(data->diagram->selection().count()).toEqual(1U, "selection");

    // Keep db objects
    mforms::stub::UtilitiesWrapper::set_message_callback(message_cancel_callback);
    data->diagramForm->delete_selection();
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete relationship (deleted referenced table)", [this]() {
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    model_ConnectionRef conn(data->diagram->connections()[0]);

    data->diagram->unselectAll();

    data->diagram->selectObject(find_named_object_in_list(data->diagram->figures(), "table1"));
    $expect(data->diagram->selection().count()).toEqual(1U, "selection");

    data->diagramForm->delete_selection();
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete relationship and Ref Table", [this]() {
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    model_ConnectionRef conn(data->diagram->connections()[0]);

    data->diagram->unselectAll();

    data->diagram->selectObject(conn->endFigure());
    data->diagram->selectObject(conn);
    $expect(data->diagram->selection().count()).toEqual(2U, "selection");

    data->diagramForm->delete_selection();
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete relationship and Table", [this]() {
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    model_ConnectionRef conn(data->diagram->connections()[0]);

    data->diagram->unselectAll();

    data->diagram->selectObject(conn);
    data->diagram->selectObject(conn->startFigure());
    $expect(data->diagram->selection().count()).toEqual(2U, "selection");

    mforms::stub::UtilitiesWrapper::set_message_callback(message_ok_callback);
    data->diagramForm->delete_selection();
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
  });

  //--------------------------------------------------------------------------------------------------------------------

  $it("Delete Relationship and both Tables", [this]() {
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count");

    model_ConnectionRef conn(data->diagram->connections()[0]);

    data->diagram->unselectAll();

    data->diagram->selectObject(conn->startFigure());
    data->diagram->selectObject(conn);
    data->diagram->selectObject(conn->endFigure());
    $expect(data->diagram->selection().count()).toEqual(3U, "selection");

    mforms::stub::UtilitiesWrapper::set_message_callback(message_ok_callback);
    data->diagramForm->delete_selection();
    data->checkOnlyOneUndoAdded();

    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
    $expect(data->diagram->connections().count()).toEqual(1U, "rel count after undo");

    data->checkRedo();
    $expect(data->diagram->connections().count()).toEqual(0U, "rel count");

    data->checkUndo();
  });

};

}
