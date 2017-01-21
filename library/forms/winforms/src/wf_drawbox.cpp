/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_drawbox.h"

using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Windows::Forms::Layout;

using namespace MySQL;
using namespace MySQL::Forms;

using namespace System::Drawing;

//--------------------------------------------------------------------------------------------------

/**
 * Converts Windows specific mouse button identifiers to plain numbers for the back end.
 */
static int convert_mouse_button(MouseButtons button) {
  switch (button) {
    case MouseButtons::Left:
      return 0;
    case MouseButtons::Right:
      return 1;
    case MouseButtons::Middle:
      return 2;
    default:
      return -1;
  }
}

//--------------------------------------------------------------------------------------------------

static System::Windows::Forms::AccessibleRole convert_accessible_role(mforms::Accessible::Role be_role) {
  System::Windows::Forms::AccessibleRole role = System::Windows::Forms::AccessibleRole::None;

  switch (be_role) {
    case mforms::Accessible::RoleNone:
      role = System::Windows::Forms::AccessibleRole::None;
      break;

    case mforms::Accessible::Client:
      role = System::Windows::Forms::AccessibleRole::Client;
      break;

    case mforms::Accessible::Pane:
      role = System::Windows::Forms::AccessibleRole::Pane;
      break;

    case mforms::Accessible::Link:
      role = System::Windows::Forms::AccessibleRole::Link;
      break;

    case mforms::Accessible::List:
      role = System::Windows::Forms::AccessibleRole::List;
      break;

    case mforms::Accessible::ListItem:
      role = System::Windows::Forms::AccessibleRole::ListItem;
      break;

    case mforms::Accessible::PushButton:
      role = System::Windows::Forms::AccessibleRole::PushButton;
      break;

    case mforms::Accessible::StaticText:
      role = System::Windows::Forms::AccessibleRole::StaticText;
      break;

    case mforms::Accessible::Text:
      role = System::Windows::Forms::AccessibleRole::Text;
      break;

    case mforms::Accessible::Outline:
      role = System::Windows::Forms::AccessibleRole::Outline;
      break;

    case mforms::Accessible::OutlineButton:
      role = System::Windows::Forms::AccessibleRole::OutlineButton;
      break;

    case mforms::Accessible::OutlineItem:
      role = System::Windows::Forms::AccessibleRole::OutlineItem;
      break;
  }
  return role;
}

//----------------- WBControlAccessibleObject ------------------------------------------------------

WBControlAccessibleObject::WBControlAccessibleObject(Control ^ owner, mforms::Accessible *backendOwner)
  : ControlAccessibleObject(owner) {
  backend = backendOwner;
}

//--------------------------------------------------------------------------------------------------

String ^ WBControlAccessibleObject::Name::get() {
  return CppStringToNativeRaw(backend->get_acc_name());
}

//--------------------------------------------------------------------------------------------------

int WBControlAccessibleObject::GetChildCount() {
  return backend->get_acc_child_count();
}

//--------------------------------------------------------------------------------------------------

String ^ WBControlAccessibleObject::Description::get() {
  return CppStringToNative(backend->get_acc_description());
}

//--------------------------------------------------------------------------------------------------

String ^ WBControlAccessibleObject::DefaultAction::get() {
  return CppStringToNativeRaw(backend->get_acc_default_action());
}

//--------------------------------------------------------------------------------------------------

