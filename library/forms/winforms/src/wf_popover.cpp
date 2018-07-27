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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_popover.h"
#include "wf_utilities.h"

using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Windows::Forms;
using namespace System::Threading;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;
using namespace MySQL::Utilities::SysUtils;

//--------------------------------------------------------------------------------------------------

/**
 * Converts Windows specific mouse button identifiers to plain numbers for the back end.
 */
static mforms::MouseButton convert_mouse_button(MouseButtons button) {
  switch (button) {
    case MouseButtons::Left:
      return mforms::MouseButtonLeft;
    case MouseButtons::Right:
      return mforms::MouseButtonRight;
    case MouseButtons::Middle:
      return mforms::MouseButtonOther;
    default:
      return mforms::MouseButtonNone;
  }
}

//----------------- PopoverControl -----------------------------------------------------------------

ref class MySQL::Forms::PopoverControl : System::Windows::Forms::Form {
private:
  int cornerSize;
  int animationSteps;
  bool animated;
  Drawing::Point hotSpot;
  Drawing::Size baseSize;
  Bitmap ^ contentBitmap;
  GraphicsPath ^ outline;

  mforms::StartPosition relativePosition;
  mforms::PopoverStyle style;

protected:
  virtual void OnKeyPress(KeyPressEventArgs ^ args) override {
    if (args->KeyChar == 27) // Escape char
      Close();

    __super ::OnKeyPress(args);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnPaint(PaintEventArgs ^ args) override {
    // Draw our border. Child controls placed on the popover do their own drawing as usual.
    Pen ^ borderPen = gcnew Pen(Color::FromArgb(255, 170, 170, 170));
    borderPen->Width = 1.5;

    args->Graphics->SmoothingMode = SmoothingMode::HighQuality;
    args->Graphics->DrawPath(borderPen, outline);

    GraphicsPath ^ helper = (GraphicsPath ^)outline->Clone();
    Matrix ^ matrix = gcnew Matrix();
    matrix->Translate(-1, -1);
    matrix->Scale((Width + 2) / (float)Width, (Height + 1) / (float)Height);
    helper->Transform(matrix);
    args->Graphics->DrawPath(borderPen, helper);

    delete borderPen;
  }

  //------------------------------------------------------------------------------------------------

  void UpdateAndShowPopover(bool doAnimated) {
    ComputeOutline();
    Region = gcnew System::Drawing::Region(outline);

    // Scale down the outline a bit as lines are drawn not including right and bottom coordinates.
    Matrix ^ matrix = gcnew Matrix();
    matrix->Scale((Width - 1) / (float)Width, (Height - 0.5f) / (float)Height);
    outline->Transform(matrix);

    // Don't use animations in a terminal session (remote desktop).
    animated = doAnimated && !SystemInformation::TerminalServerSession;
    if (animated) {
      Opacity = 0;
      Visible = true;
      Update();
      for (int i = 1; i <= animationSteps; i++) {
        Opacity = i / (float)animationSteps;
        Sleep(200 / animationSteps); // 200ms for the entire fade.
      }
    } else
      Show();
  }

//------------------------------------------------------------------------------------------------

#define DEFAULT_PADDING 7 // Padding on all sides.

#define ARROW_SIZE 16 // Number of pixels from arrow base to arrow tip.
#define ARROW_BASE 32 // Number of pixels the base line of the arrow is wide.

  void ComputeCoordinatesAndPadding() {
    // The base size is the size of the main part, without arrow.
    System::Drawing::Size actualSize = baseSize;
    actualSize.Width += 2 * DEFAULT_PADDING;
    actualSize.Height += 2 * DEFAULT_PADDING;

    if (style == mforms::PopoverStyleNormal) {
      // Add the arrow size to either width or height, depending on the proposed relative position.
      if (relativePosition == mforms::StartLeft || relativePosition == mforms::StartRight)
        actualSize.Width += ARROW_SIZE;
      else
        actualSize.Height += ARROW_SIZE;
    }

    Size = actualSize;

    // The initial position of the arrow is not the center on its side but only 1/3 of side's size
    // for a more appealing look. Additionally, add the arrow's size to the padding on this size to
    // exclude its area from the main content area.
    Point newLocation = hotSpot;
    if (style == mforms::PopoverStyleNormal) {
      switch (relativePosition) {
        case mforms::StartLeft:
          newLocation.X = hotSpot.X - actualSize.Width;
          newLocation.Y = hotSpot.Y - actualSize.Height / 3;
          Padding = System::Windows::Forms::Padding(DEFAULT_PADDING, DEFAULT_PADDING, DEFAULT_PADDING + ARROW_SIZE, 7);
          break;
        case mforms::StartRight:
          newLocation.X = hotSpot.X;
          newLocation.Y = hotSpot.Y - actualSize.Height / 3;
          Padding = System::Windows::Forms::Padding(DEFAULT_PADDING + ARROW_SIZE, DEFAULT_PADDING, DEFAULT_PADDING,
                                                    DEFAULT_PADDING);
          break;
        case mforms::StartAbove:
          newLocation.X = hotSpot.X - actualSize.Width / 3;
          newLocation.Y = hotSpot.Y - actualSize.Height;
          Padding = System::Windows::Forms::Padding(DEFAULT_PADDING, DEFAULT_PADDING, DEFAULT_PADDING,
                                                    DEFAULT_PADDING + ARROW_SIZE);
          break;
        case mforms::StartBelow:
          newLocation.X = hotSpot.X - actualSize.Width / 3;
          newLocation.Y = hotSpot.Y;
          Padding = System::Windows::Forms::Padding(DEFAULT_PADDING, DEFAULT_PADDING + ARROW_SIZE, DEFAULT_PADDING,
                                                    DEFAULT_PADDING);
          break;
      }
    }

    Screen ^ currentScreen = Screen::FromHandle(IntPtr(GetForegroundWindow()));
    System::Drawing::Rectangle screenBounds = currentScreen->WorkingArea;

    // Check the control's bounds and determine the amount of pixels we have to move it make
    // it fully appear on screen. This will usually not move the hot spot, unless the movement
    // of the control is so much that it would leave the arrow outside its bounds.
    int deltaX = 0;
    int deltaY = 0;
    if (newLocation.X < screenBounds.Left)
      deltaX = screenBounds.Left - newLocation.X;
    if (newLocation.X + Width > screenBounds.Right)
      deltaX = screenBounds.Right - (newLocation.X + Width);

    if (newLocation.Y < screenBounds.Top)
      deltaY = screenBounds.Top - newLocation.Y;
    if (newLocation.Y + Height > screenBounds.Bottom)
      deltaY = screenBounds.Bottom - (newLocation.Y + Height);
    newLocation.X += deltaX;
    newLocation.Y += deltaY;

    // Now that we have the final location check the arrow again.
    switch (relativePosition) {
      case mforms::StartLeft:
      case mforms::StartRight:
        hotSpot.X += deltaX;
        if ((hotSpot.Y - ARROW_BASE / 2) < (newLocation.Y + cornerSize))
          hotSpot.Y = newLocation.Y + cornerSize + ARROW_BASE / 2;
        if ((hotSpot.Y + ARROW_BASE / 2) > (newLocation.Y + actualSize.Height - cornerSize))
          hotSpot.Y = newLocation.Y + actualSize.Height - cornerSize - ARROW_BASE / 2;
        break;
      case mforms::StartAbove:
      case mforms::StartBelow:
        if ((hotSpot.X - ARROW_BASE / 2) < (newLocation.X + cornerSize))
          hotSpot.X = newLocation.X + cornerSize + ARROW_BASE / 2;
        if ((hotSpot.X + ARROW_BASE / 2) > (newLocation.X + actualSize.Width - cornerSize))
          hotSpot.X = newLocation.X + actualSize.Width - cornerSize - ARROW_BASE / 2;
        hotSpot.Y += deltaY;
        break;
    }

    Location = newLocation;
  }

  //------------------------------------------------------------------------------------------------

  void ComputeOutline() {
    // Generate the outline of the actual content area.
    outline = gcnew GraphicsPath();

    System::Drawing::Rectangle bounds = System::Drawing::Rectangle(0, 0, Width, Height);
    System::Drawing::Point localHotSpot = PointToClient(hotSpot);

    switch (style) {
      case mforms::PopoverStyleTooltip: {
        outline->AddRectangle(bounds);

        break;
      }

      default:
        switch (relativePosition) {
          case mforms::StartLeft: {
            outline->AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, 180, 90);
            outline->AddArc(bounds.Right - cornerSize - ARROW_SIZE, bounds.Top, cornerSize, cornerSize, -90, 90);

            // Arrow.
            outline->AddLine(bounds.Right - ARROW_SIZE, bounds.Top + cornerSize, bounds.Right - ARROW_SIZE,
                             localHotSpot.Y - ARROW_BASE / 2);
            outline->AddLine(bounds.Right - ARROW_SIZE, localHotSpot.Y - ARROW_BASE / 2, bounds.Right, localHotSpot.Y);
            outline->AddLine(bounds.Right, localHotSpot.Y, bounds.Right - ARROW_SIZE, localHotSpot.Y + ARROW_BASE / 2);
            outline->AddLine(bounds.Right - ARROW_SIZE, localHotSpot.Y + ARROW_BASE / 2, bounds.Right - ARROW_SIZE,
                             bounds.Bottom - cornerSize);

            outline->AddArc(bounds.Right - cornerSize - ARROW_SIZE, bounds.Bottom - cornerSize, cornerSize, cornerSize,
                            0, 90);
            outline->AddArc(bounds.Left, bounds.Bottom - cornerSize, cornerSize, cornerSize, 90, 90);
            break;
          }
          case mforms::StartRight: {
            outline->AddArc(bounds.Left + ARROW_SIZE, bounds.Top, cornerSize, cornerSize, 180, 90);
            outline->AddArc(bounds.Right - cornerSize, bounds.Top, cornerSize, cornerSize, -90, 90);
            outline->AddArc(bounds.Right - cornerSize, bounds.Bottom - cornerSize, cornerSize, cornerSize, 0, 90);
            outline->AddArc(bounds.Left + ARROW_SIZE, bounds.Bottom - cornerSize, cornerSize, cornerSize, 90, 90);

            // Arrow.
            outline->AddLine(bounds.Left + ARROW_SIZE, bounds.Bottom - cornerSize, bounds.Left + ARROW_SIZE,
                             localHotSpot.Y + ARROW_BASE / 2);
            outline->AddLine(bounds.Left + ARROW_SIZE, localHotSpot.Y + ARROW_BASE / 2, bounds.Left, localHotSpot.Y);
            outline->AddLine(bounds.Left, localHotSpot.Y, bounds.Left + ARROW_SIZE, localHotSpot.Y - ARROW_BASE / 2);
            outline->AddLine(bounds.Left + ARROW_SIZE, localHotSpot.Y - ARROW_BASE / 2, bounds.Left + ARROW_SIZE,
                             bounds.Top + cornerSize);

            break;
          }
          case mforms::StartAbove: {
            outline->AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, 180, 90);
            outline->AddArc(bounds.Right - cornerSize, bounds.Top, cornerSize, cornerSize, -90, 90);
            outline->AddArc(bounds.Right - cornerSize, bounds.Bottom - cornerSize - ARROW_SIZE, cornerSize, cornerSize,
                            0, 90);

            // Arrow.
            outline->AddLine(bounds.Right - cornerSize, bounds.Bottom - ARROW_SIZE, localHotSpot.X + ARROW_BASE / 2,
                             bounds.Bottom - ARROW_SIZE);
            outline->AddLine(localHotSpot.X + ARROW_BASE / 2, bounds.Bottom - ARROW_SIZE, localHotSpot.X,
                             bounds.Bottom);
            outline->AddLine(localHotSpot.X, bounds.Bottom, localHotSpot.X - ARROW_BASE / 2,
                             bounds.Bottom - ARROW_SIZE);
            outline->AddLine(localHotSpot.X - ARROW_BASE / 2, bounds.Bottom - ARROW_SIZE, bounds.Left + cornerSize,
                             bounds.Bottom - ARROW_SIZE);

            outline->AddArc(bounds.Left, bounds.Bottom - cornerSize - ARROW_SIZE, cornerSize, cornerSize, 90, 90);
            break;
          }
          case mforms::StartBelow: {
            outline->AddArc(bounds.Left, bounds.Top + ARROW_SIZE, cornerSize, cornerSize, 180, 90);

            // Arrow.
            outline->AddLine(bounds.Left + cornerSize, bounds.Top + ARROW_SIZE, localHotSpot.X - ARROW_BASE / 2,
                             bounds.Top + ARROW_SIZE);
            outline->AddLine(localHotSpot.X - ARROW_BASE / 2, bounds.Top + ARROW_SIZE, localHotSpot.X, bounds.Top);
            outline->AddLine(localHotSpot.X, bounds.Top, localHotSpot.X + ARROW_BASE / 2, bounds.Top + ARROW_SIZE);
            outline->AddLine(localHotSpot.X + ARROW_BASE / 2, bounds.Top + ARROW_SIZE, bounds.Right - cornerSize,
                             bounds.Top + ARROW_SIZE);

            outline->AddArc(bounds.Right - cornerSize, bounds.Top + ARROW_SIZE, cornerSize, cornerSize, -90, 90);
            outline->AddArc(bounds.Right - cornerSize, bounds.Bottom - cornerSize, cornerSize, cornerSize, 0, 90);
            outline->AddArc(bounds.Left, bounds.Bottom - cornerSize, cornerSize, cornerSize, 90, 90);
            break;
          }

          break;
        }
    }
    outline->CloseAllFigures();
  }

  //------------------------------------------------------------------------------------------------

