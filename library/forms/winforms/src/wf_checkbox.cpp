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
#include "wf_button.h"
#include "wf_checkbox.h"

using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

//----------------- MformsCheckBox -----------------------------------------------------------------

ref class MformsCheckBox : CheckBox {
protected:
  virtual void OnClick(System::EventArgs ^ args) override {
    __super ::OnClick(args);

    mforms::CheckBox *button = CheckBoxWrapper::GetBackend<mforms::CheckBox>(this);
    if (button != NULL)
      button->callback();
  }

public:
  MformsCheckBox() {
    ForeColor = Color::FromKnownColor(KnownColor::WindowText);
  }
};

//----------------- CheckBoxWrapper ----------------------------------------------------------------

bool CheckBoxWrapper::create(mforms::CheckBox *backend, bool square) {
  CheckBoxWrapper *wrapper = new CheckBoxWrapper(backend);
  MformsCheckBox ^ box = CheckBoxWrapper::Create<MformsCheckBox>(backend, wrapper);

  if (square)
    box->Appearance = Appearance::Button;

  return true;
}

//--------------------------------------------------------------------------------------------------

void CheckBoxWrapper::set_active(mforms::CheckBox *backend, bool flag) {
  CheckBox ^ checkbox = CheckBoxWrapper::GetManagedObject<CheckBox>(backend);
  checkbox->Checked = flag;
}

//--------------------------------------------------------------------------------------------------

bool CheckBoxWrapper::get_active(mforms::CheckBox *backend) {
  CheckBox ^ checkbox = CheckBoxWrapper::GetManagedObject<CheckBox>(backend);
  return checkbox->Checked;
}

//--------------------------------------------------------------------------------------------------

CheckBoxWrapper::CheckBoxWrapper(mforms::CheckBox *cbox) : ButtonWrapper(cbox) {
}

//--------------------------------------------------------------------------------------------------

int CheckBoxWrapper::set_text(const std::string &text) {
  int height = __super ::set_text(text);
  MformsCheckBox ^ checkbox = GetManagedObject<MformsCheckBox>();
  assert(checkbox != nullptr);
  if (height < checkbox->PreferredSize.Height)
    height = checkbox->PreferredSize.Height;
  checkbox->MinimumSize = System::Drawing::Size(checkbox->MinimumSize.Width, height);
  return height;
}

//--------------------------------------------------------------------------------------------------

void CheckBoxWrapper::set_font(const std::string &fontDescription) {
  __super ::set_font(fontDescription);
  MformsCheckBox ^ checkbox = GetManagedObject<MformsCheckBox>();
  assert(checkbox != nullptr);
  Graphics ^ graphics = checkbox->CreateGraphics();
  if (graphics != nullptr) {
    Font ^ font = checkbox->Font != nullptr ? checkbox->Font : checkbox->DefaultFont;
    int height = 0;
    if (font) {
      String ^ text = String::IsNullOrEmpty(checkbox->Text) ? "Some text" : checkbox->Text;
      height = (int)graphics->MeasureString(text, font).Height;
    }
    if (height < checkbox->PreferredSize.Height)
      height = checkbox->PreferredSize.Height;
    checkbox->MinimumSize = System::Drawing::Size(checkbox->MinimumSize.Width, height);
  }
}

//--------------------------------------------------------------------------------------------------

void CheckBoxWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_checkbox_impl.create = &CheckBoxWrapper::create;
  f->_checkbox_impl.set_active = &CheckBoxWrapper::set_active;
  f->_checkbox_impl.get_active = &CheckBoxWrapper::get_active;
}

//--------------------------------------------------------------------------------------------------
