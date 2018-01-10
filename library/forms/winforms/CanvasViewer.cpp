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

#include "CanvasViewer.h"

using namespace System::Windows::Forms;

using namespace MySQL::GUI::Mdc;

WindowsCanvasViewer::WindowsCanvasViewer() {
  canvasPanel = gcnew WindowsCanvasViewerPanel();

  canvasPanel->VScrollbar = gcnew VScrollBar();
  canvasPanel->HScrollbar = gcnew HScrollBar();

  canvasPanel->VScrollbar->Scroll += gcnew ScrollEventHandler(canvasPanel, &WindowsCanvasViewerPanel::HandleScroll);
  canvasPanel->HScrollbar->Scroll += gcnew ScrollEventHandler(canvasPanel, &WindowsCanvasViewerPanel::HandleScroll);

  Controls->Add(canvasPanel->VScrollbar);
  Controls->Add(canvasPanel->HScrollbar);
  Controls->Add(canvasPanel);

  canvasPanel->Dock = DockStyle::Fill;
  canvasPanel->HScrollbar->Dock = DockStyle::Bottom;
  canvasPanel->VScrollbar->Dock = DockStyle::Right;

  canvasPanel->Width = ClientSize.Width - canvasPanel->VScrollbar->Width;
  canvasPanel->Height = ClientSize.Height - canvasPanel->HScrollbar->Height;
}

BaseWindowsCanvasView ^ WindowsCanvasViewer::CreateCanvasView(Form ^ ownerForm, bool handleInput,
                                                              bool software_rendering_enforced,
                                                              bool opengl_rendering_enforced) {
  // Logger->LogDebug();

  BaseWindowsCanvasView ^ result = nullptr;

  if (!software_rendering_enforced || opengl_rendering_enforced)
    result = canvasPanel->CreateGLCanvas(ownerForm, handleInput);

  if (result == nullptr)
    result = canvasPanel->CreateGDICanvas(ownerForm, handleInput);

  return result;
}

//--------------------------------------------------------------------------------------

WindowsCanvasViewerPanel::WindowsCanvasViewerPanel() {
  Click += gcnew EventHandler(this, &WindowsCanvasViewerPanel::ScrollablePanel_Click);
  handleInput = true;
}

WindowsGLCanvasView ^ WindowsCanvasViewerPanel::CreateGLCanvas(Form ^ ownerForm, bool handleInput) {
  canvas = gcnew WindowsGLCanvasView(Handle, ClientRectangle.Width, ClientRectangle.Height);
  if (!canvas->initialize()) {
    // OpenGL initialization can fail.
    canvas = nullptr;
    return nullptr;
  }

  this->handleInput = handleInput;

  canvas->set_on_queue_repaint(
    gcnew BaseWindowsCanvasView::Void4IntDelegate(this, &WindowsCanvasViewerPanel::OnNeedsRepaint));
  canvas->set_on_viewport_changed(
    gcnew BaseWindowsCanvasView::VoidVoidDelegate(this, &WindowsCanvasViewerPanel::OnViewportChanged));
  OwnerForm = ownerForm;
  canvasInitialized = true;

  UpdateScrollBarSizes();
  return (WindowsGLCanvasView ^)canvas;
}

WindowsGDICanvasView ^ WindowsCanvasViewerPanel::CreateGDICanvas(Form ^ ownerForm, bool handleInput) {
  SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::UserPaint | ControlStyles::Opaque |
             ControlStyles::OptimizedDoubleBuffer,
           true);
  UpdateStyles();

  canvas = gcnew WindowsGDICanvasView(Handle, ClientRectangle.Width, ClientRectangle.Height);
  if (!canvas->initialize()) {
    // Should never happen.
    //  canvas->Dispose();
    return nullptr;
  }

  this->handleInput = handleInput;

  canvas->set_on_queue_repaint(
    gcnew BaseWindowsCanvasView::Void4IntDelegate(this, &WindowsCanvasViewerPanel::OnNeedsRepaint));
  canvas->set_on_viewport_changed(
    gcnew BaseWindowsCanvasView::VoidVoidDelegate(this, &WindowsCanvasViewerPanel::OnViewportChanged));
  OwnerForm = ownerForm;
  canvasInitialized = true;

  UpdateScrollBarSizes();

  return (WindowsGDICanvasView ^)canvas;
}

void WindowsCanvasViewerPanel::FinalizeCanvas() {
  canvasInitialized = false;
  canvas->SetOwnerForm(nullptr);
  // canvas->Dispose();
  canvas = nullptr;
}

void WindowsCanvasViewerPanel::OnNeedsRepaint(int x, int y, int w, int h) {
  Invalidate(gcnew System::Drawing::Region(System::Drawing::Rectangle(x, y, w, h)));
}

void WindowsCanvasViewerPanel::OnViewportChanged() {
  if (scrolling)
    return;
  UpdateScrollbars();
  canvas->scroll_to(hScrollbar->Value, vScrollbar->Value);
  Refresh();
}

void WindowsCanvasViewerPanel::OnMouseMove(MouseEventArgs ^ e) {
  if (canvasInitialized && handleInput)
    canvas->OnMouseMove(e, ModifierKeys, MouseButtons);
  Panel::OnMouseMove(e);
}

void WindowsCanvasViewerPanel::OnMouseDown(MouseEventArgs ^ e) {
  if (canvasInitialized && handleInput)
    canvas->OnMouseDown(e, ModifierKeys, MouseButtons);
  Panel::OnMouseDown(e);
}

