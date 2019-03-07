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
#include "wf_textentry.h"

using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;

//----------------- TextEntryEventTarget -----------------------------------------------------------

ref class MformsTextBox : TextBox {
public:
  bool hasRealText;
  bool changingText;

  String ^ placeholderString;
  Drawing::Color placeholderColor;
  Drawing::Color textColor; // Normal text color.

public:
  MformsTextBox() : TextBox() {
    hasRealText = false;
    changingText = false;

    placeholderColor = SystemColors::GrayText;
    textColor = ForeColor;
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnTextChanged(EventArgs ^ args) override {
    __super ::OnTextChanged(args);

    if (changingText)
      return;

    hasRealText = Text->Length > 0;

    mforms::TextEntry *entry = TextEntryWrapper::GetBackend<mforms::TextEntry>(this);
    if (entry != NULL)
      entry->callback();
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnKeyDown(KeyEventArgs ^ args) override {
    __super ::OnKeyDown(args);

    mforms::TextEntry *entry = TextEntryWrapper::GetBackend<mforms::TextEntry>(this);
    switch (args->KeyCode) {
      case Keys::Return:
        entry->action(mforms::EntryActivate);
        args->SuppressKeyPress = true;
        break;

      case Keys::Up:
        if (args->Control)
          entry->action(mforms::EntryCKeyUp);
        else
          entry->action(mforms::EntryKeyUp);
        args->SuppressKeyPress = true;
        break;

      case Keys::Down:
        if (args->Control)
          entry->action(mforms::EntryCKeyDown);
        else
          entry->action(mforms::EntryKeyDown);
        args->SuppressKeyPress = true;

        break;

      case Keys::Escape:
        entry->action(mforms::EntryEscape);
        args->SuppressKeyPress = true;

        break;
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnGotFocus(EventArgs ^ args) override {
    __super ::OnGotFocus(args);

    if (!hasRealText) {
      ForeColor = textColor;
      changingText = true;
      Text = "";
      changingText = false;
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnLostFocus(EventArgs ^ args) override {
    __super ::OnLostFocus(args);

    if (!hasRealText && placeholderString != nullptr) {
      ForeColor = placeholderColor;
      changingText = true;
      Text = placeholderString;
      changingText = false;
    }
  }

  //------------------------------------------------------------------------------------------------

  void SetText(String ^ newText) {
    Text = newText;

    // Make sure the text box is never made taller than their preferred height.
    Drawing::Size size = PreferredSize;
    size.Width = MaximumSize.Width;
    MaximumSize = size;

    if (newText->Length > 0) {
      if (!hasRealText)
        OnLostFocus(gcnew EventArgs());
      hasRealText = true;
    } else {
      if (hasRealText)
        OnGotFocus(gcnew EventArgs());
      hasRealText = false;
    }
  }

  //------------------------------------------------------------------------------------------------

  String ^
    GetText() {
      if (hasRealText)
        return Text;

      return "";
    }

    //------------------------------------------------------------------------------------------------

    void SetPlaceholderText(String ^ newText) {
    placeholderString = newText;
    if (!Focused && !hasRealText)
      OnLostFocus(gcnew EventArgs());
  }

  //------------------------------------------------------------------------------------------------

  void SetPlaceholderColor(Color color) {
    placeholderColor = color;
    OnLostFocus(gcnew EventArgs());
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- TextEntryWrapper ---------------------------------------------------------------

bool TextEntryWrapper::create(mforms::TextEntry *backend, mforms::TextEntryType type) {
  TextEntryWrapper *wrapper = new TextEntryWrapper(backend);

  MformsTextBox ^ textbox = TextEntryWrapper::Create<MformsTextBox>(backend, wrapper);
  textbox->Multiline = false;
  textbox->AutoSize = false;

  switch (type) {
    case mforms::NormalEntry:
      break;

    case mforms::PasswordEntry:
      textbox->PasswordChar = '*';
      break;

    case mforms::SearchEntry:
      break;
  }

  textbox->Size = Size(100, textbox->PreferredSize.Height); // DefaultSize is not accessible here.
  return true;
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::set_text(mforms::TextEntry *backend, const std::string &text) {
  MformsTextBox ^ box = TextEntryWrapper::GetManagedObject<MformsTextBox>(backend);
  box->SetText(CppStringToNative(text));
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::set_placeholder_text(mforms::TextEntry *backend, const std::string &text) {
  MformsTextBox ^ box = TextEntryWrapper::GetManagedObject<MformsTextBox>(backend);
  box->SetPlaceholderText(CppStringToNative(text));
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::set_placeholder_color(mforms::TextEntry *backend, const std::string &color) {
  MformsTextBox ^ box = TextEntryWrapper::GetManagedObject<MformsTextBox>(backend);
  box->SetPlaceholderColor(ColorTranslator::FromHtml(CppStringToNativeRaw(color)));
}

//--------------------------------------------------------------------------------------------------

std::string TextEntryWrapper::get_text(mforms::TextEntry *backend) {
  MformsTextBox ^ box = TextEntryWrapper::GetManagedObject<MformsTextBox>(backend);
  return NativeToCppString(box->GetText());
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::set_max_length(mforms::TextEntry *backend, int length) {
  TextBox ^ textbox = TextEntryWrapper::GetManagedObject<TextBox>(backend);
  textbox->MaxLength = length;
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::set_read_only(mforms::TextEntry *backend, bool flag) {
  TextBox ^ textbox = TextEntryWrapper::GetManagedObject<TextBox>(backend);
  textbox->ReadOnly = flag;
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::set_bordered(mforms::TextEntry *backend, bool flag) {
  TextBox ^ textbox = TextEntryWrapper::GetManagedObject<TextBox>(backend);
  textbox->BorderStyle = flag ? BorderStyle::Fixed3D : BorderStyle::None;
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::set_front_color(String ^ color) {
  __super ::set_front_color(color);
  MformsTextBox ^ box = GetManagedObject<MformsTextBox>();
  box->textColor = box->ForeColor;
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::cut(mforms::TextEntry *self) {
  TextBox ^ textbox = TextEntryWrapper::GetManagedObject<TextBox>(self);
  textbox->Cut();
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::copy(mforms::TextEntry *self) {
  TextBox ^ textbox = TextEntryWrapper::GetManagedObject<TextBox>(self);
  textbox->Copy();
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::paste(mforms::TextEntry *self) {
  TextBox ^ textbox = TextEntryWrapper::GetManagedObject<TextBox>(self);
  textbox->Paste();
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::select(mforms::TextEntry *self, const base::Range &range) {
  TextBox ^ textbox = TextEntryWrapper::GetManagedObject<TextBox>(self);
  textbox->Select((int)range.position, range.size == (size_t)-1 ? textbox->Text->Length : (int)range.size);
}

//--------------------------------------------------------------------------------------------------

base::Range TextEntryWrapper::get_selection(mforms::TextEntry *self) {
  TextBox ^ textbox = TextEntryWrapper::GetManagedObject<TextBox>(self);
  return base::Range(textbox->SelectionStart, textbox->SelectionLength);
}

//--------------------------------------------------------------------------------------------------

TextEntryWrapper::TextEntryWrapper(mforms::TextEntry *text) : ViewWrapper(text) {
}

//--------------------------------------------------------------------------------------------------

void TextEntryWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_textentry_impl.create = &TextEntryWrapper::create;
  f->_textentry_impl.set_text = &TextEntryWrapper::set_text;
  f->_textentry_impl.set_placeholder_text = &TextEntryWrapper::set_placeholder_text;
  f->_textentry_impl.set_placeholder_color = &TextEntryWrapper::set_placeholder_color;
  f->_textentry_impl.get_text = &TextEntryWrapper::get_text;
  f->_textentry_impl.set_max_length = &TextEntryWrapper::set_max_length;
  f->_textentry_impl.set_read_only = &TextEntryWrapper::set_read_only;
  f->_textentry_impl.set_bordered = &TextEntryWrapper::set_bordered;
  f->_textentry_impl.cut = &TextEntryWrapper::cut;
  f->_textentry_impl.copy = &TextEntryWrapper::copy;
  f->_textentry_impl.paste = &TextEntryWrapper::paste;
  f->_textentry_impl.select = &TextEntryWrapper::select;
  f->_textentry_impl.get_selection = &TextEntryWrapper::get_selection;
}

//--------------------------------------------------------------------------------------------------
