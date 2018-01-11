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

#include "base/log.h"
#include "base/string_utilities.h"

#include "wf_base.h"
#include "wf_view.h"
#include "wf_code_editor.h"
#include "wf_menu.h"
#include "wf_find_panel.h"

#include "Scintilla.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;
using namespace MySQL::Utilities::SysUtils;
using namespace MySQL::Controls;

//----------------- ScintillaControl ---------------------------------------------------------------

ScintillaControl::ScintillaControl() : Control() {
  direct_pointer = 0;
  message_function = NULL;
  backend = NULL;
  file_drop_target = NULL;
  destroying = false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Calls directly into the scintilla backend, which avoids going through the message loop first.
 *   Advantage: fast
 *   Disadvantage: no thread synchronization, but cross-thread calls would not be a good idea anyway
 *                 (considering the other platforms).
 *   The underlying scintilla window has a separate check for cross thread calls.
 */
sptr_t ScintillaControl::direct_call(unsigned int message, uptr_t wParam, sptr_t lParam) {
  if (destroying || Disposing || IsDisposed)
    return -1;

  HWND handle = (HWND)Handle.ToPointer();
  if (handle == 0)
    return -1;

  if (message_function == NULL)
    message_function = (SciFnDirect)SendMessage(handle, SCI_GETDIRECTFUNCTION, 0, 0);

  if (direct_pointer == 0)
    direct_pointer = (sptr_t)SendMessage(handle, SCI_GETDIRECTPOINTER, 0, 0);

  // This call will throw an exception if this thread and the one in which the window has been created
  // are not identical.
  return message_function(direct_pointer, message, wParam, lParam);
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::SetBackend(mforms::CodeEditor *editor) {
  backend = editor;
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::SetDropTarget(mforms::DropDelegate *target) {
  file_drop_target = target;
}

//--------------------------------------------------------------------------------------------------

bool ScintillaControl::CanUndo::get() {
  return direct_call(SCI_CANUNDO, 0, 0) != 0;
}

//--------------------------------------------------------------------------------------------------

bool ScintillaControl::CanRedo::get() {
  return direct_call(SCI_CANREDO, 0, 0) != 0;
}

//--------------------------------------------------------------------------------------------------

bool ScintillaControl::CanCopy::get() {
  sptr_t length = direct_call(SCI_GETSELECTIONEND, 0, 0) - direct_call(SCI_GETSELECTIONSTART, 0, 0);
  return length > 0;
}

//--------------------------------------------------------------------------------------------------

bool ScintillaControl::CanCut::get() {
  return CanCopy && CanDelete;
}

//--------------------------------------------------------------------------------------------------

bool ScintillaControl::CanPaste::get() {
  try {
    // Doesn't usually crash but if the clipboard chain is forcibly broken we might get this
    // error. No sense to show it to the user.
    return Clipboard::ContainsText() && direct_call(SCI_GETREADONLY, 0, 0) == 0;
  } catch (...) {
    logError("Clipboard::ContainsText returned with exception\n");
    return false;
  }
}

//--------------------------------------------------------------------------------------------------

bool ScintillaControl::CanDelete::get() {
  return CanCopy && direct_call(SCI_GETREADONLY, 0, 0) == 0;
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::Undo() {
  direct_call(SCI_UNDO, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::Redo() {
  direct_call(SCI_REDO, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::Copy() {
  direct_call(SCI_COPY, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::Cut() {
  direct_call(SCI_CUT, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::Paste() {
  direct_call(SCI_PASTE, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::Delete() {
  direct_call(SCI_REPLACESEL, 0, (sptr_t) "");
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::SelectAll() {
  direct_call(SCI_SELECTALL, 0, 0);
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::CreateParams ^ ScintillaControl::CreateParams::get() {
  System::Windows::Forms::CreateParams ^ params = Control::CreateParams::get();
  params->ClassName = "Scintilla";

  return params;
};

//--------------------------------------------------------------------------------------------------

mforms::KeyCode ScintillaControl::GetKeyCode(int code) {
  auto key = mforms::KeyUnkown;
  switch (code) {
    case Keys::Return:
      key = mforms::KeyReturn;
      break;
    case Keys::Up:
      key = mforms::KeyUp;
      break;
    case Keys::Down:
      key = mforms::KeyDown;
      break;
    default:
      break;
  }
  return key;
}

//--------------------------------------------------------------------------------------------------

mforms::ModifierKey ScintillaControl::GetModifiers(System::Windows::Forms::Keys keyData) {
  return ViewWrapper::GetModifiers(keyData);
}

//--------------------------------------------------------------------------------------------------

bool ScintillaControl::ProcessCmdKey(System::Windows::Forms::Message % msg, System::Windows::Forms::Keys keyData) {
  if (msg.Msg == WM_KEYDOWN) {
    if (backend->key_event(GetKeyCode(msg.WParam.ToInt32()), GetModifiers(keyData), ""))
      return __super::ProcessCmdKey(msg, keyData);
    return false;
  }
  return __super::ProcessCmdKey(msg, keyData);
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::WndProc(System::Windows::Forms::Message % m) {
  switch (m.Msg) {
    case WM_PAINT:
      // Weird, this message must be forwarded to the default window proc not the inherited WndProc,
      // otherwise nothing is drawn because no WM_PAINT message reaches Scintilla.
      Control::DefWndProc(m);
      break;

    case WM_SETCURSOR:
    case WM_GETTEXT:
    case WM_GETTEXTLENGTH:
      // Only used for getting the window text, which we don't have. The default implementation
      // returns the full text of the editor which produces problems with large content.
      m.Result = IntPtr(0);
      break;

    case WM_NOTIFY + 0x2000: // WM_NOTIFY reflected by .NET from the parent window.
    {
      // Parent notification. Details are passed as SCNotification structure.
      SCNotification *scn = reinterpret_cast<SCNotification *>(m.LParam.ToPointer());
      backend->on_notify(scn);
      break;
    }

    case WM_COMMAND + 0x2000: // Ditto for WM_COMMAND.
    {
      backend->on_command(m.WParam.ToInt32() >> 16);
      break;
    }

    case WM_DROPFILES:
      if (file_drop_target != NULL) {
        HDROP hdrop = (HDROP)m.WParam.ToPointer();
        unsigned count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
        std::vector<std::string> file_names;
        for (UINT i = 0; i < count; ++i) {
          unsigned size = DragQueryFile(hdrop, i, NULL, 0); // Size not including terminator.
          if (size == 0)
            continue;

          TCHAR *buffer = new TCHAR[size + 1];
          if (DragQueryFile(hdrop, i, buffer, size + 1) != 0)
            file_names.push_back(base::wstring_to_string(buffer));
          delete buffer;
        }

        POINT point;
        DragQueryPoint(hdrop, &point);
        ScreenToClient((HWND)Handle.ToPointer(), &point);
        file_drop_target->files_dropped(backend, base::Point(point.x, point.y), mforms::DragOperationCopy, file_names);
      }
      break;

    case WM_NCDESTROY:
      destroying = true;
      break;
    /*case WM_KEYDOWN:
      {
        std::string val;
        val.append((char*)m.WParam.ToInt32());
        if (backend->key_event(GetKeyCode(m.WParam.ToInt32()), GetModifiers(m.LParam.ToInt32()), val))
          Control::WndProc(m);
      }
      break;*/

    default:
      Control::WndProc(m);
  }
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::ShowFindPanel(bool doReplace) {
  backend->show_find_panel(doReplace);
}

//--------------------------------------------------------------------------------------------------

void ScintillaControl::OnMouseDown(MouseEventArgs ^ e) {
  if (e->Button == System::Windows::Forms::MouseButtons::Right) {
    // Update the associated context menu.
    if (backend->get_context_menu() != NULL) {
      System::Windows::Forms::ContextMenuStrip ^ menu =
        CodeEditorWrapper::GetManagedObject<System::Windows::Forms::ContextMenuStrip>(backend->get_context_menu());
      if (menu != ContextMenuStrip) {
        ContextMenuStrip = menu;
        (*backend->get_context_menu()->signal_will_show())();
      }
    } else
      ContextMenuStrip = nullptr;
  }
}

//----------------- CodeEditorWrapper -----------------------------------------------------------------

CodeEditorWrapper::CodeEditorWrapper(mforms::CodeEditor *backend) : ViewWrapper(backend) {
  backend->set_show_find_panel_callback(std::bind(show_find_panel, std::placeholders::_1, std::placeholders::_2));
}

//--------------------------------------------------------------------------------------------------

bool CodeEditorWrapper::create(mforms::CodeEditor *backend, bool showInfo) {
  CodeEditorWrapper *wrapper = new CodeEditorWrapper(backend);
  ScintillaControl ^ editor = CodeEditorWrapper::Create<ScintillaControl>(backend, wrapper);
  editor->SetBackend(backend);

  return true;
}

//--------------------------------------------------------------------------------------------------

sptr_t CodeEditorWrapper::send_editor(mforms::CodeEditor *backend, unsigned int message, uptr_t wParam, sptr_t lParam) {
  ScintillaControl ^ editor = CodeEditorWrapper::GetManagedObject<ScintillaControl>(backend);
  return editor->direct_call(message, wParam, lParam);
}

//--------------------------------------------------------------------------------------------------

void CodeEditorWrapper::show_find_panel(mforms::CodeEditor *backend, bool show) {
  mforms::FindPanel *find_panel = backend->get_find_panel();
  if (find_panel != NULL) {
    Control ^ find_control = FindPanelWrapper::GetControl(find_panel);
    if (show) {
      if (find_control->Parent == nullptr) {
        Control ^ editor_control = CodeEditorWrapper::GetControl(backend);

        // Insert the find panel directly before the editor control (same index).
        int index = editor_control->Parent->Controls->GetChildIndex(editor_control);
        editor_control->Parent->Controls->Add(find_control);
        if (editor_control->Dock == DockStyle::Fill)
          editor_control->Parent->Controls->SetChildIndex(find_control, index + 1);
        else
          editor_control->Parent->Controls->SetChildIndex(find_control, index);
        find_control->Dock = DockStyle::Top;
        find_control->Show();
      }
    } else
      find_control->Parent->Controls->Remove(find_control);
  }
}

//--------------------------------------------------------------------------------------------------

void CodeEditorWrapper::register_file_drop(mforms::DropDelegate *target) {
  ScintillaControl ^ editor = CodeEditorWrapper::GetManagedObject<ScintillaControl>();
  editor->SetDropTarget(target);
  DragAcceptFiles((HWND)editor->Handle.ToPointer(), true);
}

//--------------------------------------------------------------------------------------------------

void CodeEditorWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_code_editor_impl.create = &CodeEditorWrapper::create;
  f->_code_editor_impl.send_editor = &CodeEditorWrapper::send_editor;
}

//--------------------------------------------------------------------------------------------------
