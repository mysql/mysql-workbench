/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

namespace MySQL {
  namespace Forms {

  public
    ref class ScintillaControl : public System::Windows::Forms::Control {
    private:
      sptr_t direct_pointer;
      SciFnDirect message_function;
      mforms::CodeEditor *backend;
      mforms::DropDelegate *file_drop_target;
      bool destroying;

    protected:
      virtual void WndProc(System::Windows::Forms::Message % m) override;
      virtual void OnMouseDown(System::Windows::Forms::MouseEventArgs ^ args) override;
      virtual bool ProcessCmdKey(System::Windows::Forms::Message % msg, System::Windows::Forms::Keys keyData) override;

      virtual property System::Windows::Forms::CreateParams ^
        CreateParams { System::Windows::Forms::CreateParams ^ get() override; }

        public : ScintillaControl();

      sptr_t direct_call(unsigned int message, uptr_t wParam, sptr_t lParam);
      void SetBackend(mforms::CodeEditor *editor);
      void SetDropTarget(mforms::DropDelegate *target);

      mforms::KeyCode GetKeyCode(int code);
      mforms::ModifierKey GetModifiers(System::Windows::Forms::Keys keyData);

      // For interaction with the UI we need some public methods/properties and forward these events
      // to the backend.
      property bool CanUndo {
        bool get();
      }
      property bool CanRedo {
        bool get();
      }
      property bool CanCopy {
        bool get();
      }
      property bool CanCut {
        bool get();
      }
      property bool CanPaste {
        bool get();
      }
      property bool CanDelete {
        bool get();
      }

      void Undo();
      void Redo();
      void Copy();
      void Cut();
      void Paste();
      void Delete();
      void SelectAll();

      void ShowFindPanel(bool doReplace);
    };

    ref class ScintillaControl;

  public
    class CodeEditorWrapper : public ViewWrapper {
    private:
    protected:
      CodeEditorWrapper(mforms::CodeEditor *backend);

      static bool create(mforms::CodeEditor *editor, bool showInfo);
      static sptr_t send_editor(mforms::CodeEditor *editor, unsigned int message, uptr_t wParam, sptr_t lParam);
      static void show_find_panel(mforms::CodeEditor *editor, bool show);

      virtual void register_file_drop(mforms::DropDelegate *target);

    public:
      static void init();
    };
  };
};