String ^ WBControlAccessibleObject::Value::get() {
  return CppStringToNative(backend->get_acc_value());
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::AccessibleRole WBControlAccessibleObject::Role::get() {
  return convert_accessible_role(backend->get_acc_role());
}

//--------------------------------------------------------------------------------------------------

void WBControlAccessibleObject::DoDefaultAction() {
  return backend->do_default_action();
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::AccessibleObject ^ WBControlAccessibleObject::GetChild(int index) {
  mforms::Accessible *child = backend->get_acc_child(index);

  if (child)
    return gcnew WBAccessibleObject(child, this);
  else
    return nullptr;
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::AccessibleObject ^ WBControlAccessibleObject::HitTest(int x, int y) {
  Point point = Owner->PointToClient(Point(x, y));

  mforms::Accessible *accessible = backend->hit_test(point.X, point.Y);

  if (accessible)
    return gcnew WBAccessibleObject(accessible, this);
  else if (Owner->RectangleToScreen(Owner->ClientRectangle).Contains(x, y))
    return this;

  return nullptr;
}

//----------------- WBAccessibleObject -------------------------------------------------------------

WBAccessibleObject::WBAccessibleObject(mforms::Accessible *backendOwner, WBControlAccessibleObject ^ parent_control) {
  backend = backendOwner;
  parent = parent_control;
}

//--------------------------------------------------------------------------------------------------

String ^ WBAccessibleObject::Name::get() {
  return CppStringToNativeRaw(backend->get_acc_name());
}

//--------------------------------------------------------------------------------------------------

String ^ WBAccessibleObject::Description::get() {
  return CppStringToNative(backend->get_acc_description());
}

//--------------------------------------------------------------------------------------------------

String ^ WBAccessibleObject::DefaultAction::get() {
  return CppStringToNativeRaw(backend->get_acc_default_action());
}

//--------------------------------------------------------------------------------------------------

String ^ WBAccessibleObject::Value::get() {
  return CppStringToNative(backend->get_acc_value());
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::AccessibleRole WBAccessibleObject::Role::get() {
  return convert_accessible_role(backend->get_acc_role());
}

//--------------------------------------------------------------------------------------------------

System::Drawing::Rectangle WBAccessibleObject::Bounds::get() {
  base::Rect backend_bounds = backend->get_acc_bounds();
  System::Drawing::Rectangle bounds = System::Drawing::Rectangle(
    (int)backend_bounds.left(), (int)backend_bounds.top(), (int)backend_bounds.width(), (int)backend_bounds.height());

  if (parent)
    bounds = parent->Owner->RectangleToScreen(bounds);

  return bounds;
}

//--------------------------------------------------------------------------------------------------

int WBAccessibleObject::GetChildCount() {
  return backend->get_acc_child_count();
}

//--------------------------------------------------------------------------------------------------

void WBAccessibleObject::DoDefaultAction() {
  return backend->do_default_action();
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::AccessibleObject ^ WBAccessibleObject::GetChild(int index) {
  mforms::Accessible *child = backend->get_acc_child(index);

  if (child)
    return gcnew WBAccessibleObject(child, parent);
  else
    return nullptr;
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::AccessibleObject ^ WBAccessibleObject::HitTest(int x, int y) {
  mforms::Accessible *accessible = backend->hit_test(x, y);

  if (accessible && accessible != backend)
    return gcnew WBAccessibleObject(accessible, parent);
  else
    return nullptr;
}

//----------------- CanvasControl ------------------------------------------------------------------

CanvasControl::CanvasControl() {
  // Enable custom draw and double buffering for flicker reduction.
  SetStyle(ControlStyles::UserPaint, true);
  SetStyle(ControlStyles::AllPaintingInWmPaint, true);
  SetStyle(ControlStyles::DoubleBuffer, true);
  // SetStyle(ControlStyles::SupportsTransparentBackColor, true);
  SetStyle(ControlStyles::OptimizedDoubleBuffer, true);
  UpdateStyles();
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::AccessibleObject ^ CanvasControl::CreateAccessibilityInstance() {
  return gcnew WBControlAccessibleObject(this, backend);
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::Layout::LayoutEngine ^ CanvasControl::LayoutEngine::get() {
  if (layoutEngine == nullptr)
    layoutEngine = gcnew DrawBoxLayout();
  return layoutEngine;
}

//--------------------------------------------------------------------------------------------------

System::Drawing::Size CanvasControl::GetPreferredSize(System::Drawing::Size proposedSize) {
  base::Size nativeSize = backend->getLayoutSize(base::Size(proposedSize.Width, proposedSize.Height));
  System::Drawing::Size minSize = MinimumSize;
  if (minSize.Width > nativeSize.width)
    nativeSize.width = minSize.Width;
  if (minSize.Height > nativeSize.height)
    nativeSize.height = minSize.Height;
  return System::Drawing::Size((int)nativeSize.width, (int)nativeSize.height);
}

//--------------------------------------------------------------------------------------------------

void CanvasControl::Add(Control ^ control, mforms::Alignment alignment) {
  ViewWrapper::set_layout_dirty(this, true);
  alignments[control] = alignment;
  Controls->Add(control);
}

//--------------------------------------------------------------------------------------------------

void CanvasControl::Remove(Control ^ control) {
  Controls->Remove(control);
  alignments.Remove(control);
}

//--------------------------------------------------------------------------------------------------

void CanvasControl::Move(Control ^ control, int x, int y) {
  alignments[control] = mforms::NoAlign;
  control->Location = Point(x, y);
}

//--------------------------------------------------------------------------------------------------

mforms::Alignment CanvasControl::GetAlignment(Control ^ control) {
  return (mforms::Alignment)alignments[control];
}

//--------------------------------------------------------------------------------------------------

void CanvasControl::SetBackend(mforms::DrawBox *backend) {
  this->backend = backend;
}

//--------------------------------------------------------------------------------------------------

void CanvasControl::DoRepaint() {
  Invalidate();
}

//--------------------------------------------------------------------------------------------------

void CanvasControl::OnKeyDown(KeyEventArgs ^ args) {
  __super ::OnKeyDown(args);

  if (args->KeyCode == Keys::Escape)
    backend->cancel_operation();
}

//--------------------------------------------------------------------------------------------------

void CanvasControl::OnPaint(PaintEventArgs ^ args) {
  if (backend != NULL) {
    IntPtr hdc = args->Graphics->GetHdc();

    cairo_surface_t *surface = cairo_win32_surface_create(static_cast<HDC>(hdc.ToPointer()));
    cairo_t *cr = cairo_create(surface);
    backend->repaint(cr, args->ClipRectangle.X, args->ClipRectangle.Y, args->ClipRectangle.Width,
                     args->ClipRectangle.Height);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    args->Graphics->ReleaseHdc(hdc);
  }
}

//----------------- DrawBoxLayout ------------------------------------------------------------------

bool DrawBoxLayout::Layout(Object ^ sender, LayoutEventArgs ^ arguments) {
  CanvasControl ^ canvas = (CanvasControl ^)sender;
  Size canvasSize = canvas->Size;
  for each(Control ^ control in canvas->Controls) {
      int x, y;

      switch (canvas->GetAlignment(control)) {
        case mforms::BottomLeft:
        case mforms::MiddleLeft:
        case mforms::TopLeft:
          x = canvas->Padding.Left;
          break;

        case mforms::BottomCenter:
        case mforms::MiddleCenter:
        case mforms::TopCenter:
          x = (canvasSize.Width - control->Size.Width) / 2;
          break;

        case mforms::BottomRight:
        case mforms::MiddleRight:
        case mforms::TopRight:
          x = canvasSize.Width - canvas->Padding.Right - control->Size.Width;
          break;

        default:
          x = 0;
          break;
      }

      switch (canvas->GetAlignment(control)) {
        case mforms::BottomLeft:
        case mforms::BottomCenter:
        case mforms::BottomRight:
          y = canvasSize.Height - canvas->Padding.Bottom - control->Size.Height;
          break;

        case mforms::MiddleLeft:
        case mforms::MiddleCenter:
        case mforms::MiddleRight:
          y = (canvasSize.Height - control->Size.Height) / 2;
          break;

        case mforms::TopLeft:
        case mforms::TopCenter:
        case mforms::TopRight:
          y = canvas->Padding.Top;
          break;

        default:
          y = 0;
          break;
      }

      control->Location = Point(x, y);
    }

  return false; // We don't resize the control as result of a layout process.
}

//----------------- DrawBoxWrapper -----------------------------------------------------------------

DrawBoxWrapper::DrawBoxWrapper(mforms::DrawBox *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool DrawBoxWrapper::create(mforms::DrawBox *backend) {
  DrawBoxWrapper *wrapper = new DrawBoxWrapper(backend);

  CanvasControl ^ canvas = DrawBoxWrapper::Create<CanvasControl>(backend, wrapper);
  canvas->SetBackend(backend);

  return true;
}

//--------------------------------------------------------------------------------------------------

void DrawBoxWrapper::set_needs_repaint(mforms::DrawBox *backend) {
  CanvasControl ^ canvas = DrawBoxWrapper::GetManagedObject<CanvasControl>(backend);
  canvas->Invalidate();
}

//--------------------------------------------------------------------------------------------------

void DrawBoxWrapper::add(mforms::DrawBox *backend, mforms::View *view, mforms::Alignment alignment) {
  CanvasControl ^ canvas = DrawBoxWrapper::GetManagedObject<CanvasControl>(backend);
  canvas->Add(DrawBoxWrapper::GetControl(view), alignment);
}

//--------------------------------------------------------------------------------------------------

void DrawBoxWrapper::remove(mforms::DrawBox *backend, mforms::View *view) {
  CanvasControl ^ canvas = DrawBoxWrapper::GetManagedObject<CanvasControl>(backend);
  canvas->Remove(DrawBoxWrapper::GetControl(view));
}

//--------------------------------------------------------------------------------------------------

void DrawBoxWrapper::move(mforms::DrawBox *backend, mforms::View *view, int x, int y) {
  CanvasControl ^ canvas = DrawBoxWrapper::GetManagedObject<CanvasControl>(backend);
  canvas->Move(DrawBoxWrapper::GetControl(view), x, y);
}

//--------------------------------------------------------------------------------------------------

void DrawBoxWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_drawbox_impl.create = &DrawBoxWrapper::create;
  f->_drawbox_impl.set_needs_repaint = &DrawBoxWrapper::set_needs_repaint;
  f->_drawbox_impl.add = &DrawBoxWrapper::add;
  f->_drawbox_impl.remove = &DrawBoxWrapper::remove;
  f->_drawbox_impl.move = &DrawBoxWrapper::move;
}

//--------------------------------------------------------------------------------------------------
