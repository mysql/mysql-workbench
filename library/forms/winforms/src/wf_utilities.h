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

#pragma once

using namespace System::Runtime::InteropServices;

namespace MySQL {
  namespace Forms {

    // Message type for the C# interface.
  public
    enum class MessageType { MessageInfo, MessageWarning, MessageError };

    /**
     * A custom message box for the output as there is no predefined dialog which allows to
     * have custom button captions.
     */
  public
    ref class CustomMessageBox : System::Windows::Forms::Form {
    private:
      // Constructor is private. CustomMessageBox should be accessed through the public Show() method
      CustomMessageBox();

      // GUI Elements, we have 3 buttons whose text can be customized.
      System::Windows::Forms::Label ^ _messageLabel;
      System::Windows::Forms::Button ^ _button1;
      System::Windows::Forms::Button ^ _button2;
      System::Windows::Forms::Button ^ _button3;
      System::Windows::Forms::PictureBox ^ _picture;
      System::Windows::Forms::CheckBox ^ _checkbox;

      void ComputeLayout();
      void ButtonClick(System::Object ^ sender, EventArgs ^ e);

      static mforms::DialogResult ShowInternal(const std::string &title, const std::string &text, PCWSTR mainIcon,
                                               const std::string &buttonOK, const std::string &buttonCancel,
                                               const std::string &buttonOther, const std::string &checkbox,
                                               bool &checked);

    public:
      // C++ interface
      static mforms::DialogResult Show(const std::string &title, const std::string &text, PCWSTR mainIcon,
                                       const std::string &buttonOK, const std::string &buttonCancel,
                                       const std::string &buttonOther, const std::string &checkbox, bool &checked);

      // C# interface
      static System::Windows::Forms::DialogResult Show(MessageType type, String ^ title, String ^ text,
                                                       String ^ buttonOK, String ^ buttonCancel, String ^ buttonOther,
                                                       String ^ checkbox, [Out] bool % checked);
      static System::Windows::Forms::DialogResult Show(MessageType type, String ^ title, String ^ text,
                                                       String ^ buttonOK);
    };

  private
    ref class InvokationResult {
    private:
      void *_result;

    public:
      InvokationResult(void *result) {
        _result = result;
      }

      property void *Result{void * get(){return _result;
    }
  };
};

private
ref class SlotWrapper {
public:
  const std::function<void *()> *_slot;

  SlotWrapper(const std::function<void *()> &slot) {
    // Make a copy of the slot or it will be invalid at the time we want to run it.
    _slot = new std::function<void *()>(slot);
  }

  ~SlotWrapper() {
    delete _slot;
  }
};

private
ref class DispatchControl : System::Windows::Forms::Control {
private:
  InvokationResult ^ RunSlot(SlotWrapper ^ wrapper);

public:
  void *RunOnMainThread(const std::function<void *()> &slot, bool wait);
};

public
class UtilitiesWrapper {
private:
  static gcroot<DispatchControl ^> dispatcher;

  static gcroot<Drawing::Font ^> last_font;

  static void load_passwords();
  static void unload_passwords(bool store);

protected:
  UtilitiesWrapper();

  static void beep();
  static int show_message(const std::string &title, const std::string &text, const std::string &ok,
                          const std::string &cancel, const std::string &other);
  static int show_error(const std::string &title, const std::string &text, const std::string &ok,
                        const std::string &cancel, const std::string &other);
  static int show_warning(const std::string &title, const std::string &text, const std::string &ok,
                          const std::string &cancel, const std::string &other);
  static int show_message_with_checkbox(const std::string &title, const std::string &text, const std::string &ok,
                                        const std::string &cancel, const std::string &other,
                                        const std::string &checkbox_text, bool &isChecked);
  static void show_wait_message(const std::string &title, const std::string &text);
  static bool hide_wait_message();
  static bool run_cancelable_wait_message(const std::string &title, const std::string &text,
                                          const std::function<void()> &signal_ready,
                                          const std::function<bool()> &cancel_slot);
  static void stop_cancelable_wait_message();

  static void set_clipboard_text(const std::string &content);
  static std::string get_clipboard_text();
  static std::string get_special_folder(mforms::FolderType type);

  static void open_url(const std::string &url);
  static bool move_to_trash(const std::string &file_name);
  static void reveal_file(const std::string &path);

  static mforms::TimeoutHandle add_timeout(float interval, const std::function<bool()> &slot);
  static void cancel_timeout(mforms::TimeoutHandle h);

  static void store_password(const std::string &service, const std::string &account, const std::string &password);
  static bool find_password(const std::string &service, const std::string &account, std::string &password);
  static void forget_password(const std::string &service, const std::string &account);

  static void *perform_from_main_thread(const std::function<void *()> &slot, bool wait);
  static void set_thread_name(const std::string &name);

  static double get_text_width(const std::string &text, const std::string &font);

public:
  static System::Windows::Forms::Form ^ get_mainform();

  static void init();
};
}
;
}
;
