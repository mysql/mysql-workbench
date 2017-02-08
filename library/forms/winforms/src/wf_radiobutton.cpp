/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_radiobutton.h"

#include "base/string_utilities.h"

using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

//----------------- MformsRadioButton --------------------------------------------------------------

ref class MformsRadioButton : RadioButton {
public:
  MformsRadioButton() {
    ForeColor = Color::FromKnownColor(KnownColor::WindowText);
  }

  virtual void OnClick(System::EventArgs ^ args) override {
    __super ::OnClick(args);

    mforms::RadioButton *backend = RadioButtonWrapper::GetBackend<mforms::RadioButton>(this);
    if (backend != NULL)
      backend->callback();
  }
};

//----------------- RadioButtonWrapper -------------------------------------------------------------

RadioButtonWrapper::RadioButtonWrapper(mforms::RadioButton *button) : ButtonWrapper(button) {
}

//--------------------------------------------------------------------------------------------------

bool RadioButtonWrapper::create(mforms::RadioButton *backend, int) {
  RadioButtonWrapper *wrapper = new RadioButtonWrapper(backend);

  MformsRadioButton ^ button = RadioButtonWrapper::Create<MformsRadioButton>(backend, wrapper);

  return true;
}

//--------------------------------------------------------------------------------------------------

void RadioButtonWrapper::set_active(mforms::RadioButton *backend, bool flag) {
  RadioButton ^ button = RadioButtonWrapper::GetManagedObject<RadioButton>(backend);
  button->Checked = flag;
}

//--------------------------------------------------------------------------------------------------

bool RadioButtonWrapper::get_active(mforms::RadioButton *backend) {
  RadioButton ^ button = RadioButtonWrapper::GetManagedObject<RadioButton>(backend);
  return button->Checked;
}

//--------------------------------------------------------------------------------------------------

void RadioButtonWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_radio_impl.create = &RadioButtonWrapper::create;
  f->_radio_impl.set_active = &RadioButtonWrapper::set_active;
  f->_radio_impl.get_active = &RadioButtonWrapper::get_active;
}

//--------------------------------------------------------------------------------------------------
