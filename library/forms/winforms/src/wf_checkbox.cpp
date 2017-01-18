/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
