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
#include "wf_selector.h"

using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Windows::Forms;

using namespace MySQL::Forms;

//----------------- SeparatorItem ------------------------------------------------------------------

/**
  * Helper class for drawing separator lines in a combobox.
  */
ref class SeparatorItem {
private:
  Object ^ _data;

public:
  SeparatorItem(Object ^ data) {
    _data = data;
  }

  virtual String ^ ToString() override {
    if (_data != nullptr) {
      return _data->ToString();
    }
    return "";
  }
};

//--------------------------------------------------------------------------------------------------

ref class MformsComboBox : ComboBox {
public:
#define SEPARATOR_HEIGHT 5
#define VERTICAL_ITEM_PADDING 4

  virtual void OnMeasureItem(MeasureItemEventArgs ^ args) override {
    // Fetch the current item we're painting as specified by the index.
    Object ^ comboBoxItem = Items[args->Index];

    // If we are a separator item, add in room for the separator.
    if (comboBoxItem->GetType() == SeparatorItem::typeid) {
      args->ItemHeight = SEPARATOR_HEIGHT;
      args->ItemWidth = 50;
    } else {
      // Measure the text of the item;
      String ^ s = comboBoxItem->ToString();
      if (s == "")
        s = "Ag";
      Drawing::Size textSize = args->Graphics->MeasureString(s, Font).ToSize();
      args->ItemHeight = textSize.Height + VERTICAL_ITEM_PADDING;
      args->ItemWidth = textSize.Width;
    }
  }

  //--------------------------------------------------------------------------------------------------

  virtual void OnDrawItem(DrawItemEventArgs ^ args) override {
    //__super::OnDrawItem(args);

    if (args->Index == -1)
      return;

    Object ^ comboBoxItem = Items[args->Index];

    bool isSeparatorItem = (comboBoxItem->GetType() == SeparatorItem::typeid);

    Drawing::Rectangle bounds = args->Bounds;

    // Let the box handle the default stuff.
    args->DrawBackground();
    args->DrawFocusRectangle();

    if (isSeparatorItem) {
      // Start with the background.
      SolidBrush brush(BackColor);
      args->Graphics->FillRectangle(% brush, bounds);

      // Now the gradient line.
      LinearGradientBrush gradientBrush(Point(0, 1), Point(bounds.Width, 1), Color::FromArgb(255, 66, 111, 166),
                                        SystemColors::Window);

      Pen pen(% gradientBrush);

      int center = (bounds.Bottom + bounds.Top) / 2;
      args->Graphics->DrawLine(% pen, bounds.Left + 2, center, bounds.Right - 2, center);
    } else {
      // Draw the string vertically centered but on the left.
      Brush ^ textBrush = gcnew SolidBrush(args->ForeColor);
      StringFormat ^ format = gcnew StringFormat();
      format->LineAlignment = StringAlignment::Center;
      format->Alignment = StringAlignment::Near;
      format->FormatFlags = StringFormatFlags::NoWrap;

      args->Graphics->DrawString(comboBoxItem->ToString(), Font, textBrush, bounds, format);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnTextChanged(System::EventArgs ^ args) override {
    __super ::OnTextChanged(args);

    mforms::Selector *backend = SelectorWrapper::GetBackend<mforms::Selector>(this);
    if (backend != NULL)
      backend->callback();
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnSelectedIndexChanged(System::EventArgs ^ args) override {
    __super ::OnSelectedIndexChanged(args);

    mforms::Selector *backend = SelectorWrapper::GetBackend<mforms::Selector>(this);
    if (backend != NULL)
      backend->callback();
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- SelectorWrapper ----------------------------------------------------------------

SelectorWrapper::SelectorWrapper(mforms::Selector *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool SelectorWrapper::create(mforms::Selector *backend, mforms::SelectorStyle style) {
  SelectorWrapper *wrapper = new SelectorWrapper(backend);
  MformsComboBox ^ combobox = SelectorWrapper::Create<MformsComboBox>(backend, wrapper);

  switch (style) {
    case mforms::SelectorPopup:
      combobox->DropDownStyle = ComboBoxStyle::DropDownList;
      break;
    default:
      combobox->DropDownStyle = ComboBoxStyle::DropDown;
      break;
  }

  combobox->MinimumSize = Size(50, combobox->PreferredSize.Height);
  combobox->DropDownHeight = 100;
  combobox->DrawMode = DrawMode::OwnerDrawVariable;

  return true;
}

//--------------------------------------------------------------------------------------------------

void SelectorWrapper::clear(mforms::Selector *backend) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);
  combobox->Items->Clear();
}

//--------------------------------------------------------------------------------------------------

int SelectorWrapper::add_item(mforms::Selector *backend, const std::string &item) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);

  int i;
  if (item == "-")
    i = combobox->Items->Add(gcnew SeparatorItem(""));
  else
    i = combobox->Items->Add(CppStringToNativeRaw(item));
  if (i == 0) // select 1st item by default, instead of -1
    combobox->SelectedIndex = 0;
  return i;
}

//--------------------------------------------------------------------------------------------------

/**
  * Computes the width of the widest entry in the given combobox. Can be used to resize the combobox
  * or only its dropdown list.
  */
int ComputeContentWidth(ComboBox ^ box) {
  int width = 0;
  Graphics ^ g = box->CreateGraphics();
  Font ^ font = box->Font;
  int vertScrollBarWidth = (box->Items->Count > box->MaxDropDownItems) ? SystemInformation::VerticalScrollBarWidth : 0;

  int newWidth;
  for each(Object ^ item in box->Items) {
      if (item->GetType() == SeparatorItem::typeid)
        newWidth = 30;
      else
        newWidth = (int)g->MeasureString((String ^)item, font).Width + vertScrollBarWidth;
      if (width < newWidth)
        width = newWidth;
    }
  return width;
}

//------------------------------------------------------------------------------------------------

void SelectorWrapper::add_items(mforms::Selector *backend, const std::list<std::string> &items) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);

  cli::array<Object ^> ^ strings = gcnew cli::array<Object ^>((int)items.size());
  int i = 0;

  for (std::list<std::string>::const_iterator iter = items.begin(); iter != items.end(); ++iter) {
    if (*iter == "-")
      strings[i++] = gcnew SeparatorItem("");
    else
      strings[i++] = CppStringToNativeRaw(*iter);
  }

  combobox->Items->AddRange(strings);

  if (i > 0)
    combobox->SelectedIndex = 0;

  int width = ComputeContentWidth(combobox);

  // Do some sanity checks.
  if (width < 50)
    width = 50;
  else if (width > 500)
    width = 500;
  combobox->Width = width + 20; // Account for the dropdown arrow.
  combobox->MinimumSize = Size(width + 20, combobox->PreferredSize.Height);
}

