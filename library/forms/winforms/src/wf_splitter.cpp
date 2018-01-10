/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_splitter.h"

using namespace MySQL::Forms;
using namespace System::Windows::Forms;

//----------------- MformsSplitContainer -----------------------------------------------------------

ref class MformsSplitContainer : public SplitContainer {
private:
  int pendingSplitterDistance;

public:
  mforms::Splitter *backend;

  MformsSplitContainer() {
    pendingSplitterDistance = -1;

    BackColor = Drawing::Color::Transparent;
    Size = Drawing::Size(100, 100);
    TabStop = false;
    SplitterWidth = 6;
    SplitterDistance = 50;

    // OnSplitterMoved is not virtual (for whatever reason), so we have to register an event
    // instead simply overriding it.
    SplitterMoved += gcnew SplitterEventHandler(this, &MformsSplitContainer::DoSplitterMoved);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnResize(EventArgs ^ args) override {
    __super ::OnResize(args);

    if (pendingSplitterDistance > -1) {
      SafeAssignSplitterDistance(pendingSplitterDistance);
      pendingSplitterDistance = -1;
    }
  }

  //------------------------------------------------------------------------------------------------

  void DoSplitterMoved(Object ^ sender, SplitterEventArgs ^ args) {
    backend->position_changed();
  }

  //--------------------------------------------------------------------------------------------------

  void SafeAssignSplitterDistance(int distance) {
    // If the splitter is within any of the min size areas of the two panels set it to the center
    // between both. If that still fails it can only mean the container is smaller than the sum of
    // the min sizes. In that case don't assign a splitter position at all.
    int size = Orientation == System::Windows::Forms::Orientation::Horizontal ? Height : Width;
    if (distance < Panel1MinSize)
      distance = Panel1MinSize;
    if (distance > size - Panel2MinSize)
      distance = size - Panel2MinSize;

    try {
      if (distance > 0)
        SplitterDistance = distance;
    } catch (...) {
      // Ignore stupid splitter distance errors. The SplitContainer really should adjust values
      // outside the valid range to a meaningful position.
    }
  }

  //------------------------------------------------------------------------------------------------

  void SafeAssignMinSize(int min_size, bool first) {
    // Similar as for the splitter distance the split container will throw an exception if the
    // min sizes don't match the size.
    int size = (Orientation == System::Windows::Forms::Orientation::Horizontal) ? Height : Width;
    if (first) {
      if (size - Panel2MinSize - SplitterWidth <= min_size) {
        int new_size = Panel2MinSize + SplitterWidth + min_size;
        if (Orientation == System::Windows::Forms::Orientation::Horizontal)
          Height = new_size;
        else
          Width = new_size;
      }

      try {
        Panel1MinSize = min_size;
      } catch (...) {
      }
    } else {
      if (size - Panel1MinSize - SplitterWidth <= min_size) {
        int new_size = Panel1MinSize + SplitterWidth + min_size;
        if (Orientation == System::Windows::Forms::Orientation::Horizontal)
          Height = new_size;
        else
          Width = new_size;
      }

      try {
        Panel2MinSize = min_size;
      } catch (...) {
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  void SetPosition(int position) {
    if (IsHandleCreated && Visible)
      SafeAssignSplitterDistance(position);
    else
      pendingSplitterDistance = position;
  }

  //------------------------------------------------------------------------------------------------

  int GetPosition() {
    if (pendingSplitterDistance == -1)
      return SplitterDistance;
    else
      return pendingSplitterDistance;
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- SplitterWrapper ----------------------------------------------------------------

SplitterWrapper::SplitterWrapper(mforms::Splitter *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool SplitterWrapper::create(mforms::Splitter *backend, bool horizontal) {
  SplitterWrapper *wrapper = new SplitterWrapper(backend);
  MformsSplitContainer ^ container = SplitterWrapper::Create<MformsSplitContainer>(backend, wrapper);
  container->backend = backend;

  // Note: orientation is that of the splitter, not the container layout.
  container->Orientation = horizontal ? Orientation::Vertical : Orientation::Horizontal;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Adds a new child window to the splitter. Behavior is as follows:
 * - If the left panel is empty add to this.
 * - If not and the right panel is empty add to this.
 * - If no panel is empty then we have an error.
 */
void SplitterWrapper::add(mforms::Splitter *backend, mforms::View *child, int min_size, bool fixed) {
  MformsSplitContainer ^ container = SplitterWrapper::GetManagedObject<MformsSplitContainer>(backend);
  ViewWrapper *view = child->get_data<ViewWrapper>();
  Control ^ control = view->GetControl();

  if (container->Panel1->Controls->Count == 0) {
    container->Panel1->Controls->Add(control);
    container->SafeAssignMinSize(min_size, true);
    if (fixed)
      container->FixedPanel = FixedPanel::Panel1;
    control->Dock = DockStyle::Fill;
    view->set_resize_mode(AutoResizeMode::ResizeBoth);
  } else if (container->Panel2->Controls->Count == 0) {
    container->Panel2->Controls->Add(control);
    container->SafeAssignMinSize(min_size, false);
    if (fixed)
      container->FixedPanel = FixedPanel::Panel2;
    control->Dock = DockStyle::Fill;
    view->set_resize_mode(AutoResizeMode::ResizeBoth);
  } else
    throw std::logic_error("mforms splitter error: adding more than 2 child controls is not allowed.");

  // Hide the panel that has no child control.
  container->Panel1Collapsed = container->Panel1->Controls->Count == 0;
  container->Panel2Collapsed = container->Panel2->Controls->Count == 0;
}

//--------------------------------------------------------------------------------------------------

void SplitterWrapper::remove(mforms::Splitter *backend, mforms::View *child) {
  MformsSplitContainer ^ container = SplitterWrapper::GetManagedObject<MformsSplitContainer>(backend);
  Control ^ control = SplitterWrapper::GetControl(child);

  // Since there can only be one child window on each panel we do a Clear() instead removing only the
  // individual control, fixing so possibly invalid setups (if they ever occur).
  // Additionally, collapse the panel that is now empty unless both panels are empty then.
  if (container->Panel1->Controls->Contains(control)) {
    container->Panel1->Controls->Clear();
    if (container->Panel2->Controls->Count > 0)
      container->Panel1Collapsed = true;
  } else if (container->Panel2->Controls->Contains(control)) {
    container->Panel2->Controls->Clear();
    if (container->Panel1->Controls->Count > 0)
      container->Panel2Collapsed = true;
  }
}

//--------------------------------------------------------------------------------------------------

void SplitterWrapper::set_divider_position(mforms::Splitter *backend, int position) {
  MformsSplitContainer ^ container = SplitterWrapper::GetManagedObject<MformsSplitContainer>(backend);
  container->SetPosition(position);
}

//--------------------------------------------------------------------------------------------------

int SplitterWrapper::get_divider_position(mforms::Splitter *backend) {
  MformsSplitContainer ^ container = SplitterWrapper::GetManagedObject<MformsSplitContainer>(backend);
  return container->GetPosition();
}

//--------------------------------------------------------------------------------------------------

void SplitterWrapper::set_expanded(mforms::Splitter *backend, bool first, bool expand) {
  MformsSplitContainer ^ container = SplitterWrapper::GetManagedObject<MformsSplitContainer>(backend);
  if (first)
    container->Panel1Collapsed = !expand;
  else
    container->Panel2Collapsed = !expand;
}

//--------------------------------------------------------------------------------------------------

void SplitterWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_splitter_impl.create = &SplitterWrapper::create;
  f->_splitter_impl.add = &SplitterWrapper::add;
  f->_splitter_impl.remove = &SplitterWrapper::remove;
  f->_splitter_impl.set_divider_position = &SplitterWrapper::set_divider_position;
  f->_splitter_impl.get_divider_position = &SplitterWrapper::get_divider_position;
  f->_splitter_impl.set_expanded = &SplitterWrapper::set_expanded;
}

//--------------------------------------------------------------------------------------------------
