/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_app.h"
#include "wf_button.h"

#include "base/string_utilities.h"

using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

//----------------- MformsButton -------------------------------------------------------------------

ref class MformsButton : Button {
public:
  virtual void OnClick(System::EventArgs ^ args) override {
    __super ::OnClick(args);

    mforms::Button *button = ButtonWrapper::GetBackend<mforms::Button>(this);
    if (button != NULL)
      button->callback();
  }
};

//----------------- ButtonWrapper ------------------------------------------------------------------

ButtonWrapper::ButtonWrapper(mforms::Button *backend) : ViewWrapper(backend) {
}

//-------------------------------------------------------------------------------------------------

bool ButtonWrapper::create(mforms::Button *backend, mforms::ButtonType btype) {
  ButtonWrapper *wrapper = new ButtonWrapper(backend);
  MformsButton ^ button = ButtonWrapper::Create<MformsButton>(backend, wrapper);
  button->AutoSize = false;
  button->UseMnemonic = true;

  switch (btype) {
    case mforms::ToolButton:
      button->FlatStyle = FlatStyle::Flat;
      button->TabStop = false;
      button->Text = "";
      button->FlatAppearance->BorderSize = 0;
      button->AutoSize = true;
      button->AutoSizeMode = AutoSizeMode::GrowAndShrink;
      break;

    case mforms::AdminActionButton:
      button->FlatStyle = FlatStyle::System; // Important, other styles don't work.

      // Show the shield icon only if we are running restricted. Otherwise it is meaningless.
      if (!ControlUtilities::ApplicationHasAdminRights())
        Button_SetElevationRequiredState((HWND)button->Handle.ToPointer(), true);
      break;

    case mforms::SmallButton:
      button->FlatStyle = FlatStyle::System;
      button->Font = gcnew Font(DEFAULT_FONT_FAMILY, 7, FontStyle::Regular, GraphicsUnit::Point);
      button->Height = 15;
      break;

    default:
      button->FlatStyle = FlatStyle::System;
  }

  return true;
}

//-------------------------------------------------------------------------------------------------

int ButtonWrapper::set_text(const std::string &text) {
  Control ^ control = GetManagedObject<Control>();
  control->Text = CppStringToNativeRaw(text);

  if (control->Padding.Left == 0 && uses_internal_padding())
    enable_internal_padding(true);

  // Resize the button to fit its content.
  // Keep the button's width, though, if that is currently larger than the computed width.
  Graphics ^ g = control->CreateGraphics();
  SizeF size = g->MeasureString(control->Text, control->Font);
  delete g;

  if (std::ceill(size.Width) < control->Width)
    size.Width = static_cast<float>(control->Width);
  control->Width = static_cast<int>(std::ceill(size.Width));
  return static_cast<int>(std::ceill(size.Height));
}

//-------------------------------------------------------------------------------------------------

void ButtonWrapper::set_text(mforms::Button *backend, const std::string &text) {
  // Note: DON'T try to replace underscores by ampersand to make the UI interpreting
  //       them as mnemonic chars (in conjunction with the following character).
  //       Underscores are VALID characters and here is the wrong place to handle this.
  //       Only the caller can know the context and has to do such replacements.
  ButtonWrapper *wrapper = backend->get_data<ButtonWrapper>();
  wrapper->set_text(text);
}

//-------------------------------------------------------------------------------------------------

void ButtonWrapper::set_icon(mforms::Button *backend, const std::string &icon) {
  Button ^ button = ButtonWrapper::GetManagedObject<Button>(backend);
  String ^ path = AppWrapper::get_image_path(CppStringToNativeRaw(icon));

  if (IO::File::Exists(path)) {
    try {
      button->Image = Image::FromFile(path);

      // Resize the button to fit its content.
      Size size = button->PreferredSize;
      if (size.Width < button->Width)
        size.Width = button->Width;
      if (size.Height < button->Height)
        size.Height = button->Height;
      button->Size = size;
    } catch (...) {
      mforms::Utilities::show_error(_("Error while loading image"), _("An error occured while loading image ") + icon,
                                    _("Close"));
    }
  }
}

//-------------------------------------------------------------------------------------------------

void ButtonWrapper::enable_internal_padding(mforms::Button *backend, bool flag) {
  ButtonWrapper *wrapper = backend->get_data<ButtonWrapper>();
  wrapper->enable_internal_padding(flag);
}

//-------------------------------------------------------------------------------------------------

void ButtonWrapper::enable_internal_padding(bool flag) {
  // The internal padding feature is just some beautifying added to the button
  // which gives it a bit more room left and right of the text than what is the default
  // for it. It should actually be implemented via the View::set_padding function.
  Control ^ control = GetManagedObject<Control>();
  if (flag) {
    if (control->Padding.Left == 0) {
      // Only add extra padding if that didn't happen already.
      int extra = 10;
      control->Padding = Padding::Add(control->Padding, Padding(extra, 0, extra, 0));
    }
  } else
    control->Padding = Padding(0, 0, 0, 0);

  internal_padding = flag;
}

//-------------------------------------------------------------------------------------------------

bool ButtonWrapper::uses_internal_padding() {
  return internal_padding;
}

//-------------------------------------------------------------------------------------------------

void ButtonWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_button_impl.create = &ButtonWrapper::create;
  f->_button_impl.set_text = &ButtonWrapper::set_text;
  f->_button_impl.set_icon = &ButtonWrapper::set_icon;
  f->_button_impl.enable_internal_padding = &ButtonWrapper::enable_internal_padding;
}
