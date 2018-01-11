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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_panel.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Controls;

//----------------- FillLayout ---------------------------------------------------------------------

System::Drawing::Size FillLayout::ComputeLayout(Control ^ control, System::Drawing::Size proposedSize,
                                                bool resizeChildren) {
  if (control->Controls->Count > 0) {
    // Exclude any space needed to draw decoration (e.g. border) from layout processing.
    System::Drawing::Rectangle inner = control->DisplayRectangle;
    System::Drawing::Size current_size = control->Size;
    int horizontal_padding = current_size.Width - inner.Width;
    int vertical_padding = current_size.Height - inner.Height;

    Control ^ content = control->Controls[0];
    proposedSize.Width -= horizontal_padding;
    proposedSize.Height -= vertical_padding;

    ViewWrapper::set_full_auto_resize(content);
    System::Drawing::Size contentSize = content->GetPreferredSize(proposedSize);

    if (ViewWrapper::use_min_width_for_layout(content))
      contentSize.Width = content->MinimumSize.Width;
    if (ViewWrapper::use_min_height_for_layout(content))
      contentSize.Height = content->MinimumSize.Height;

    // Adjust width of the container if it is too small or auto resizing is enabled.
    if (proposedSize.Width < contentSize.Width || ViewWrapper::can_auto_resize_horizontally(control)) {
      proposedSize.Width = contentSize.Width;
      if (proposedSize.Width < control->MinimumSize.Width - control->Padding.Horizontal)
        proposedSize.Width = control->MinimumSize.Width - control->Padding.Horizontal;
    }

    // Adjust height of the container if it is too small or auto resizing is enabled.
    if (proposedSize.Height < contentSize.Height || ViewWrapper::can_auto_resize_vertically(control)) {
      proposedSize.Height = contentSize.Height;
      if (proposedSize.Height < control->MinimumSize.Height - control->Padding.Vertical)
        proposedSize.Height = control->MinimumSize.Height - control->Padding.Vertical;
    }

    if (resizeChildren) {
      // Now stretch the client control to fill the entire display area.
      ViewWrapper::remove_auto_resize(content, AutoResizeMode::ResizeBoth);

      ValueSetter ^ valueSetter = dynamic_cast<ValueSetter ^>(control);
      System::Drawing::Rectangle newBounds = System::Drawing::Rectangle(inner.Location, proposedSize);

      // Windows has a nesting depth limitation, which is for 64 bit Windows versions quite low
      // (like 16 or so). Exceeding this depth causes internal trouble with the result that
      // not all the messages a window usually gets are actually sent. This in turn will break our
      // layouting so we have to ensure re-layouting does not exceed this max depth by breaking up
      // the child control bounds setting. This is only needed at every 16th level but since it is hard
      // to control that reliably we do it in panels, which do not appear too often but are almost always
      // part of such control hierarchies that have a deep nesting level.
      // If you are in need to break up a deeply nested hierarchy simply insert a panel where appropriate.
      if (!control->IsHandleCreated)
        content->Bounds = newBounds;
      else {
        // The value for the content can be asynchronously only if a handle is created on the parent panel.
        ApplyBoundsDelegate ^ applyBoundsDelegate =
          gcnew ApplyBoundsDelegate(valueSetter, &ValueSetter::ApplyContentBounds);
        control->BeginInvoke(applyBoundsDelegate, gcnew array<Object ^>{newBounds});
      }
    }

    proposedSize.Width += horizontal_padding;
    proposedSize.Height += vertical_padding;
  }

  return proposedSize;
}

//-------------------------------------------------------------------------------------------------

