/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/treeview.h"

#include "workbench/wb_context.h"
#include "model/wb_model_diagram_form.h"

#include "ModelDiagramFormWrapper.h"

#include "ConvUtils.h"

using namespace System::Windows::Forms;

using namespace Aga::Controls::Tree;

using namespace wb;

using namespace MySQL::Base;
using namespace MySQL::Grt;
using namespace MySQL::Forms;
using namespace MySQL::GUI::Mdc;
using namespace MySQL::Workbench;

//--------------------------------------------------------------------------------------------------

ModelDiagramFormWrapper::ModelDiagramFormWrapper(ModelDiagramForm *inn) : UIForm(inn) {
}

//--------------------------------------------------------------------------------------------------

ModelDiagramFormWrapper::~ModelDiagramFormWrapper() {
  init(NULL); // Reset the reference to the backend object. We are not responsible for it.
}

//--------------------------------------------------------------------------------------------------

ModelDiagramForm *ModelDiagramFormWrapper::get_unmanaged_object() {
  return (ModelDiagramForm *)inner;
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::OnMouseMove(MouseEventArgs ^ e, int X, int Y, Keys keystate, MouseButtons buttons) {
  get_unmanaged_object()->handle_mouse_move(X, Y, BaseWindowsCanvasView::getEventState(keystate, buttons));
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::OnMouseDown(MouseEventArgs ^ e, int X, int Y, Keys keystate, MouseButtons buttons) {
  mdc::MouseButton butt = mdc::ButtonLeft;
  switch (e->Button) {
    case MouseButtons::Left:
      butt = mdc::ButtonLeft;
      break;

    case MouseButtons::Middle:
      butt = mdc::ButtonMiddle;
      break;

    case MouseButtons::Right:
      butt = mdc::ButtonRight;
      break;
  }
  get_unmanaged_object()->handle_mouse_button(butt, true, X, Y,
                                              BaseWindowsCanvasView::getEventState(keystate, buttons));
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::OnMouseUp(MouseEventArgs ^ e, int X, int Y, Keys keystate, MouseButtons buttons) {
  mdc::MouseButton butt = mdc::ButtonLeft;
  switch (e->Button) {
    case MouseButtons::Left:
      butt = mdc::ButtonLeft;
      break;

    case MouseButtons::Middle:
      butt = mdc::ButtonMiddle;
      break;

    case MouseButtons::Right:
      butt = mdc::ButtonRight;
      break;
  }
  get_unmanaged_object()->handle_mouse_button(butt, false, X, Y,
                                              BaseWindowsCanvasView::getEventState(keystate, buttons));
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::OnMouseDoubleClick(MouseEventArgs ^ e, int X, int Y, Keys keystate,
                                                 MouseButtons buttons) {
  mdc::MouseButton butt = mdc::ButtonLeft;
  switch (e->Button) {
    case MouseButtons::Left:
      butt = mdc::ButtonLeft;
      break;

    case MouseButtons::Middle:
      butt = mdc::ButtonMiddle;
      break;

    case MouseButtons::Right:
      butt = mdc::ButtonRight;
      break;
  }
  get_unmanaged_object()->handle_mouse_double_click(butt, X, Y,
                                                    BaseWindowsCanvasView::getEventState(keystate, buttons));
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::OnKeyDown(KeyEventArgs ^ e, Keys keystate) {
  if (get_unmanaged_object()->handle_key(BaseWindowsCanvasView::getKeyInfo(e), true,
                                         BaseWindowsCanvasView::getEventState(keystate, MouseButtons::None))) {
    // If the keyboard input has been handled then tell the caller not to process this any further.
    e->SuppressKeyPress = true;
  }
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::OnKeyUp(KeyEventArgs ^ e, Keys keystate) {
  get_unmanaged_object()->handle_key(BaseWindowsCanvasView::getKeyInfo(e), false,
                                     BaseWindowsCanvasView::getEventState(keystate, MouseButtons::None));
}

//--------------------------------------------------------------------------------------------------

String ^ ModelDiagramFormWrapper::get_tool_cursor() {
  return CppStringToNative(get_unmanaged_object()->get_cursor());
}

//--------------------------------------------------------------------------------------------------

bool ModelDiagramFormWrapper::accepts_drop(int x, int y, IDataObject ^ data) {
  void *native_data = ObjectMapper::ManagedToNativeDragData(data, WB_DBOBJECT_DRAG_TYPE);
  if (native_data == NULL)
    return false;

  std::list<GrtObjectRef> *objects = reinterpret_cast<std::list<GrtObjectRef> *>(native_data);
  return get_unmanaged_object()->accepts_drop(x, y, WB_DBOBJECT_DRAG_TYPE, *objects);
}

//--------------------------------------------------------------------------------------------------

bool ModelDiagramFormWrapper::accepts_drop(int x, int y, String ^ type, String ^ text) {
  return get_unmanaged_object()->accepts_drop(x, y, NativeToCppString(type), NativeToCppString(text));
}

//--------------------------------------------------------------------------------------------------

bool ModelDiagramFormWrapper::perform_drop(int x, int y, IDataObject ^ data) {
  void *native_data = ObjectMapper::ManagedToNativeDragData(data, WB_DBOBJECT_DRAG_TYPE);
  if (native_data == NULL)
    return false;

  std::list<GrtObjectRef> *objects = reinterpret_cast<std::list<GrtObjectRef> *>(native_data);
  return get_unmanaged_object()->perform_drop(x, y, WB_DBOBJECT_DRAG_TYPE, *objects);
}

//--------------------------------------------------------------------------------------------------

bool ModelDiagramFormWrapper::perform_drop(int x, int y, String ^ type, String ^ text) {
  return get_unmanaged_object()->perform_drop(x, y, NativeToCppString(type), NativeToCppString(text));
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::set_closed(bool flag) {
  if (get_unmanaged_object())
    get_unmanaged_object()->set_closed(flag);
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::close() {
  if (get_unmanaged_object())
    get_unmanaged_object()->close();
}

//--------------------------------------------------------------------------------------------------

bool ModelDiagramFormWrapper::is_closed() {
  if (get_unmanaged_object())
    return get_unmanaged_object()->is_closed();
  return false;
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::setup_mini_view(BaseWindowsCanvasView ^ view) {
  // Ownership of the given view remains with the caller, which is responsible for freeing it.
  mini_view = view;
  get_unmanaged_object()->setup_mini_view(mini_view->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::update_mini_view_size(int w, int h) {
  if (get_unmanaged_object() != NULL)
    get_unmanaged_object()->update_mini_view_size(w, h);
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::update_options_toolbar() {
  get_unmanaged_object()->update_options_toolbar();
}

//--------------------------------------------------------------------------------------------------

double ModelDiagramFormWrapper::get_zoom() {
  if (get_unmanaged_object() != NULL)
    return get_unmanaged_object()->get_zoom();

  return 1;
}

//--------------------------------------------------------------------------------------------------

void ModelDiagramFormWrapper::set_zoom(double zoom) {
  if (get_unmanaged_object() != NULL)
    get_unmanaged_object()->set_zoom(zoom);
}

//--------------------------------------------------------------------------------------------------

String ^ ModelDiagramFormWrapper::get_title() {
  return CppStringToNative(get_unmanaged_object()->get_title());
}

//--------------------------------------------------------------------------------------------------

ToolStrip ^ ModelDiagramFormWrapper::get_tools_toolbar() {
  mforms::ToolBar *toolbar = get_unmanaged_object()->get_tools_toolbar();
  if (toolbar == NULL)
    return nullptr;

  return dynamic_cast<ToolStrip ^>(ObjectMapper::GetManagedComponent(toolbar));
}

//--------------------------------------------------------------------------------------------------

ToolStrip ^ ModelDiagramFormWrapper::get_options_toolbar() {
  mforms::ToolBar *toolbar = get_unmanaged_object()->get_options_toolbar();
  if (toolbar == NULL)
    return nullptr;

  return dynamic_cast<ToolStrip ^>(ObjectMapper::GetManagedComponent(toolbar));
}

//--------------------------------------------------------------------------------------------------

TreeViewAdv ^ ModelDiagramFormWrapper::get_layer_tree() {
  mforms::TreeView *tree = get_unmanaged_object()->get_layer_tree();

  return dynamic_cast<TreeViewAdv ^>(ObjectMapper::GetManagedComponent(tree));
}

//--------------------------------------------------------------------------------------------------

TreeViewAdv ^ ModelDiagramFormWrapper::get_catalog_tree() {
  return dynamic_cast<TreeViewAdv ^>(ObjectMapper::GetManagedComponent(get_unmanaged_object()->get_catalog_tree()));
}

//--------------------------------------------------------------------------------------------------
