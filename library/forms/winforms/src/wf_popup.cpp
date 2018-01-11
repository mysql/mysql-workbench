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
#include "wf_popup.h"

using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Windows::Forms;
using namespace System::Threading;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;
using namespace MySQL::Utilities::SysUtils;

//-------------------------------------------------------------------------------------------------

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

//----------------- PopupControl ------------------------------------------------------------------

ref class MySQL::Forms::PopupControl : System::Windows::Forms::Form {
protected:
  int modalResult;

public:
  mforms::Popup *backend;

  PopupControl() {
    AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
    AutoValidate = System::Windows::Forms::AutoValidate::Disable;
    FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
    Name = "PopupControl";
    ShowIcon = false;
    ShowInTaskbar = false;
    StartPosition = FormStartPosition::Manual;
    UseWaitCursor = false;
  }

  //-------------------------------------------------------------------------------------------------

  virtual void OnMouseDown(MouseEventArgs ^ args) override {
    __super ::OnMouseDown(args);
    backend->mouse_down(convert_mouse_button(args->Button), args->X, args->Y);
  }

  //-------------------------------------------------------------------------------------------------

  virtual void OnMouseUp(MouseEventArgs ^ args) override {
    __super ::OnMouseUp(args);
    backend->mouse_up(convert_mouse_button(args->Button), args->X, args->Y);
  }

  //-------------------------------------------------------------------------------------------------

  virtual void OnMouseClick(MouseEventArgs ^ args) override {
    __super ::OnMouseClick(args);
    backend->mouse_click(convert_mouse_button(args->Button), args->X, args->Y);
  }

  //-------------------------------------------------------------------------------------------------

  virtual void OnMouseDoubleClick(MouseEventArgs ^ args) override {
    __super ::OnMouseDoubleClick(args);
    backend->mouse_double_click(convert_mouse_button(args->Button), args->X, args->Y);
  }

  //-------------------------------------------------------------------------------------------------

  virtual void OnMouseMove(MouseEventArgs ^ args) override {
    __super ::OnMouseMove(args);
    backend->mouse_move(convert_mouse_button(args->Button), args->X, args->Y);
  }

  //-------------------------------------------------------------------------------------------------

  virtual void OnMouseLeave(EventArgs ^ args) override {
    __super ::OnMouseLeave(args);
    backend->mouse_leave();
  }

  //-------------------------------------------------------------------------------------------------

  virtual void OnMouseEnter(EventArgs ^ args) override {
    __super ::OnMouseEnter(args);
    backend->mouse_enter();
  }

  //-------------------------------------------------------------------------------------------------

  virtual void DoRepaint() {
    Invalidate();
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Shows the popup with its hotspot at the given position. The window is moved accordingly and also
   * considers screen borders.
   */
  virtual int PopupControl::Show(int x, int y) {
    // In the base class the hot spot is the upper left corner.
    Point newLocation = Point(x, y);
    Screen ^ activeScreen = Screen::FromControl(Form::ActiveForm);
    System::Drawing::Rectangle screenBounds = activeScreen->Bounds;

    if (newLocation.X < screenBounds.Left)
      newLocation.X = screenBounds.Left;
    if (newLocation.X + Width > screenBounds.Right)
      newLocation.X = screenBounds.Right - Width;

    if (newLocation.Y < screenBounds.Top)
      newLocation.Y = screenBounds.Top;
    if (newLocation.Y + Height > screenBounds.Bottom)
      newLocation.Y = screenBounds.Bottom - Height;

    Location = newLocation;
    return (int)ShowDialog();
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

  virtual property int ModalResult {
    int get() {
      return modalResult;
    }
    void set(int value) {
      modalResult = value;
    }
  }
};

//----------------- TransparentPopupControl --------------------------------------------------------

/**
  * A simple transparent and rectangular window.
  */
ref class TransparentPopupControl : PopupControl {
private:
  Bitmap ^ contentBitmap;

public:
  TransparentPopupControl::TransparentPopupControl() : PopupControl() {
    contentBitmap = nullptr;
  }

  //------------------------------------------------------------------------------------------------

  virtual void DoRepaint() override {
    if (Visible) {
      PrepareBitmap();
      ControlUtilities::SetBitmap(this, contentBitmap, 255);
    }
  }

  //------------------------------------------------------------------------------------------------

  /// <summary>
  /// Prepares the bitmap used to draw the window. Layered windows (like this one) use a bitmap for their
  /// content, including alpha channel.
  /// </summary>
  void PrepareBitmap() {
    contentBitmap = gcnew Bitmap(Width, Height);
    Graphics ^ g = Graphics::FromImage(contentBitmap);
    g->SmoothingMode = SmoothingMode::HighQuality;

    // Create a 32 bit image surface for cairo from our bitmap.
    Imaging::BitmapData ^ bitmapData = contentBitmap->LockBits(
      System::Drawing::Rectangle(0, 0, Width, Height), Imaging::ImageLockMode::ReadWrite, contentBitmap->PixelFormat);
    unsigned char *data = (unsigned char *)bitmapData->Scan0.ToPointer();
    cairo_surface_t *surface =
      cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, Width, Height, bitmapData->Stride);

    // Let the backend draw its part.
    cairo_t *cr = cairo_create(surface);
    base::Rect displayRect = DisplayRect;

    backend->repaint(cr, (int)displayRect.left(), (int)displayRect.top(), (int)displayRect.width(),
                     (int)displayRect.height());

    contentBitmap->UnlockBits(bitmapData);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnDeactivate(EventArgs ^ args) override {
    __super ::OnDeactivate(args);

    Close();
    backend->closed();
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnKeyPress(KeyPressEventArgs ^ args) override {
    if (args->KeyChar == 27) // Escape char
      Close();               // Will trigger OnDeactivate.
    else
      __super ::OnKeyPress(args);
  }

  //------------------------------------------------------------------------------------------------

  virtual int Show(int x, int y) override {
    Point newLocation = Point(x, y);

    Location = newLocation;
    TopMost = true;
    PrepareBitmap();
    ControlUtilities::SetBitmap(this, contentBitmap, 255);
    Win32::ShowWindow(Handle, (unsigned int)SW::SHOWNOACTIVATE);
    Capture = true;

    return 1;
  }

  //------------------------------------------------------------------------------------------------

  virtual property int ModalResult {
    void set(int value) override {
      // Setting a modal result is our sign to close.
      modalResult = value;
      Close();
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual property System::Windows::Forms::CreateParams ^ CreateParams {
    System::Windows::Forms::CreateParams ^ get() override {
      System::Windows::Forms::CreateParams ^ cp = PopupControl::CreateParams;
      cp->ExStyle |= WS_EX_LAYERED | WS_EX_NOACTIVATE;
      return cp;
    }
  }
};

//----------------- BezelPopupControl --------------------------------------------------------------

ref class BezelPopupControl : PopupControl {
private:
  Bitmap ^ contentBitmap;
  Drawing::Size baseSize;
  float borderSize;
  int shadowSize;
  Drawing::Color shadowColor;
  int shadowOffset;
  int cornerSize;
  int animationSteps;
  bool animated;
  Drawing::Point hotSpot;

public:
  BezelPopupControl() : PopupControl() {
    BackColor = Color::Black;
    ForeColor = Color::White;
    Name = "BezelPopupControl";
    Padding = System::Windows::Forms::Padding(26, 14, 26, 14);
    borderSize = 3;
    shadowSize = 20;
    shadowColor = Color::Black;
    shadowOffset = 3;
    cornerSize = 20;
    animationSteps = 8;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Runs a local message loop to simulate a modal form. Due to the way the popup is displayed we
   * cannot use ShowDialog.
   */
  void RunLoop() {
    modalResult = -1;

    while (modalResult < 0) // TODO: maybe we need to listen to the application exit event.
    {
      Application::DoEvents();
      Thread::Sleep(20);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void DoRepaint() override {
    if (Visible) {
      PrepareBitmap();
      ControlUtilities::SetBitmap(this, contentBitmap, 255);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual int Show(int x, int y) override {
    // Release the current mouse capture, in case this method was called in a mouse down/click event handler.
    Win32::ReleaseCapture();

    hotSpot.X = x;
    hotSpot.Y = y;

    baseSize = System::Drawing::Size(825, 351); // Size by design, without outer shadow.
    Size = System::Drawing::Size(baseSize.Width + 2 * shadowSize + shadowOffset,
                                 baseSize.Height + 2 * shadowSize + shadowOffset);

    // The given position is a hot spot, i.e. we have to move the popup so that our hot spot is at that
    // location. For now we define a point on the border in the right upper border as hotspot.
    // This will later be extended to include an arrow or pointer tip.
    Point newLocation =
      Point(x - baseSize.Width - shadowSize + shadowOffset + cornerSize, y - shadowSize + shadowOffset);
    Screen ^ currentScreen = Screen::FromControl(this);
    System::Drawing::Rectangle screenBounds = currentScreen->WorkingArea;

    if (newLocation.X < screenBounds.Left)
      newLocation.X = screenBounds.Left;
    if (newLocation.X + Width > screenBounds.Right)
      newLocation.X = screenBounds.Right - Width;

    if (newLocation.Y < screenBounds.Top)
      newLocation.Y = screenBounds.Top;
    if (newLocation.Y + Height > screenBounds.Bottom)
      newLocation.Y = screenBounds.Bottom - Height;

    Location = newLocation;

    UpdateAndShowPopup(true);
    Focus();
    RunLoop();
    HidePopup();

    return modalResult;
  }

  //------------------------------------------------------------------------------------------------

  void UpdateAndShowPopup(bool doAnimated) {
    PrepareBitmap();

    // Don't use animations in a terminal session (remote desktop).
    animated = doAnimated && !SystemInformation::TerminalServerSession;
    if (animated) {
      ControlUtilities::SetBitmap(this, contentBitmap, 0);
      Win32::ShowWindow(Handle, (unsigned int)SW::SHOW);
      for (int i = 1; i <= animationSteps; i++)
        ControlUtilities::SetBitmap(this, contentBitmap, (int)(255.0 * i / animationSteps));
    } else {
      ControlUtilities::SetBitmap(this, contentBitmap, 255);
      Win32::ShowWindow(Handle, (unsigned int)SW::SHOW);
    }
  }

  //------------------------------------------------------------------------------------------------

  void HidePopup() {
    if (animated && !IsDisposed) {
      for (int i = animationSteps; i >= 0; i--)
        ControlUtilities::SetBitmap(this, contentBitmap, (int)(255.0 * i / animationSteps));
    }
    Close();
  }

  //------------------------------------------------------------------------------------------------

  GraphicsPath ^
    GetPath() {
      // Generate the outline of the actual content area. Center it around the origin.
      // It will later get transformed to the final position.
      GraphicsPath ^ result = gcnew GraphicsPath();

      float width = (float)baseSize.Width;
      float height = (float)baseSize.Height;
      float cornerSizeF = (float)cornerSize; // Just to avoid a dozen type casts.
      System::Drawing::RectangleF bounds = System::Drawing::RectangleF(-width / 2, -height / 2, width, height);
      result->AddArc(bounds.Left, bounds.Top, cornerSizeF, cornerSizeF, 180, 90);
      result->AddArc(bounds.Right - cornerSizeF, bounds.Top, cornerSizeF, cornerSizeF, -90, 90);
      result->AddArc(bounds.Right - cornerSizeF, bounds.Bottom - cornerSizeF, cornerSizeF, cornerSizeF, 0, 90);
      result->AddArc(bounds.Left, bounds.Bottom - cornerSizeF, cornerSizeF, cornerSizeF, 90, 90);
      result->CloseAllFigures();
      return result;
    }

    //------------------------------------------------------------------------------------------------

    /// <summary>
    /// Prepares the bitmap used to draw the window. Layered windows (like this one) use a bitmap for their
    /// content, including alpha channel.
    /// </summary>
    void PrepareBitmap() {
    contentBitmap = gcnew Bitmap(Width, Height);

    GraphicsPath ^ path = GetPath();
    GraphicsPath ^ innerPath = (GraphicsPath ^)path->Clone();

    // Increase size of the outline by the shadow size and move it to the center.
    // The inner path keeps the original bounds for clipping.
    Matrix ^ matrix = gcnew Matrix();

    float offsetX = (float)Width / 2;
    float offsetY = (float)Height / 2;
    matrix->Translate(offsetX + shadowOffset, offsetY + shadowOffset);
    matrix->Scale(1 + (2 * shadowSize + borderSize) / (float)baseSize.Width,
                  1 + (2 * shadowSize + borderSize) / (float)baseSize.Height);
    path->Transform(matrix);

    // Also move the inner part to its final place.
    matrix->Reset();
    matrix->Translate(offsetX, offsetY);
    innerPath->Transform(matrix);

    Graphics ^ g = Graphics::FromImage(contentBitmap);
    g->SmoothingMode = SmoothingMode::HighQuality;

    // Fill interior.
    Brush ^ brush = gcnew SolidBrush(Color::FromArgb(191, 1, 0, 0));
    g->FillPath(brush, innerPath);
    delete brush;

    // ... and draw border around the interior.
    Pen ^ borderPen = gcnew Pen(Color::FromArgb(200, Color::White));
    borderPen->EndCap = LineCap::Round;
    borderPen->StartCap = LineCap::Round;
    borderPen->Width = borderSize;
    GraphicsPath ^ borderPath = (GraphicsPath ^)innerPath->Clone();
    borderPath->Widen(borderPen);

    brush = gcnew SolidBrush(Color::FromArgb(255, Color::White));
    g->FillPath(brush, borderPath);
    delete brush;

    // Clip out interior. Exclude both, the panel itself as well as its border.
    System::Drawing::Region ^ region = gcnew System::Drawing::Region(innerPath);
    g->SetClip(region, CombineMode::Exclude);
    delete region;

    innerPath->Widen(borderPen);
    region = gcnew System::Drawing::Region(innerPath);
    g->SetClip(region, CombineMode::Exclude);
    delete region;

    PathGradientBrush ^ backStyle = gcnew PathGradientBrush(path);

    backStyle->CenterColor = shadowColor;
    array<Color> ^ colors = {Color::Transparent};
    backStyle->SurroundColors = colors;

    // Make a smooth fade out of the shadow color using the built-in sigma bell curve generator.
    backStyle->SetSigmaBellShape((float)0.4, 1);

    // Now draw the shadow.
    g->FillPath(backStyle, path);
    delete backStyle;

    // Remove clipping for the remaining interior.
    g->ResetClip();
    RectangleF innerBounds = innerPath->GetBounds();

    // Create a 32 bit image surface for cairo from our bitmap.
    Imaging::BitmapData ^ bitmapData = contentBitmap->LockBits(
      System::Drawing::Rectangle(0, 0, Width, Height), Imaging::ImageLockMode::ReadWrite, contentBitmap->PixelFormat);
    unsigned char *data = (unsigned char *)bitmapData->Scan0.ToPointer();
    cairo_surface_t *surface =
      cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, Width, Height, bitmapData->Stride);

    // Let the backend draw its part.
    cairo_t *cr = cairo_create(surface);
    base::Rect displayRect = DisplayRect;

    backend->repaint(cr, (int)displayRect.left(), (int)displayRect.top(), (int)displayRect.width(),
                     (int)displayRect.height());

    contentBitmap->UnlockBits(bitmapData);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnKeyPress(KeyPressEventArgs ^ args) override {
    if (args->KeyChar == 27) // Escape char
      modalResult = 0;
    else
      __super ::OnKeyPress(args);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnLostFocus(EventArgs ^ args) override {
    // When we lose the focus then it means the same as pressing escape. The popup closes with a
    // cancel result.
    modalResult = 0;
    __super ::OnLostFocus(args);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnMouseDown(MouseEventArgs ^ args) override {
    if (!ClientRectangle.Contains(args->Location))
      modalResult = 0;
    else
      __super ::OnMouseDown(args);
  }

  //------------------------------------------------------------------------------------------------

  virtual property base::Rect DisplayRect {
    base::Rect get() override {
      base::Rect rect(0, 0, baseSize.Width, baseSize.Height);
      rect.pos.x += shadowSize + shadowOffset + Padding.Left;
      rect.pos.y += shadowSize + shadowOffset + Padding.Top;
      rect.size.width -= Padding.Horizontal;
      rect.size.height -= Padding.Vertical;

      return rect;
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual property bool ShowWithoutActivation {
    bool get() override {
      return true;
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual property System::Windows::Forms::CreateParams ^ CreateParams {
    System::Windows::Forms::CreateParams ^ get() override {
      System::Windows::Forms::CreateParams ^ cp = __super ::CreateParams;

      cp->ExStyle |= (int)MySQL::Utilities::SysUtils::WS::EX_LAYERED;
      cp->ExStyle |= (int)MySQL::Utilities::SysUtils::WS::EX_NOACTIVATE;
      return cp;
    }
  }
};

//----------------- PopupWrapper -------------------------------------------------------------------

PopupWrapper::PopupWrapper(mforms::Popup *backend) : ObjectWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool PopupWrapper::create(mforms::Popup *backend, mforms::PopupStyle style) {
  PopupWrapper *wrapper = new PopupWrapper(backend);

  PopupControl ^ popup = nullptr;
  switch (style) {
    case mforms::PopupPlain:
      popup = PopupWrapper::Create<TransparentPopupControl>(backend, wrapper);
      break;

    case mforms::PopupBezel:
      popup = PopupWrapper::Create<BezelPopupControl>(backend, wrapper);
      break;

    default:
      popup = PopupWrapper::Create<PopupControl>(backend, wrapper);
      break;
  }

  if (popup != nullptr)
    popup->backend = backend;

  return true;
}

//--------------------------------------------------------------------------------------------------

void PopupWrapper::destroy(mforms::Popup *backend) {
}

//--------------------------------------------------------------------------------------------------

void PopupWrapper::set_needs_repaint(mforms::Popup *backend) {
  PopupControl ^ control = PopupWrapper::GetManagedObject<PopupControl>(backend);
  control->DoRepaint();
}

//--------------------------------------------------------------------------------------------------

void PopupWrapper::set_size(mforms::Popup *backend, int width, int height) {
  PopupControl ^ control = PopupWrapper::GetManagedObject<PopupControl>(backend);
  control->Size = Size(width, height);
}

//--------------------------------------------------------------------------------------------------

int PopupWrapper::show(mforms::Popup *backend, int spot_x, int spot_y) {
  PopupControl ^ control = PopupWrapper::GetManagedObject<PopupControl>(backend);
  control->DoRepaint();
  control->Show(spot_x, spot_y);

  return 0;
}

//--------------------------------------------------------------------------------------------------

base::Rect PopupWrapper::get_content_rect(mforms::Popup *backend) {
  PopupControl ^ control = PopupWrapper::GetManagedObject<PopupControl>(backend);
  return control->DisplayRect;
}

//--------------------------------------------------------------------------------------------------

void PopupWrapper::set_modal_result(mforms::Popup *backend, int result) {
  PopupControl ^ control = PopupWrapper::GetManagedObject<PopupControl>(backend);
  control->ModalResult = result;
}

//--------------------------------------------------------------------------------------------------

void PopupWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_popup_impl.create = &PopupWrapper::create;
  f->_popup_impl.destroy = &PopupWrapper::destroy;
  f->_popup_impl.set_needs_repaint = &PopupWrapper::set_needs_repaint;
  f->_popup_impl.set_size = &PopupWrapper::set_size;
  f->_popup_impl.show = &PopupWrapper::show;
  f->_popup_impl.get_content_rect = &PopupWrapper::get_content_rect;
  f->_popup_impl.set_modal_result = &PopupWrapper::set_modal_result;
}

//--------------------------------------------------------------------------------------------------