void WindowsCanvasViewerPanel::OnMouseUp(MouseEventArgs ^ e) {
  if (canvasInitialized && handleInput)
    canvas->OnMouseUp(e, ModifierKeys, MouseButtons);
  Panel::OnMouseUp(e);
}

void WindowsCanvasViewerPanel::OnMouseDoubleClick(MouseEventArgs ^ e) {
  if (canvasInitialized && handleInput)
    canvas->OnMouseDoubleClick(e, ModifierKeys, MouseButtons);
  Panel::OnMouseDoubleClick(e);
}

void WindowsCanvasViewerPanel::OnMouseWheel(MouseEventArgs ^ e) {
  if (canvasInitialized) {
    if ((ModifierKeys & Keys::Alt) != (Keys)0) {
      // Zoom change.
      if (e->Delta > 0) {
        if (canvas->Zoom < 2)
          canvas->Zoom = canvas->Zoom + 0.01f;
      } else {
        if (canvas->Zoom > 0.1f)
          canvas->Zoom = canvas->Zoom - 0.01f;
      }
    } else {
      // Scroll change.
      double x, y, w, h;
      canvas->get_viewport(x, y, w, h);

      if ((ModifierKeys & Keys::Control) != (Keys)0)
        x -= e->Delta / 5;
      else
        y -= e->Delta / 5;

      if (y < 0)
        y = 0;
      else if (y > vScrollbar->Maximum)
        y = vScrollbar->Maximum;
      if (x < 0)
        x = 0;
      else if (x > hScrollbar->Maximum)
        x = hScrollbar->Maximum;

      if (vScrollbar->Value != (int)y)
        vScrollbar->Value = (int)y;
      if (hScrollbar->Value != (int)x)
        hScrollbar->Value = (int)x;

      HandleScroll(nullptr, nullptr);
    }
  }
  Panel::OnMouseWheel(e);
}

void WindowsCanvasViewerPanel::OnKeyDown(KeyEventArgs ^ e) {
  if (canvasInitialized)
    canvas->OnKeyDown(e, ModifierKeys);
  Panel::OnKeyDown(e);
}

void WindowsCanvasViewerPanel::OnKeyUp(KeyEventArgs ^ e) {
  if (canvasInitialized)
    canvas->OnKeyUp(e, ModifierKeys);
  Panel::OnKeyUp(e);
}

void WindowsCanvasViewerPanel::OnSizeChanged(EventArgs ^ e) {
  Panel::OnSizeChanged(e);
  if (canvasInitialized)
    canvas->OnSizeChanged(ClientRectangle.Width, ClientRectangle.Height);
}

void WindowsCanvasViewerPanel::ScrollablePanel_Click(Object ^ sender, EventArgs ^ e) {
  this->Focus();
}

void WindowsCanvasViewerPanel::OnPaintBackground(PaintEventArgs ^ e) {
  // Don't do anything to avoid flickering.
  if (canvas == nullptr)
    Panel::OnPaintBackground(e);
}

void WindowsCanvasViewerPanel::OnPaint(PaintEventArgs ^ e) {
  try {
    if (canvasInitialized) {
      IntPtr hdc = e->Graphics->GetHdc();
      canvas->repaint(hdc, e->ClipRectangle.Left, e->ClipRectangle.Top, e->ClipRectangle.Width,
                      e->ClipRectangle.Height);
      e->Graphics->ReleaseHdc();

      if (canvasFPSLabel != nullptr)
        canvasFPSLabel->Text = String::Format("{0:0.00} fps", canvas->get_fps());
    }
  } catch (Exception ^ exc) {
    MessageBox::Show(exc->Message);
  }
}

void WindowsCanvasViewerPanel::UpdateScrollbars() {
  UpdateScrollBarSizes();
  UpdateScrollBarPositions();
}

void WindowsCanvasViewerPanel::UpdateScrollBarPositions() {
  double x, y, w, h;

  if (canvas != nullptr) {
    canvas->get_viewport(x, y, w, h);

    if (y < 0)
      y = 0;
    else if (y > vScrollbar->Maximum)
      y = vScrollbar->Maximum;

    if (x < 0)
      x = 0;
    else if (x > hScrollbar->Maximum)
      x = hScrollbar->Maximum;

    if (vScrollbar->Value != (int)y)
      vScrollbar->Value = (int)y;
    if (hScrollbar->Value != (int)x)
      hScrollbar->Value = (int)x;
  }
}

void WindowsCanvasViewerPanel::UpdateScrollBarSizes() {
  double x, y, w, h;
  double total_w, total_h;

  if (canvas != nullptr) {
    canvas->get_total_view_size(total_w, total_h);

    canvas->get_viewport(x, y, w, h);

    vScrollbar->Minimum = 0;
    hScrollbar->Minimum = 0;

    vScrollbar->Visible = (total_h > h);
    vScrollbar->Maximum = (int)(total_h);
    vScrollbar->SmallChange = ClientSize.Height / 20;
    vScrollbar->LargeChange = (int)(h);

    hScrollbar->Visible = (total_w > w);
    hScrollbar->Maximum = (int)(total_w);
    hScrollbar->SmallChange = ClientSize.Width / 20;
    hScrollbar->LargeChange = (int)(w);
  }
}

void WindowsCanvasViewerPanel::HandleScroll(Object ^ sender, ScrollEventArgs ^ args) {
  scrolling = true;
  canvas->scroll_to(hScrollbar->Value, vScrollbar->Value);
  UpdateScrollBarPositions();
  scrolling = false;
  Update();
}

void WindowsCanvasViewerPanel::DoMouseMove(MouseEventArgs ^ e) {
  if (canvasInitialized)
    canvas->OnMouseMove(e, ModifierKeys, MouseButtons);
}