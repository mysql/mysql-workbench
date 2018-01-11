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
#include "wf_textbox.h"

#include "base/log.h"
#include "base/drawing.h"

using namespace System::Drawing;
using namespace System::Text;
using namespace System::Windows::Forms;

using namespace MySQL::Forms;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

//----------------- TextBoxEx ----------------------------------------------------------------------

bool TextBoxEx::ProcessCmdKey(Message % msg, Keys keyData) {
  // In order to be able to determine certain special keys we have to hook into the chain before any
  // other key handling is performed.
  // XXX: is this really necessary? We can check for the return key in the methods below.
  if (msg.Msg == WM_KEYDOWN) {
    switch (msg.WParam.ToInt32()) {
      case Keys::Return: {
        bool result;
        mforms::TextBox *backend = TextBoxWrapper::GetBackend<mforms::TextBox>(this);
        if (((msg.LParam.ToInt32() >> 16) & KF_EXTENDED) == KF_EXTENDED)
          result = backend->key_event(mforms::KeyReturn, ViewWrapper::GetModifiers(keyData), "");
        else
          result = backend->key_event(mforms::KeyEnter, ViewWrapper::GetModifiers(keyData), "");

        if (result)
          return __super ::ProcessCmdKey(msg, keyData);
        else
          return false;

        break;
      }

      default:
        return __super ::ProcessCmdKey(msg, keyData);
    }
  } else
    return __super ::ProcessCmdKey(msg, keyData);
}

//--------------------------------------------------------------------------------------------------

void TextBoxEx::OnTextChanged(EventArgs ^ args) {
  __super ::OnTextChanged(args);
  mforms::TextBox *textbox = TextBoxWrapper::GetBackend<mforms::TextBox>(this);
  if (textbox != NULL)
    textbox->callback();
}

//--------------------------------------------------------------------------------------------------

void TextBoxEx::OnKeyDown(KeyEventArgs ^ args) {
  __super ::OnKeyDown(args);

  modifiers = ViewWrapper::GetModifiers(args->KeyData);

  // Don't call the backend for the return key. We have already done that.
  if (args->KeyCode != Keys::Return) {
    mforms::KeyCode code = mforms::KeyNone;
    switch (args->KeyCode & Keys::KeyCode) {
      case Keys::Home:
        code = mforms::KeyHome;
        break;

      case Keys::End:
        code = mforms::KeyEnd;
        break;

      case Keys::Prior:
        code = mforms::KeyPrevious;
        break;

      case Keys::Next:
        code = mforms::KeyNext;
        break;

      case Keys::ShiftKey:
      case Keys::ControlKey:
      case Keys::Menu: // Alt key
      case Keys::LWin: // Command on Mac.
        code = mforms::KeyModifierOnly;
        break;
    }

    if (code != mforms::KeyNone) {
      mforms::TextBox *backend = ViewWrapper::GetBackend<mforms::TextBox>(this);
      if (!backend->key_event(code, modifiers, ""))
        args->Handled =
          true; // If the backend consumed the key (by returning false) we mark that to stop further processing.
    }
  }
}

//--------------------------------------------------------------------------------------------------

void TextBoxEx::OnKeyPress(KeyPressEventArgs ^ args) {
  __super ::OnKeyPress(args);

  // Don't call the back end for the return key. We have already done that.
  if (args->KeyChar != '\r') {
    String ^ string = gcnew String(args->KeyChar, 1);

    mforms::TextBox *backend = ViewWrapper::GetBackend<mforms::TextBox>(this);
    if (!backend->key_event(mforms::KeyChar, modifiers, NativeToCppString(string)))
      args->Handled = true;
  }
}

//----------------- TextBoxWrapper --------------------------------------------------------------------

TextBoxWrapper::TextBoxWrapper(mforms::TextBox *text) : ViewWrapper(text) {
}

//--------------------------------------------------------------------------------------------------