bool FillLayout::Layout(Object ^ container, LayoutEventArgs ^ arguments) {
  Control ^ control = (Control ^)container;
  if (!ViewWrapper::can_layout(control, arguments->AffectedProperty))
    return false;

  ViewWrapper::adjust_auto_resize_from_docking(control);
  System::Drawing::Size newSize = ComputeLayout(control, control->Size, true);

  if (newSize.Width < control->MinimumSize.Width)
    newSize.Width = control->MinimumSize.Width;
  if (newSize.Height < control->MinimumSize.Height)
    newSize.Height = control->MinimumSize.Height;

  // Finally adjust the container.
  bool parentLayoutNeeded = !control->Size.Equals(newSize);
  if (parentLayoutNeeded)
    ViewWrapper::resize_with_docking(control, newSize);

  return parentLayoutNeeded;
}

//-------------------------------------------------------------------------------------------------

System::Drawing::Size FillLayout::GetPreferredSize(Control ^ control, System::Drawing::Size proposedSize) {
  return ComputeLayout(control, proposedSize, false);
}

//----------------- FillGroupBox ------------------------------------------------------------------

FillGroupBox::FillGroupBox() {
}

//--------------------------------------------------------------------------------------------------

System::Drawing::Size FillGroupBox::GetPreferredSize(System::Drawing::Size proposedSize) {
  return layoutEngine->GetPreferredSize(this, proposedSize);
}

//--------------------------------------------------------------------------------------------------

void FillGroupBox::OnPaintBackground(PaintEventArgs ^ args) {
  GroupBox::OnPaintBackground(args);

  ViewWrapper *view = PanelWrapper::GetWrapper<ViewWrapper>(this);
  view->DrawBackground(args);
}

//----------------- FillPanel ---------------------------------------------------------------------

FillPanel::FillPanel() {
}

//--------------------------------------------------------------------------------------------------

System::Drawing::Size FillPanel::GetPreferredSize(System::Drawing::Size proposedSize) {
  return layoutEngine->GetPreferredSize(this, proposedSize);
}

//--------------------------------------------------------------------------------------------------

void FillPanel::OnPaintBackground(PaintEventArgs ^ args) {
  Panel::OnPaintBackground(args);

  ViewWrapper *view = PanelWrapper::GetWrapper<ViewWrapper>(this);
  view->DrawBackground(args);
}

//----------------- HeaderFillPanel ----------------------------------------------------------------

HeaderFillPanel::HeaderFillPanel() {
}

//--------------------------------------------------------------------------------------------------

System::Drawing::Size HeaderFillPanel::GetPreferredSize(System::Drawing::Size proposedSize) {
  return layoutEngine->GetPreferredSize(this, proposedSize);
}

//----------------- PanelWrapper ---------------------------------------------------------------------