public:
  PopoverControl() {
    AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
    AutoValidate = System::Windows::Forms::AutoValidate::Disable;
    FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
    Owner = UtilitiesWrapper::get_mainform();
    Name = "PopoverControl";
    ShowIcon = false;
    ShowInTaskbar = false;
    StartPosition = FormStartPosition::Manual;
    UseWaitCursor = false;
    Padding = System::Windows::Forms::Padding(7);

    cornerSize = 14;
    animationSteps = 8;
  }

  //------------------------------------------------------------------------------------------------

  void SetBaseSize(int width, int height) {
    baseSize = System::Drawing::Size(width, height);
  }

  //------------------------------------------------------------------------------------------------

  void DoRepaint() {
    Invalidate();
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Shows the popover with its hotspot at the given position. The window is moved accordingly and also
   * considers screen borders.
   */
  void Show(int x, int y, mforms::StartPosition position) {
    if (x < 0 && y < 0) {
      x = ::Cursor::Position.X + 8;
      y = ::Cursor::Position.Y + 8;
    }

    // Release the current mouse capture, in case this method was called in a mouse down/click event handler.
    Win32::ReleaseCapture();

    hotSpot.X = x;
    hotSpot.Y = y;

    // Compute the coordinates starting with the given position.
    ComputeCoordinatesAndPadding();

    if (!Visible || relativePosition != position) {
      relativePosition = position;
      UpdateAndShowPopover(true);
    }
  }

  //------------------------------------------------------------------------------------------------

  void HidePopup() {
    if (animated && !IsDisposed) {
      for (int i = animationSteps; i > 0; i--) {
        Opacity = i / (float)animationSteps;
        Sleep(200 / animationSteps);
      }
    }
    Hide();
  }

  //------------------------------------------------------------------------------------------------

  virtual property base::Rect DisplayRect {
    base::Rect get() {
      System::Drawing::Rectangle content_area = ClientRectangle;
      content_area.X += Padding.Left;
      content_area.Y += Padding.Top;
      content_area.Width -= Padding.Horizontal;
      content_area.Height -= Padding.Vertical;

      return base::Rect(content_area.Left, content_area.Top, content_area.Width, content_area.Height);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual property bool ShowWithoutActivation {
    bool get() override {
      return true;
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual property System::Windows::Forms::CreateParams ^
    CreateParams {
      System::Windows::Forms::CreateParams ^ get() override {
        System::Windows::Forms::CreateParams ^ cp = Form::CreateParams;

        cp->ExStyle |= (int)MySQL::Utilities::SysUtils::WS::EX_NOACTIVATE;
        cp->ClassStyle |= CS_DROPSHADOW;

        return cp;
      }
    }

    //------------------------------------------------------------------------------------------------

    property mforms::PopoverStyle Style {
    mforms::PopoverStyle get() {
      return style;
    };

    void set(mforms::PopoverStyle aStyle) {
      style = aStyle;
      switch (style) {
        case mforms::PopoverStyleTooltip:
          cornerSize = 0; // Not used currently.
          break;

        default:
          cornerSize = 14;
          break;
      }
    };
  }
};

//----------------- PopoverWrapper -----------------------------------------------------------------

PopoverWrapper::PopoverWrapper(mforms::Popover *backend) : ObjectWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool PopoverWrapper::create(mforms::Popover *backend, mforms::View *owner, mforms::PopoverStyle style) {
  PopoverWrapper *wrapper = new PopoverWrapper(backend);
  PopoverControl ^ control = PopoverWrapper::Create<PopoverControl>(backend, wrapper);
  control->Style = style;

  return true;
}

//--------------------------------------------------------------------------------------------------

void PopoverWrapper::destroy(mforms::Popover *backend) {
  PopoverControl ^ popover = PopoverWrapper::GetManagedObject<PopoverControl>(backend);
  PopoverWrapper *wrapper = PopoverWrapper::GetWrapper<PopoverWrapper>(popover);
  wrapper->_track_connection.disconnect();
}

//--------------------------------------------------------------------------------------------------

void PopoverWrapper::set_content(mforms::Popover *backend, mforms::View *content) {
  Control ^ child = PopoverWrapper::GetControl(content);
  child->Dock = DockStyle::Fill;

  Control ^ host = PopoverWrapper::GetControl(backend);
  host->Controls->Add(child);
}

//--------------------------------------------------------------------------------------------------

void PopoverWrapper::set_size(mforms::Popover *backend, int width, int height) {
  PopoverControl ^ popover = PopoverWrapper::GetManagedObject<PopoverControl>(backend);
  popover->SetBaseSize(width, height);
}

//--------------------------------------------------------------------------------------------------

void PopoverWrapper::show(mforms::Popover *backend, int spot_x, int spot_y, mforms::StartPosition position) {
  PopoverControl ^ popover = PopoverWrapper::GetManagedObject<PopoverControl>(backend);
  popover->Show(spot_x, spot_y, position);
}

//--------------------------------------------------------------------------------------------------

void PopoverWrapper::show_and_track(mforms::Popover *backend, mforms::View *owner, int spot_x, int spot_y,
                                    mforms::StartPosition position) {
  PopoverControl ^ popover = PopoverWrapper::GetManagedObject<PopoverControl>(backend);
  PopoverWrapper *wrapper = PopoverWrapper::GetWrapper<PopoverWrapper>(popover);
  wrapper->_track_connection =
    owner->signal_mouse_leave()->connect(std::bind(&PopoverWrapper::mouse_left_tracked_object, wrapper));

  popover->Show(spot_x, spot_y, position);
}

//--------------------------------------------------------------------------------------------------

bool PopoverWrapper::mouse_left_tracked_object() {
  _track_connection.disconnect();

  mforms::Popover *popover = GetBackend<mforms::Popover>();
  (*popover->signal_close())();

  return false;
}

//--------------------------------------------------------------------------------------------------

base::Rect PopoverWrapper::get_content_rect(mforms::Popover *backend) {
  PopoverControl ^ popover = PopoverWrapper::GetManagedObject<PopoverControl>(backend);
  return popover->DisplayRect;
}

//--------------------------------------------------------------------------------------------------

void PopoverWrapper::close(mforms::Popover *backend) {
  PopoverControl ^ popover = PopoverWrapper::GetManagedObject<PopoverControl>(backend);
  popover->HidePopup();
}

//--------------------------------------------------------------------------------------------------

void PopoverWrapper::setName(mforms::Popover *backend, const std::string &name) {
  PopoverControl ^ popover = PopoverWrapper::GetManagedObject<PopoverControl>(backend);
  popover->Name = CppStringToNative(name);
  popover->AccessibleName = popover->Name;
}

//--------------------------------------------------------------------------------------------------

void PopoverWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_popover_impl.create = &PopoverWrapper::create;
  f->_popover_impl.destroy = &PopoverWrapper::destroy;
  f->_popover_impl.set_content = &PopoverWrapper::set_content;
  f->_popover_impl.set_size = &PopoverWrapper::set_size;
  f->_popover_impl.show = &PopoverWrapper::show;
  f->_popover_impl.show_and_track = &PopoverWrapper::show_and_track;
  f->_popover_impl.close = &PopoverWrapper::close;
  f->_popover_impl.setName = &PopoverWrapper::setName;
}

//--------------------------------------------------------------------------------------------------