//--------------------------------------------------------------------------------------------------

std::string SelectorWrapper::get_text(mforms::Selector *backend) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);
  return NativeToCppStringRaw(combobox->Text);
}

//--------------------------------------------------------------------------------------------------

std::string SelectorWrapper::get_item(mforms::Selector *backend, int index) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);
  Object ^ comboBoxItem = combobox->Items[index];

  if (comboBoxItem->GetType() == SeparatorItem::typeid)
    return "";
  return NativeToCppStringRaw((String ^)combobox->Items[index]);
}

//--------------------------------------------------------------------------------------------------

void SelectorWrapper::set_index(mforms::Selector *backend, int index) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);
  if (index < combobox->Items->Count)
    combobox->SelectedIndex = index;
}

//--------------------------------------------------------------------------------------------------

int SelectorWrapper::get_index(mforms::Selector *backend) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);
  return combobox->SelectedIndex;
}

//--------------------------------------------------------------------------------------------------

int SelectorWrapper::get_item_count(mforms::Selector *backend) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);
  return combobox->Items->Count;
}

//--------------------------------------------------------------------------------------------------

void SelectorWrapper::set_value(mforms::Selector *backend, const std::string &value) {
  ComboBox ^ combobox = SelectorWrapper::GetManagedObject<ComboBox>(backend);
  combobox->Text = CppStringToNativeRaw(value);
}

//--------------------------------------------------------------------------------------------------

void SelectorWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_selector_impl.create = &SelectorWrapper::create;
  f->_selector_impl.clear = &SelectorWrapper::clear;
  f->_selector_impl.add_item = &SelectorWrapper::add_item;
  f->_selector_impl.add_items = &SelectorWrapper::add_items;
  f->_selector_impl.set_index = &SelectorWrapper::set_index;
  f->_selector_impl.get_item = &SelectorWrapper::get_item;
  f->_selector_impl.get_text = &SelectorWrapper::get_text;
  f->_selector_impl.get_index = &SelectorWrapper::get_index;
  f->_selector_impl.get_item_count = &SelectorWrapper::get_item_count;
  f->_selector_impl.set_value = &SelectorWrapper::set_value;
}

//--------------------------------------------------------------------------------------------------