PanelWrapper::PanelWrapper(mforms::View *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool PanelWrapper::create(mforms::Panel *backend, mforms::PanelType panelType) {
  PanelWrapper *wrapper = new PanelWrapper(backend);
  wrapper->type = panelType;

  Control ^ control;
  switch (panelType) {
    case mforms::TransparentPanel: // just a container with no background
    case mforms::FilledPanel:      // just a container with color filled background
      // The background color can be specified separately and does not determine the type of panel we create.
      control = PanelWrapper::Create<FillPanel>(backend, wrapper);
      break;

    case mforms::TitledBoxPanel:   // native grouping box with a title with border
    case mforms::BorderedPanel:    // container with native border
    case mforms::TitledGroupPanel: // native grouping container with a title (may have no border)
      // The title can be set separately and does not determine the type of the group box we create.
      // A control with title but no border is not supported on Windows, so we need to create a special composite.
      // TODO: implement box with title, but no border.
      control = PanelWrapper::Create<FillGroupBox>(backend, wrapper);
      control->Padding = Padding(7, 0, 7, 8);
      break;

    case mforms::LineBorderPanel: // container with a solid line border
    {
      FillPanel ^ native_panel = PanelWrapper::Create<FillPanel>(backend, wrapper);
      control = native_panel;
      native_panel->BorderStyle = BorderStyle::FixedSingle;
      break;
    }

    case mforms::FilledHeaderPanel:
    case mforms::StyledHeaderPanel: // Panel with header
    {
      HeaderFillPanel ^ native_panel = PanelWrapper::Create<HeaderFillPanel>(backend, wrapper);
      control = native_panel;
      control->Padding = Padding(0, 21, 0, 0);
      control->BackColor = Color::FromArgb(40, 55, 82);

      native_panel->HeaderPadding = Padding(5, 0, 5, 0);
      native_panel->HeaderColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelHeader, false);
      native_panel->ForeColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelHeader, true);
      native_panel->HeaderColorFocused =
        Conversions::GetApplicationColor(ApplicationColor::AppColorPanelHeaderFocused, false);
      native_panel->ForeColorFocused =
        Conversions::GetApplicationColor(ApplicationColor::AppColorPanelHeaderFocused, true);
      break;
    }

    default:
      throw std::logic_error("Internal error: unhandled mforms panel type.");
  }

  control->AutoSize = false;
  return true;
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::set_title(mforms::Panel *backend, const std::string &title) {
  PanelWrapper *wrapper = backend->get_data<PanelWrapper>();
  wrapper->set_title(title);
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::set_back_color(mforms::Panel *backend, const std::string &color) {
  PanelWrapper *wrapper = backend->get_data<PanelWrapper>();
  wrapper->set_back_color(color);
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::add(mforms::Panel *backend, mforms::View *view) {
  PanelWrapper *wrapper = backend->get_data<PanelWrapper>();
  wrapper->add(view);
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::set_active(mforms::Panel *backend, bool value) {
  PanelWrapper *wrapper = backend->get_data<PanelWrapper>();
  wrapper->set_active(value);
}

//-------------------------------------------------------------------------------------------------

bool PanelWrapper::get_active(mforms::Panel *backend) {
  PanelWrapper *wrapper = backend->get_data<PanelWrapper>();
  return wrapper->get_active();
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::remove(mforms::Panel *backend, mforms::View *view) {
  PanelWrapper *wrapper = backend->get_data<PanelWrapper>();
  wrapper->remove(view);
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::set_title(const std::string &title) {
  Control ^ control = GetControl();
  control->Text = CppStringToNative(title);
  control->Refresh();
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::set_back_color(const std::string &color) {
  switch (type) {
    case mforms::TitledBoxPanel:
    case mforms::BorderedPanel:
    case mforms::TitledGroupPanel:
      GetManagedObject<GroupBox>()->BackColor = System::Drawing::ColorTranslator::FromHtml(CppStringToNativeRaw(color));
      break;
    default:
      GetManagedObject<Panel>()->BackColor = System::Drawing::ColorTranslator::FromHtml(CppStringToNativeRaw(color));
  }
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::add(mforms::View *view) {
  child = view;
  GetControl()->Controls->Add(PanelWrapper::GetControl(view));
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::set_active(bool value) {
  // TODO: implement as soon as the checkbox is available
}

//-------------------------------------------------------------------------------------------------

bool PanelWrapper::get_active() {
  // TODO: implement as soon as the checkbox is available
  return false;
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::remove(mforms::View *view) {
  if (child == view)
    child = NULL;
  GetControl()->Controls->Remove(PanelWrapper::GetControl(view));
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::remove() {
  GetControl()->Controls->Clear();
}

//-------------------------------------------------------------------------------------------------

void PanelWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_panel_impl.create = &PanelWrapper::create;
  f->_panel_impl.set_title = &PanelWrapper::set_title;
  f->_panel_impl.add = &PanelWrapper::add;
  f->_panel_impl.set_back_color = &PanelWrapper::set_back_color;
  f->_panel_impl.set_active = &PanelWrapper::set_active;
  f->_panel_impl.get_active = &PanelWrapper::get_active;
  f->_panel_impl.remove = &PanelWrapper::remove;
}

//-------------------------------------------------------------------------------------------------
