/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "Canvas.h"

#include "ConvUtils.h"

using namespace MySQL::GUI::Mdc;
using namespace base;

//----------------- BaseWindowsCanvasView ----------------------------------------------------------

BaseWindowsCanvasView::BaseWindowsCanvasView() : inner(nullptr) {
}

//--------------------------------------------------------------------------------------------------

BaseWindowsCanvasView::~BaseWindowsCanvasView() {
  // XXX TODO
  // Let the owner know we are going away.
  // if (owner != NULL && owner->get_data() != NULL)
  //   owner->get_data()->unrealize();

  delete inner;
  ReleaseHandle();
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::on_queue_repaint_wrapper(int x, int y, int w, int h) {
  on_queue_repaint_delegate(x, y, w, h);
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::on_viewport_changed_wrapper() {
  on_viewport_changed_delegate();
}

//--------------------------------------------------------------------------------------------------

::mdc::CanvasView* BaseWindowsCanvasView::get_unmanaged_object() {
  return inner;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a fixed pointer to this object that will not be modified by the GC.
 */
IntPtr BaseWindowsCanvasView::GetFixedId() {
  if (!m_gch.IsAllocated)
    m_gch = GCHandle::Alloc(this);
  return GCHandle::ToIntPtr(m_gch);
}

//--------------------------------------------------------------------------------------------------

/**
 * Needs to be called when destroying the object.
 */
void BaseWindowsCanvasView::ReleaseHandle() {
  m_gch.Free();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the object based on the fixed pointer retrieved by GetFixedId().
 */
BaseWindowsCanvasView ^ BaseWindowsCanvasView::GetFromFixedId(IntPtr ip) {
  GCHandle gcHandle = GCHandle::FromIntPtr(ip);
  return (BaseWindowsCanvasView ^)gcHandle.Target;
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::set_on_queue_repaint(Void4IntDelegate ^ dt) {
  on_queue_repaint_delegate = dt;
  on_queue_repaint_wrapper_delegate =
    gcnew Void4IntWrapperDelegate(this, &BaseWindowsCanvasView::on_queue_repaint_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(on_queue_repaint_wrapper_delegate);
  VOID_4INT_CB cb = static_cast<VOID_4INT_CB>(ip.ToPointer());
  inner->signal_repaint()->connect(cb);
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::set_on_viewport_changed(VoidVoidDelegate ^ dt) {
  on_viewport_changed_delegate = dt;
  on_viewport_changed_wrapper_delegate =
    gcnew VoidVoidWrapperDelegate(this, &BaseWindowsCanvasView::on_viewport_changed_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(on_viewport_changed_wrapper_delegate);
  VOID_VOID_CB cb = static_cast<VOID_VOID_CB>(ip.ToPointer());
  inner->signal_viewport_changed()->connect(cb);
}

//--------------------------------------------------------------------------------------------------

bool BaseWindowsCanvasView::initialize() {
  return get_unmanaged_object()->initialize();
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::repaint(IntPtr hdc, int x, int y, int width, int height) {
  set_target_context((HDC)hdc.ToPointer());
  get_unmanaged_object()->repaint(x, y, width, height);
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::repaint(IntPtr hdc) {
  set_target_context((HDC)hdc.ToPointer());
  get_unmanaged_object()->repaint();
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::set_target_context(HDC hdc){};

//--------------------------------------------------------------------------------------------------

double BaseWindowsCanvasView::get_fps() {
  return get_unmanaged_object()->get_fps();
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::OnMouseMove(MouseEventArgs ^ e, Keys keystate, MouseButtons buttons) {
  get_unmanaged_object()->handle_mouse_move(e->X, e->Y, getEventState(keystate, buttons));
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::OnMouseDown(MouseEventArgs ^ e, Keys keystate, MouseButtons buttons) {
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
  get_unmanaged_object()->handle_mouse_button(butt, true, e->X, e->Y, getEventState(keystate, buttons));
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::OnMouseUp(MouseEventArgs ^ e, Keys keystate, MouseButtons buttons) {
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
  get_unmanaged_object()->handle_mouse_button(butt, false, e->X, e->Y, getEventState(keystate, buttons));
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::OnMouseDoubleClick(MouseEventArgs ^ e, Keys keystate, MouseButtons buttons) {
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
  get_unmanaged_object()->handle_mouse_double_click(butt, e->X, e->Y, getEventState(keystate, buttons));
}

//--------------------------------------------------------------------------------------------------

bool BaseWindowsCanvasView::OnKeyDown(KeyEventArgs ^ e, Keys keystate) {
  return get_unmanaged_object()->handle_key(getKeyInfo(e), true, getEventState(keystate, MouseButtons::None));
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::OnKeyUp(KeyEventArgs ^ e, Keys keystate) {
  get_unmanaged_object()->handle_key(getKeyInfo(e), false, getEventState(keystate, MouseButtons::None));
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::OnSizeChanged(int w, int h) {
  ::mdc::CanvasView* canvas = get_unmanaged_object();

  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;

  if (canvas != nullptr)
    canvas->update_view_size(w, h);
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::SetOwnerForm(System::Windows::Forms::Form ^ ownerForm) {
  owner_form = ownerForm;
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::Form ^ BaseWindowsCanvasView::GetOwnerForm() {
  return owner_form;
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::get_viewport_range([Out] double % x, [Out] double % y, [Out] double % w, [Out] double % h) {
  Rect rect;
  rect = get_unmanaged_object()->get_viewport_range();
  x = rect.left();
  y = rect.top();
  w = rect.width();
  h = rect.height();
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::get_viewport([Out] double % x, [Out] double % y, [Out] double % w, [Out] double % h) {
  Rect rect;
  rect = get_unmanaged_object()->get_viewport();
  x = rect.left();
  y = rect.top();
  w = rect.width();
  h = rect.height();
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::set_offset(double x, double y) {
  get_unmanaged_object()->set_offset(base::Point(x, y));
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::scroll_to(double x, double y) {
  get_unmanaged_object()->scroll_to(base::Point(x, y));
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::get_total_view_size([Out] double % w, [Out] double % h) {
  base::Size size;
  size = get_unmanaged_object()->get_total_view_size();
  w = size.width;
  h = size.height;
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::window_to_canvas(int x, int y, [Out] double % ox, [Out] double % oy) {
  base::Point p = get_unmanaged_object()->window_to_canvas(x, y);
  ox = p.x;
  oy = p.y;
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::window_to_canvas(int x, int y, int w, int h, [Out] double % ox, [Out] double % oy,
                                             [Out] double % ow, [Out] double % oh) {
  base::Rect r = get_unmanaged_object()->window_to_canvas(x, y, w, h);
  ox = r.pos.x;
  oy = r.pos.y;
  ow = r.size.width;
  oh = r.size.height;
}

//--------------------------------------------------------------------------------------------------

void BaseWindowsCanvasView::update_view_size(int w, int h) {
  get_unmanaged_object()->update_view_size(w, h);
}

//--------------------------------------------------------------------------------------------------

mdc::EventState BaseWindowsCanvasView::getEventState(Keys keys, MouseButtons buttons) {
  mdc::EventState state = ((mdc::EventState)0);

  if ((keys & Keys::Control) == Keys::Control)
    state = state | mdc::SControlMask;
  if ((keys & Keys::Shift) == Keys::Shift)
    state = state | mdc::SShiftMask;
  if ((keys & Keys::Alt) == Keys::Alt)
    state = state | mdc::SAltMask;

  if ((buttons & MouseButtons::Left) == MouseButtons::Left)
    state = state | mdc::SLeftButtonMask;
  if ((buttons & MouseButtons::Middle) == MouseButtons::Middle)
    state = state | mdc::SMiddleButtonMask;
  if ((buttons & MouseButtons::Right) == MouseButtons::Right)
    state = state | mdc::SRightButtonMask;

  return state;
}

//--------------------------------------------------------------------------------------------------

mdc::KeyInfo BaseWindowsCanvasView::getKeyInfo(KeyEventArgs ^ e) {
  static KeyCodeMapping keycodes[] = {{Keys::Back, mdc::KBackspace},
                                      {Keys::Delete, mdc::KDelete},
                                      {Keys::Down, mdc::KDown},
                                      {Keys::End, mdc::KEnd},
                                      {Keys::Enter, mdc::KEnter},
                                      {Keys::Escape, mdc::KEscape},
                                      {Keys::F1, mdc::KF1},
                                      {Keys::F2, mdc::KF2},
                                      {Keys::F3, mdc::KF3},
                                      {Keys::F4, mdc::KF4},
                                      {Keys::F5, mdc::KF5},
                                      {Keys::F6, mdc::KF6},
                                      {Keys::F7, mdc::KF7},
                                      {Keys::F8, mdc::KF8},
                                      {Keys::F9, mdc::KF9},
                                      {Keys::F10, mdc::KF10},
                                      {Keys::F11, mdc::KF11},
                                      {Keys::F12, mdc::KF12},
                                      {Keys::Home, mdc::KHome},
                                      {Keys::Insert, mdc::KInsert},
                                      {Keys::Left, mdc::KLeft},
                                      {Keys::Next, mdc::KPageDown},
                                      {Keys::PageDown, mdc::KPageDown},
                                      {Keys::PageUp, mdc::KPageUp},
                                      {Keys::Prior, mdc::KPageUp},
                                      {Keys::Return, mdc::KEnter},
                                      {Keys::Shift, mdc::KShift},
                                      {Keys::Tab, mdc::KTab},

                                      {Keys::Space, mdc::KSpace},
                                      {Keys::OemPeriod, mdc::KPeriod},
                                      {Keys::Oemcomma, mdc::KComma},
                                      {Keys::OemSemicolon, mdc::KSemicolon}};

  mdc::KeyInfo k;

  k.keycode = mdc::KNone;
  k.string = "";
  for (int i = 0; i < sizeof(keycodes) / sizeof(*keycodes); i++) {
    if (keycodes[i].key == e->KeyCode) {
      k.keycode = keycodes[i].kcode;
      break;
    }
  }

  if (k.keycode == 0) {
    KeysConverter ^ c = gcnew KeysConverter();
    k.string = MySQL::NativeToCppString(c->ConvertToString(e->KeyCode));
  }

  return k;
}

//--------------------------------------------------------------------------------------------------