bool TextBoxWrapper::create(mforms::TextBox *backend, mforms::ScrollBars scroll_bars) {
  TextBoxWrapper *wrapper = new TextBoxWrapper(backend);

  TextBoxEx ^ textbox = TextBoxWrapper::Create<TextBoxEx>(backend, wrapper);
  textbox->Multiline = true;
  textbox->AcceptsReturn = true;
  ScrollBars native_scrollbars = ScrollBars::None;
  if ((scroll_bars & mforms::HorizontalScrollBar) != 0) {
    if ((scroll_bars & mforms::VerticalScrollBar) != 0)
      native_scrollbars = ScrollBars::Both;
    else
      native_scrollbars = ScrollBars::Horizontal;
  } else if ((scroll_bars & mforms::VerticalScrollBar) != 0)
    native_scrollbars = ScrollBars::Vertical;
  textbox->ScrollBars = native_scrollbars;
  textbox->Size = Size(100, textbox->PreferredSize.Height); // DefaultSize is not accessible here.

  return true;
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::set_bordered(mforms::TextBox *backend, bool bordered) {
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);
  textbox->BorderStyle = bordered ? BorderStyle::FixedSingle : BorderStyle::None;
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::set_text(mforms::TextBox *backend, const std::string &text) {
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);
  textbox->Text = CppStringToNative(text);
  textbox->Select(0, 0);
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::append_text(mforms::TextBox *backend, const std::string &text, bool scroll_to_end) {
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);

  if (!textbox->IsDisposed) {
    textbox->AppendText(CppStringToNative(text));
    if (scroll_to_end && textbox->Text->Length > 0) {
      textbox->Select(textbox->Text->Length - 1, 0);
      textbox->ScrollToCaret();
    }
  }
}

//--------------------------------------------------------------------------------------------------

std::string TextBoxWrapper::get_text(mforms::TextBox *backend) {
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);
  return NativeToCppString(textbox->Text);
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::set_read_only(mforms::TextBox *backend, bool flag) {
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);
  textbox->ReadOnly = flag;
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::set_padding(mforms::TextBox *backend, int pad) {
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);
  textbox->Padding = Padding(pad); // Doesn't have any effect.
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::clear(mforms::TextBox *backend) {
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);
  textbox->Clear();
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::set_monospaced(mforms::TextBox *backend, bool flag) {
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);

  if (flag)
    try {
      textbox->Font = gcnew System::Drawing::Font(DEFAULT_MONOSPACE_FONT_FAMILY, textbox->Font->Size);
    } catch (System::ArgumentException ^ e) {
      // Argument exception pops up when the system cannot find the Regular font style (corrupt font).
      logError("TextBoxWrapper::set_monospaced failed. %s\n", e->Message);
    }
  else
    textbox->ResetFont();
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::get_selected_range(mforms::TextBox *backend, int &start, int &end) {
  // TODO: convert signature to return a std::pair<int, int> instead.
  TextBoxEx ^ textbox = TextBoxWrapper::GetManagedObject<TextBoxEx>(backend);
  start = textbox->SelectionStart;
  end = start + textbox->SelectionLength;
}

//--------------------------------------------------------------------------------------------------

void TextBoxWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_textbox_impl.create = &TextBoxWrapper::create;
  f->_textbox_impl.set_bordered = &TextBoxWrapper::set_bordered;
  f->_textbox_impl.set_text = &TextBoxWrapper::set_text;
  f->_textbox_impl.append_text = &TextBoxWrapper::append_text;
  f->_textbox_impl.set_read_only = &TextBoxWrapper::set_read_only;
  f->_textbox_impl.set_padding = &TextBoxWrapper::set_padding;
  f->_textbox_impl.get_text = &TextBoxWrapper::get_text;
  f->_textbox_impl.get_selected_range = &TextBoxWrapper::get_selected_range;
  f->_textbox_impl.set_monospaced = &TextBoxWrapper::set_monospaced;
  f->_textbox_impl.clear = &TextBoxWrapper::clear;
}

//--------------------------------------------------------------------------------------------------
