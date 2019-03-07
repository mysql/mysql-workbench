/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_scrollpanel.h"

/**
 * Implementation of a specialized mforms panel, which has scrollbars.
 * On Windows every container has scrollbars, so we don't need a separate implementation here.
 * Instead we use the same implementation as in the panel wrapper.
 */

using namespace System::Windows::Forms;
using namespace System::Windows::Forms::Layout;

using namespace MySQL;
using namespace MySQL::Forms;

ref class ScrollFillLayout : public LayoutEngine {
public:
  virtual bool Layout(Object ^ sender, LayoutEventArgs ^ args) override;
};

ref class WheelMessageFilter : System::Windows::Forms::IMessageFilter {
private:
  ScrollFillPanel ^ panel;

public:
  WheelMessageFilter(ScrollFillPanel ^ panel);

  virtual bool PreFilterMessage(Message % m);
};

//----------------- ScrollFillPanel ----------------------------------------------------------------

public
ref class MySQL::Forms::ScrollFillPanel : public Control {
private:
  System::Windows::Forms::IMessageFilter ^ wheelMessageFilter;
  ScrollFillLayout ^ layoutEngine;
  bool autoHideScrollbars;
  bool hideHorizontalScrollbar;
  bool hideVerticalScrollbar;
  int horizontalOffset;
  int verticalOffset;
  int updateCount;

public:
  ScrollFillPanel() {
    autoHideScrollbars = true;
    hideHorizontalScrollbar = false;
    hideVerticalScrollbar = false;
    horizontalOffset = 0;
    verticalOffset = 0;
    updateCount = 0;

    // In addition to the normal scroll wheel handling we add a message filter for scroll wheel
    // messages to react also to them if the mouse is within our client area but we are not focused.
    wheelMessageFilter = gcnew WheelMessageFilter(this);
    Application::AddMessageFilter(wheelMessageFilter);
  }

  //------------------------------------------------------------------------------------------------

  ScrollFillPanel::~ScrollFillPanel() {
    Application::RemoveMessageFilter(wheelMessageFilter);
  }

  //------------------------------------------------------------------------------------------------

  void HandleHorizontalScrolling(Message % m) {
    switch (m.WParam.ToInt32() & 0xFFFF) // Low word only.
    {
      case SB_RIGHT: {
        System::Drawing::Size maxSize(0, 0);
        if (Controls->Count > 0) {
          Control ^ content = Controls[0];
          maxSize = content->Size;
        }
        SetOffset(-maxSize.Width, verticalOffset);
        break;
      }

      case SB_ENDSCROLL:
        break;

      case SB_LINELEFT:
        SetOffset(horizontalOffset + 8, verticalOffset);
        break;

      case SB_LINERIGHT:
        SetOffset(horizontalOffset - 8, verticalOffset);
        break;

      case SB_PAGELEFT:
        SetOffset(horizontalOffset + ClientSize.Width, verticalOffset);
        break;

      case SB_PAGERIGHT:
        SetOffset(horizontalOffset - ClientSize.Height, verticalOffset);
        break;

      case SB_THUMBPOSITION:
      case SB_THUMBTRACK: {
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_TRACKPOS;
        GetScrollInfo((HWND)Handle.ToPointer(), SB_HORZ, &si);
        SetOffset(-si.nTrackPos, verticalOffset);
        break;
      }

      case SB_TOP:
        SetOffset(0, verticalOffset);
        break;
    }

    m.Result = IntPtr::Zero;
  }

  //------------------------------------------------------------------------------------------------

  void HandleVerticalScrolling(Message % m) {
    switch (m.WParam.ToInt32() & 0xFFFF) // Low word only.
    {
      case SB_BOTTOM: {
        System::Drawing::Size maxSize(0, 0);
        if (Controls->Count > 0) {
          Control ^ content = Controls[0];
          maxSize = content->Size;
        }
        SetOffset(horizontalOffset, -maxSize.Height);
        break;
      }

      case SB_ENDSCROLL:
        break;

      case SB_LINEUP:
        SetOffset(horizontalOffset, verticalOffset + 8);
        break;

      case SB_LINEDOWN:
        SetOffset(horizontalOffset, verticalOffset - 8);
        break;

      case SB_PAGEUP:
        SetOffset(horizontalOffset, verticalOffset + ClientSize.Height);
        break;

      case SB_PAGEDOWN:
        SetOffset(horizontalOffset, verticalOffset - ClientSize.Height);
        break;

      case SB_THUMBPOSITION:
      case SB_THUMBTRACK: {
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_TRACKPOS;
        GetScrollInfo((HWND)Handle.ToPointer(), SB_VERT, &si);
        SetOffset(horizontalOffset, -si.nTrackPos);
        break;
      }

      case SB_TOP:
        SetOffset(horizontalOffset, 0);
        break;
    }

    m.Result = IntPtr::Zero;
  }

  //------------------------------------------------------------------------------------------------

  void HandleWheelScrolling(Message % m) {
    System::Drawing::Size maxSize(0, 0);
    if (Controls->Count > 0) {
      Control ^ content = Controls[0];
      maxSize = content->Size;
    }
    int scrollAmount;
    double wheelFactor = GET_WHEEL_DELTA_WPARAM(m.WParam.ToInt64()) / WHEEL_DELTA;
    DWORD scrollLines;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
    if (maxSize.Height > ClientSize.Height) {
      if (scrollLines == WHEEL_PAGESCROLL)
        scrollAmount = (int)(wheelFactor * ClientSize.Height);
      else
        scrollAmount = (int)(wheelFactor * scrollLines * 16); // TODO: find a better way to define a line.
      SetOffset(horizontalOffset, verticalOffset + scrollAmount);
    } else if (maxSize.Width > ClientSize.Width) {
      scrollAmount = (int)(wheelFactor * scrollLines);
      SetOffset(horizontalOffset + scrollAmount, verticalOffset);
    }

    m.Result = IntPtr::Zero;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Scrolls the control's content to the given position.
   */
  void ScrollFillPanel::SetOffset(int x, int y) {
    System::Drawing::Size maxSize(0, 0);
    if (Controls->Count > 0) {
      Control ^ content = Controls[0];
      maxSize = content->Size;
    }

    // Sanity checks.
    if (x < (ClientSize.Width - maxSize.Width))
      x = ClientSize.Width - maxSize.Width;
    if (x > 0)
      x = 0;
    int deltaX = x - horizontalOffset;
    if (y < (ClientSize.Height - maxSize.Height))
      y = ClientSize.Height - maxSize.Height;
    if (y > 0)
      y = 0;
    int deltaY = y - verticalOffset;

    horizontalOffset = x;
    verticalOffset = y;

    if (deltaX != 0 || deltaY != 0)
      ScrollWindow((HWND)Handle.ToPointer(), deltaX, deltaY, NULL, NULL);

    if (updateCount == 0) {
      UpdateHorizontalScrollbar(-1);
      UpdateVerticalScrollbar(-1);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual System::Drawing::Size GetPreferredSize(System::Drawing::Size proposedSize) override {
    return proposedSize;
  }

  //------------------------------------------------------------------------------------------------

  virtual void WndProc(Message % m) override {
    switch (m.Msg) {
      case WM_HSCROLL:
        HandleHorizontalScrolling(m);
        return;
      case WM_VSCROLL:
        HandleVerticalScrolling(m);
        return;
      case WM_MOUSEWHEEL:
        HandleWheelScrolling(m);
        return;
      case WM_SIZE:
        if (updateCount == 0) {
          updateCount++;
          UpdateVerticalScrollbar(-1);
          UpdateHorizontalScrollbar(-1);
          updateCount--;
        }
        break;
    }
    Control::WndProc(m);
  }

  //------------------------------------------------------------------------------------------------

  void UpdateHorizontalScrollbar(int newWidth) {
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;

    updateCount++;

    System::Drawing::Size contentSize(newWidth, 0);
    if (newWidth < 0 && Controls->Count > 0)
      contentSize = Controls[0]->Size;

    bool needScrollbar = contentSize.Width > ClientSize.Width;
    if (!needScrollbar)
      contentSize.Width = ClientSize.Width;
    if (!hideHorizontalScrollbar) {
      if (needScrollbar) {
        si.nMax = contentSize.Width;
        si.nPage = ClientSize.Width + 1;
        si.nPos = -horizontalOffset;
      } else
        // No scrollbar needed. Hide it if auto hiding is enabled, otherwise disable it.
        if (!autoHideScrollbars)
        si.fMask |= SIF_DISABLENOSCROLL;
    }
    SetScrollInfo((HWND)Handle.ToPointer(), SB_HORZ, &si, true);

    SetOffset(horizontalOffset, verticalOffset);

    updateCount--;
  }

  //------------------------------------------------------------------------------------------------

  void UpdateVerticalScrollbar(int newHeight) {
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    int newVerticalOffset = verticalOffset;

    updateCount++;

    System::Drawing::Size contentSize(0, newHeight);
    if (newHeight < 0 && Controls->Count > 0)
      contentSize = Controls[0]->Size;

    bool needScrollbar = contentSize.Height > ClientSize.Height;
    if (!needScrollbar) {
      contentSize.Height = ClientSize.Height;
      newVerticalOffset = 0;
    }

    if (!hideVerticalScrollbar) {
      if (needScrollbar) {
        si.nMax = contentSize.Height;
        si.nPage = ClientSize.Height + 1;
        si.nPos = -verticalOffset;
      } else
        // No scrollbar needed. Hide it if auto hiding is enabled, otherwise disable it.
        if (!autoHideScrollbars)
        si.fMask |= SIF_DISABLENOSCROLL;
    }
    SetScrollInfo((HWND)Handle.ToPointer(), SB_VERT, &si, true);

    SetOffset(horizontalOffset, newVerticalOffset);

    updateCount--;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Scrolls the box so that the given control is in the visual portion of the box.
   * Note: For this to work we don't consider the (only) child of this box (as it is always in full size)
   *       but its children, which is the real content of the box (nested, due to the way mforms works).
   */
  void ScrollControlIntoView(Control ^ control) {
    Control ^ parent = (Controls->Count > 0) ? Controls[0] : nullptr;
    if (parent != nullptr && parent->Contains(control)) {
      Drawing::Point position = parent->PointToClient(control->PointToScreen(control->Location));
      int left = position.X + horizontalOffset; // Relative position to the view port.
      int top = position.Y + verticalOffset;    // Ditto.

      int newHorizontalOffset = horizontalOffset;
      int newVerticalOffset = verticalOffset;

      if (left < 0)
        newHorizontalOffset -= left;
      if (top < 0)
        newVerticalOffset -= top;

      int right = left + control->Bounds.Width;
      int bottom = top + control->Bounds.Height;
      if (right > Width)
        newHorizontalOffset += Width - right;
      if (bottom > Height)
        newVerticalOffset += Height - bottom;

      SetOffset(newHorizontalOffset, newVerticalOffset);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual property System::Windows::Forms::Layout::LayoutEngine ^
    LayoutEngine {
      System::Windows::Forms::Layout::LayoutEngine ^ get() override {
        if (layoutEngine == nullptr)
          layoutEngine = gcnew ScrollFillLayout();

        return layoutEngine;
      }
    }

    //------------------------------------------------------------------------------------------------

    /**
     * Used to either hide scrollbars completely if they are not needed or show them as disabled.
     */
    property bool AutoHideScrollbars {
    bool get() {
      return autoHideScrollbars;
    }
    void set(bool value) {
      if (autoHideScrollbars != value) {
        autoHideScrollbars = value;
        PerformLayout(this, "Padding");
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  property bool HideHorizontalScrollbar {
    bool get() {
      return hideHorizontalScrollbar;
    }
    void set(bool value) {
      if (hideHorizontalScrollbar != value) {
        hideHorizontalScrollbar = value;
        PerformLayout(this, "Padding");
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  property bool HideVerticalScrollbar {
    bool get() {
      return hideVerticalScrollbar;
    }
    void set(bool value) {
      if (hideVerticalScrollbar != value) {
        hideVerticalScrollbar = value;
        PerformLayout(this, "Padding");
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  property int HorizontalOffset {
    int get() {
      return horizontalOffset;
    };
  }

  //------------------------------------------------------------------------------------------------

  property int VerticalOffset {
    int get() {
      return verticalOffset;
    };
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- WheelMessageFilter -------------------------------------------------------------

WheelMessageFilter::WheelMessageFilter(ScrollFillPanel ^ panel) {
  this->panel = panel;
}

//--------------------------------------------------------------------------------------------------

bool WheelMessageFilter::PreFilterMessage(Message % m) {
  if (panel != nullptr && panel->Visible && m.Msg == WM_MOUSEWHEEL) {
    // Only handle messages if the mouse without our bounds.
    POINT cursorPoint;
    GetCursorPos(&cursorPoint);
    Drawing::Point point = panel->PointToClient(Drawing::Point(cursorPoint.x, cursorPoint.y));
    if (panel->ClientRectangle.Contains(point)) {
      panel->WndProc(m);
      return false;
    }
  }

  return false;
}

//----------------- ScrollFillLayout ---------------------------------------------------------------

bool ScrollFillLayout::Layout(Object ^ sender, LayoutEventArgs ^ args) {
  // This layout is actually very simple. Simply resize the first (and only) child control so that
  // it fills the entire client area of the container.
  // However, and that is different to just dock it with DockStyle::Fill, don't make it smaller
  // than it wants to be, i.e. as determined through its minimum size and layout size.

  ScrollFillPanel ^ container = (ScrollFillPanel ^)sender;
  if (container->Controls->Count > 0) {
    ViewWrapper::adjust_auto_resize_from_docking(container);
    Drawing::Size boxSize = container->Size;

    Control ^ content = container->Controls[0];
    Drawing::Size childSize;

    childSize = content->GetPreferredSize(container->ClientSize);
    if (ViewWrapper::use_min_width_for_layout(content))
      childSize.Width = content->MinimumSize.Width;
    if (ViewWrapper::use_min_height_for_layout(content))
      childSize.Height = content->MinimumSize.Height;

    // Stretch client to fill the entire height, if it isn't larger already.
    if (childSize.Height < container->ClientSize.Height)
      childSize.Height = container->ClientSize.Height;
    container->UpdateVerticalScrollbar(childSize.Height);

    if (childSize.Width < container->ClientSize.Width)
      childSize.Width = container->ClientSize.Width;
    container->UpdateHorizontalScrollbar(childSize.Width);

    ViewWrapper::remove_auto_resize(content, AutoResizeMode::ResizeBoth);
    content->Size = childSize;

    return false;
  }
  return false;
}

//----------------- ScrollPanelWrapper ----------------------------------------------------------------

ScrollPanelWrapper::ScrollPanelWrapper(mforms::ScrollPanel *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool ScrollPanelWrapper::create(mforms::ScrollPanel *backend, mforms::ScrollPanelFlags flags) {
  ScrollPanelWrapper *wrapper = new ScrollPanelWrapper(backend);
  if ((flags & mforms::ScrollPanelBordered) != 0) {
    // Have to fake a bordered scrollbox by embedding a panel in a groupbox.
    GroupBox ^ box = ScrollPanelWrapper::Create<GroupBox>(backend, wrapper);
    box->AutoSize = false;
    box->Padding = Padding(5);
    wrapper->container = gcnew ScrollFillPanel();
    box->Controls->Add(wrapper->container);
    wrapper->container->Dock = DockStyle::Fill;
  } else
    wrapper->container = ScrollPanelWrapper::Create<ScrollFillPanel>(backend, wrapper);
  wrapper->container->AutoSize = false;
  wrapper->container->Margin = Padding(0);
  wrapper->container->Padding = Padding(0);

  return true;
}

//--------------------------------------------------------------------------------------------------

void ScrollPanelWrapper::add(mforms::ScrollPanel *backend, mforms::View *view) {
  ScrollPanelWrapper *wrapper = backend->get_data<ScrollPanelWrapper>();
  Control ^ child = ScrollPanelWrapper::GetControl(view);
  wrapper->container->Controls->Add(child);
  child->Location = Drawing::Point(0, 0);
  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void ScrollPanelWrapper::scroll_to_view(mforms::ScrollPanel *backend, mforms::View *view) {
  ScrollPanelWrapper *wrapper = backend->get_data<ScrollPanelWrapper>();
  Control ^ child = ScrollPanelWrapper::GetControl(view);
  wrapper->container->ScrollControlIntoView(child);
}

//--------------------------------------------------------------------------------------------------

void ScrollPanelWrapper::remove(mforms::ScrollPanel *backend) {
  ScrollPanelWrapper *wrapper = backend->get_data<ScrollPanelWrapper>();
  wrapper->container->Controls->Clear();
  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void ScrollPanelWrapper::set_autohide_scrollers(mforms::ScrollPanel *backend, bool flag) {
  ScrollPanelWrapper *wrapper = backend->get_data<ScrollPanelWrapper>();
  wrapper->container->AutoHideScrollbars = flag;
}

//--------------------------------------------------------------------------------------------------

void ScrollPanelWrapper::set_visible_scrollers(mforms::ScrollPanel *backend, bool vertical, bool horizontal) {
  ScrollPanelWrapper *wrapper = backend->get_data<ScrollPanelWrapper>();
  wrapper->container->HideHorizontalScrollbar = !horizontal;
  wrapper->container->HideVerticalScrollbar = !vertical;
  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

base::Rect ScrollPanelWrapper::get_content_rect(mforms::ScrollPanel *backend) {
  ScrollPanelWrapper *wrapper = backend->get_data<ScrollPanelWrapper>();
  Drawing::Rectangle rect = wrapper->container->ClientRectangle;

  return base::Rect(rect.Left, rect.Top, rect.Width, rect.Height);
}

//--------------------------------------------------------------------------------------------------

void ScrollPanelWrapper::scroll_to(mforms::ScrollPanel *backend, int x, int y) {
  ScrollPanelWrapper *wrapper = backend->get_data<ScrollPanelWrapper>();

  // The backend works with positive offsets while we need negative ones (which is correct
  // from a coordinate system view point as the offset defines a translation in the view space).
  wrapper->container->SetOffset(-x, -y);
}

//--------------------------------------------------------------------------------------------------

void ScrollPanelWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_spanel_impl.create = &ScrollPanelWrapper::create;
  f->_spanel_impl.add = &ScrollPanelWrapper::add;
  f->_spanel_impl.remove = &ScrollPanelWrapper::remove;
  f->_spanel_impl.set_visible_scrollers = &ScrollPanelWrapper::set_visible_scrollers;
  f->_spanel_impl.set_autohide_scrollers = &ScrollPanelWrapper::set_autohide_scrollers;
  f->_spanel_impl.scroll_to_view = &ScrollPanelWrapper::scroll_to_view;
  f->_spanel_impl.get_content_rect = &ScrollPanelWrapper::get_content_rect;
  f->_spanel_impl.scroll_to = &ScrollPanelWrapper::scroll_to;
}

//--------------------------------------------------------------------------------------------------
