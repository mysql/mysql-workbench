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
#include "wf_utilities.h"

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/log.h"
#include "base/file_utilities.h"

// When linking statically against comctl32 we have to enable this manifest dependency explicitly
// otherwise loading mforms.wr fails with a BadImageFormat exception under 64 bit.
#pragma comment(linker, \
                "\"/manifestdependency:type='win32' \
  name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
  processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Media;

using namespace MySQL::Forms;
using namespace MySQL::Utilities;

using namespace System::Collections::Generic;

//--------------------------------------------------------------------------------------------------

CustomMessageBox::CustomMessageBox() {
  logDebug("Creating custom message box in old style\n");

  _picture = gcnew System::Windows::Forms::PictureBox();
  _button1 = gcnew System::Windows::Forms::Button();
  _button2 = gcnew System::Windows::Forms::Button();
  _button3 = gcnew System::Windows::Forms::Button();
  _messageLabel = gcnew System::Windows::Forms::Label();
  _checkbox = gcnew System::Windows::Forms::CheckBox();

  SuspendLayout();

  Controls->Add(_picture);
  Controls->Add(_button2);
  Controls->Add(_button1);
  Controls->Add(_button3);
  Controls->Add(_messageLabel);
  Controls->Add(_checkbox);

  // picture
  _picture->Name = "picture";
  _picture->TabIndex = 0;
  _picture->TabStop = false;

  // button1
  _button1->Name = "button1";
  _button1->TabIndex = 1;
  _button1->UseVisualStyleBackColor = true;
  _button1->Click += gcnew System::EventHandler(this, &CustomMessageBox::ButtonClick);

  // button2
  _button2->Name = "button2";
  _button2->TabIndex = 2;
  _button2->UseVisualStyleBackColor = true;
  _button2->Click += gcnew System::EventHandler(this, &CustomMessageBox::ButtonClick);

  // button3
  _button3->Name = "button3";
  _button3->TabIndex = 3;
  _button3->UseVisualStyleBackColor = true;
  _button3->Click += gcnew System::EventHandler(this, &CustomMessageBox::ButtonClick);

  // messageText
  _messageLabel->Name = "messageLabel";
  _messageLabel->Padding = System::Windows::Forms::Padding(2);
  _messageLabel->TabIndex = 4;
  _messageLabel->Text = "label1";

  _checkbox->Name = "checkBox";
  _checkbox->TabIndex = 5;

  // Form
  this->Padding = System::Windows::Forms::Padding(8, 16, 8, 8);
  this->FormBorderStyle = ::FormBorderStyle::FixedDialog;
  this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
  this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
  this->StartPosition = FormStartPosition::CenterScreen;
  this->Name = "CustomMessageBox";
  this->Text = "";
  this->ShowInTaskbar = false;
  this->HelpButton = false;
  this->MinimizeBox = false;
  this->MaximizeBox = false;
  // this->TopMost = true;
  this->Owner = UtilitiesWrapper::get_mainform();

  ResumeLayout(false);
}

//--------------------------------------------------------------------------------------------------

// Some more task dialog icons, which are not defined in commctrl.h.
#define TD_SHIELD_BLUE_ICON MAKEINTRESOURCE(-5)
#define TD_SECURITY_WARNING_ICON MAKEINTRESOURCE(-6)
#define TD_SECURITY_ERROR_ICON MAKEINTRESOURCE(-7)
#define TD_SHIELD_SUCCESS_ICON MAKEINTRESOURCE(-8)
#define TD_SHIELD_GRAY_ICON MAKEINTRESOURCE(-9)

mforms::DialogResult CustomMessageBox::ShowInternal(const std::string &title, const std::string &text, PCWSTR mainIcon,
                                                    const std::string &buttonOK, const std::string &buttonCancel,
                                                    const std::string &buttonOther, const std::string &checkbox,
                                                    bool &checked) {
  logDebug("Creating and showing custom message box\n");

  TASKDIALOGCONFIG config = {0};
  config.cbSize = sizeof(config);

  int button = 0;
  int radioButton = 0;
  BOOL verificationChecked = FALSE;

  int buttonCount = 0;
  TASKDIALOG_BUTTON buttons[3];

  std::wstring do_text = base::string_to_wstring(buttonOK);
  if (!do_text.empty()) {
    buttons[buttonCount].nButtonID = IDOK;
    buttons[buttonCount].pszButtonText = do_text.c_str();
    buttonCount++;
  }

  // Enable dialog cancellation via red-cross button if we have a cancel button.
  // If the text of the button is the standard one ("Cancel") then use the built-in
  // button, otherwise create an own command link.
  std::wstring cancel_text = base::string_to_wstring(buttonCancel);
  if (!cancel_text.empty()) {
    config.dwFlags |= TDF_ALLOW_DIALOG_CANCELLATION;
    if (base::tolower(buttonCancel) == "cancel")
      config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    else {
      buttons[buttonCount].nButtonID = IDCANCEL;
      buttons[buttonCount].pszButtonText = cancel_text.c_str();
      buttonCount++;
    }
  }

  std::wstring other_text = base::string_to_wstring(buttonOther);
  if (other_text.size() > 0) {
    buttons[buttonCount].nButtonID = 1000;
    buttons[buttonCount].pszButtonText = other_text.c_str();
    buttonCount++;
  }

  // If we have more than one normal alternatives to show then enable command links.
  if (buttonCount > 1)
    config.dwFlags |= TDF_USE_COMMAND_LINKS;

  config.hwndParent = GetForegroundWindow();

  config.pszMainIcon = mainIcon;
  config.pszWindowTitle = L"MySQL Workbench";

  std::wstring titleText = base::string_to_wstring(title);
  config.pszMainInstruction = titleText.c_str();
  std::wstring descriptionText = base::string_to_wstring(text);
  config.pszContent = descriptionText.c_str();
  config.pButtons = buttons;
  config.cButtons = buttonCount;

  std::wstring checkbox_text = base::string_to_wstring(checkbox);
  if (checkbox_text.size() > 0)
    config.pszVerificationText = checkbox_text.c_str();

  logDebug("Running custom message box\n");

  HRESULT result = TaskDialogIndirect(&config, &button, &radioButton, &verificationChecked);

  logDebug("Custom message box closed\n");

  if (!SUCCEEDED(result))
    return mforms::ResultCancel;

  checked = verificationChecked == TRUE;
  switch (button) {
    case IDOK:
      return mforms::ResultOk;
      break;
    case IDCANCEL:
      return mforms::ResultCancel;
      break;
    default:
      return mforms::ResultOther;
  }
}

//--------------------------------------------------------------------------------------------------

// A little helper for repeating a button setup.
void AdjustButton(System::Windows::Forms::Button ^ button) {
  // Note: we need to set the Enabled property too as indicator if this button should be considered
  // when layouting the form. The Visible property cannot be used as long as the button's parent
  // is still hidden (which is the case when we do the layout process).
  if (button->Text == "") {
    button->Visible = false;
    button->Enabled = false;
  } else {
    Drawing::Size size = button->GetPreferredSize(Drawing::Size::Empty);
    if (size.Width < 75)
      size.Width = 75;
    button->Size = size;
    button->Visible = true;
    button->Enabled = true;
  }
}

//--------------------------------------------------------------------------------------------------

#define MESSAGE_BOX_BUTTON_SPACING 8 // The spacing between two message buttons.
#define MESSAGE_BOX_MIN_WIDTH 300
#define MESSAGE_BOX_MIN_HEIGHT 128

/**
 * As the name already says this function computes the layout of the message box depending on the
 * content (image size, button text etc.).
 */
void CustomMessageBox::ComputeLayout() {
  logDebug2("Layouting custom message box\n");

  SuspendLayout();

  // Buttons have a minimum width of 75 px. Make them all same size and hide those which don't have
  // any text.
  AdjustButton(_button1);
  AdjustButton(_button2);
  AdjustButton(_button3);
  _checkbox->Enabled = _checkbox->Text != "";
  _checkbox->Visible = _checkbox->Enabled;

  int visibleButtonCount = 0;
  System::Drawing::Size size = System::Drawing::Size::Empty;
  if (_button1->Enabled) {
    size = _button1->Size;
    visibleButtonCount++;
  }
  if (_button2->Enabled) {
    size.Height = _button2->Height;
    if (_button2->Width > size.Width)
      size.Width = _button2->Width;
    visibleButtonCount++;
  }
  if (_button3->Enabled) {
    size.Height = _button3->Height;
    if (_button3->Width > size.Width)
      size.Width = _button3->Width;
    visibleButtonCount++;
  }

  // For the common height we use the one from the last visible button we found.
  // Since the font is assumed to be the same for all buttons they also should compute
  // the same height when asked for their preferred size.
  // Compute the total size of the button area on the way (including padding of the container).
  System::Drawing::Size buttonSize = System::Drawing::Size::Empty;
  if (size.Width > 0) {
    // Compute only if there is at least one visible button.
    buttonSize.Height = size.Height;
    if (_button1->Enabled) {
      _button1->Size = size;
      buttonSize.Width = size.Width;
    }
    if (_button2->Enabled) {
      _button2->Size = size;
      if (buttonSize.Width > 0)
        buttonSize.Width += MESSAGE_BOX_BUTTON_SPACING;
      buttonSize.Width += size.Width;
    }
    if (_button3->Enabled) {
      _button3->Size = size;
      if (buttonSize.Width > 0)
        buttonSize.Width += MESSAGE_BOX_BUTTON_SPACING;
      buttonSize.Width += size.Width;
    }

    // For the right spacing between button box and border.
    buttonSize.Width += MESSAGE_BOX_BUTTON_SPACING;
  }

  // Additional size if the checkbox is visible.
  int effective_button_width = buttonSize.Width;
  if (_checkbox->Enabled) {
    _checkbox->Size = _checkbox->PreferredSize;
    effective_button_width += Padding.Left + _checkbox->Size.Width + MESSAGE_BOX_BUTTON_SPACING;
  }

  // Compute message text layout. Start with a certain minimum width.
  int minWidth = (effective_button_width > MESSAGE_BOX_MIN_WIDTH) ? effective_button_width : MESSAGE_BOX_MIN_WIDTH;
  System::Drawing::Size proposedSize = System::Drawing::Size(minWidth, 1);

  // We use the golden ratio to create an appealing layout.
  // Increase width in 10% steps until we found the proper ratio.
  int lastWidth = 0;
  while (true) {
    size = _messageLabel->GetPreferredSize(proposedSize);

    // Terminate loop if we get a height of 0, the same width as in the last run, beyond a maximum width
    // or reached the golden ratio. Getting the same width means we never can get to the golden
    // ratio as the text is too small.
    if ((size.Height == 0) || (size.Width == lastWidth) || (size.Width >= 1000) ||
        ((float)size.Width / size.Height >= 1.618))
      break;

    lastWidth = size.Width;
    proposedSize.Width += proposedSize.Width / 10;
  };

  // GetPreferredSize might return a value smaller than our minimum width. Account for that.
  if (size.Width < minWidth)
    size.Width = minWidth;
  _messageLabel->Size = size;

  // Now that we have the text size compute the overall size of the message box.
  // The image is vertically centered at the left side (with some padding)
  // and the buttons are at the bottom (right aligned).
  int textHeight = _messageLabel->Padding.Vertical + _messageLabel->Height;
  if (textHeight < _picture->Height + _picture->Padding.Vertical)
    textHeight = _picture->Height + _picture->Padding.Vertical;
  size.Width = Padding.Horizontal + _picture->Padding.Horizontal + _picture->Width + _messageLabel->Padding.Horizontal +
               _messageLabel->Width;
  size.Height = Padding.Vertical + textHeight + buttonSize.Height;

  // Make sure we have good looking minimum height.
  if (size.Height < MESSAGE_BOX_MIN_HEIGHT)
    size.Height = MESSAGE_BOX_MIN_HEIGHT;
  ClientSize = size;

  // Move picture to its final location (center vertically over the message text's height).
  Drawing::Point location;
  location.X = Padding.Left;
  location.Y = Padding.Top + (textHeight - _picture->Height - _picture->Padding.Vertical) / 2;
  _picture->Location = location;

  // Text location too.
  location.X = _picture->Right + _messageLabel->Padding.Left;
  location.Y = Padding.Top;
  _messageLabel->Location = location;

  // Move the buttons to their final locations.
  if (buttonSize.Width > 0) {
    location =
      Drawing::Point(ClientSize.Width - buttonSize.Width, ClientSize.Height - buttonSize.Height - Padding.Bottom);
    if (_button1->Enabled) {
      _button1->Location = location;
      location.X += _button1->Width + MESSAGE_BOX_BUTTON_SPACING;
    }
    if (_button3->Enabled) {
      _button3->Location = location;
      location.X += _button3->Width + MESSAGE_BOX_BUTTON_SPACING;
    }

    // Button 2 is our Cancel button (the one which is triggered when ESC is pressed), so place it last.
    if (_button2->Enabled)
      _button2->Location = location;
  }

  // Display the checkbox on the same line as the buttons but left aligned (if visible).
  if (_checkbox->Enabled) {
    location.X = Padding.Left;
    location.Y += (buttonSize.Height - _checkbox->Size.Height) / 2;
    _checkbox->Location = location;
  }

  ResumeLayout(true);
}

//--------------------------------------------------------------------------------------------------

void CustomMessageBox::ButtonClick(Object ^ sender, EventArgs ^ arguments) {
  logDebug2("Button was clicked in custom message box\n");

  DialogResult = ((System::Windows::Forms::Button ^)sender)->DialogResult;
}

//--------------------------------------------------------------------------------------------------

mforms::DialogResult CustomMessageBox::Show(const std::string &title, const std::string &text, PCWSTR mainIcon,
                                            const std::string &buttonOK, const std::string &buttonCancel,
                                            const std::string &buttonOther, const std::string &checkbox,
                                            bool &checked) {
  logDebug("About to show a custom message box\n");

  mforms::Utilities::enter_modal_loop();
  mforms::DialogResult result =
    ShowInternal(title, text, mainIcon, buttonOK, buttonCancel, buttonOther, checkbox, checked);
  mforms::Utilities::leave_modal_loop();

  return result;
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::DialogResult CustomMessageBox::Show(MessageType type, String ^ title, String ^ text,
                                                            String ^ buttonOK, String ^ buttonCancel,
                                                            String ^ buttonOther, String ^ checkbox,
                                                            [Out] bool % checked) {
  logDebug("About to show a custom message box\n");

  mforms::DialogResult result;

  mforms::Utilities::enter_modal_loop();
  PCWSTR mainIcon;
  switch (type) {
    case MessageType::MessageWarning:
      mainIcon = TD_WARNING_ICON;
      break;
    case MessageType::MessageError:
      mainIcon = TD_ERROR_ICON;
      break;
    default:
      mainIcon = TD_INFORMATION_ICON;
      break;
  }
  bool isChecked = false;
  result = ShowInternal(NativeToCppString(title), NativeToCppString(text), mainIcon, NativeToCppString(buttonOK),
                        NativeToCppString(buttonCancel), NativeToCppString(buttonOther), NativeToCppString(checkbox),
                        isChecked);
  checked = isChecked;
  mforms::Utilities::leave_modal_loop();

  System::Windows::Forms::DialogResult native_result;
  switch (result) {
    case mforms::ResultCancel:
      native_result = System::Windows::Forms::DialogResult::Cancel;
      break;
    case mforms::ResultOther:
      native_result = System::Windows::Forms::DialogResult::Ignore;
      break;
    default:
      native_result = System::Windows::Forms::DialogResult::OK;
      break;
  }
  return native_result;
}

//--------------------------------------------------------------------------------------------------

System::Windows::Forms::DialogResult CustomMessageBox::Show(MessageType type, String ^ title, String ^ text,
                                                            String ^ buttonOK) {
  bool checked = false;
  return Show(type, title, text, buttonOK, "", "", "", checked);
}

//----------------- DispatchControl ----------------------------------------------------------------

delegate InvokationResult ^ RunSlotDelegate(SlotWrapper ^ wrapper);

void *DispatchControl::RunOnMainThread(const std::function<void *()> &slot, bool wait) {
  logDebug("Running slot on main thread (%swaiting for it)\n", wait ? "" : "not ");

  if (InvokeRequired) {
    logDebug2("Cross thread invocation required\n");

    array<Object ^> ^ parameters = gcnew array<Object ^>(1);
    parameters[0] = gcnew SlotWrapper(slot);
    if (wait) {
      InvokationResult ^ result =
        (InvokationResult ^)Invoke(gcnew RunSlotDelegate(this, &DispatchControl::RunSlot), parameters);
      return result->Result;
    } else {
      BeginInvoke(gcnew RunSlotDelegate(this, &DispatchControl::RunSlot), parameters);

      // If we don't wait for the result we cannot return it.
      return NULL;
    }
  }

  return slot();
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to run a given cancel_slot in the main thread.
 */
InvokationResult ^ DispatchControl::RunSlot(SlotWrapper ^ wrapper) {
  logDebug2("Running cancel_slot on main thread\n");

  return gcnew InvokationResult((*wrapper->_slot)());
}

//----------------- UtilitiesWrapper ------------------------------------------------------------------

UtilitiesWrapper::UtilitiesWrapper() {
}

//--------------------------------------------------------------------------------------------------

void UtilitiesWrapper::beep() {
  SystemSounds::Beep->Play();
}

//--------------------------------------------------------------------------------------------------

int UtilitiesWrapper::show_message(const std::string &title, const std::string &text, const std::string &ok,
                                   const std::string &cancel, const std::string &other) {
  logDebug("Showing a message to the user\n");

  hide_wait_message();

  bool checked;
  return CustomMessageBox::Show(title, text, TD_INFORMATION_ICON, ok, cancel, other, "", checked);
}

//--------------------------------------------------------------------------------------------------

int UtilitiesWrapper::show_error(const std::string &title, const std::string &text, const std::string &ok,
                                 const std::string &cancel, const std::string &other) {
  logDebug("Showing an error to the user\n");

  hide_wait_message();

  bool checked;
  return CustomMessageBox::Show(title, text, TD_ERROR_ICON, ok, cancel, other, "", checked);
}

//--------------------------------------------------------------------------------------------------

int UtilitiesWrapper::show_warning(const std::string &title, const std::string &text, const std::string &ok,
                                   const std::string &cancel, const std::string &other) {
  logDebug("Showing a warning to the user\n");

  hide_wait_message();

  bool checked;
  return CustomMessageBox::Show(title, text, TD_WARNING_ICON, ok, cancel, other, "", checked);
}

//--------------------------------------------------------------------------------------------------

int UtilitiesWrapper::show_message_with_checkbox(const std::string &title, const std::string &text,
                                                 const std::string &ok, const std::string &cancel,
                                                 const std::string &other, const std::string &checkbox_text,
                                                 bool &isChecked) {
  logDebug("Showing a message with checkbox to the user\n");

  hide_wait_message();

  std::string checkboxText = (checkbox_text.size() > 0) ? checkbox_text : _("Don't show this message again");
  return CustomMessageBox::Show(title, text, TD_INFORMATION_ICON, ok, cancel, other, checkboxText, isChecked);
}

//--------------------------------------------------------------------------------------------------

/**
 * Shows the warning heads-up-display with the given title and text.
 */
void UtilitiesWrapper::show_wait_message(const std::string &title, const std::string &text) {
  logDebug("Showing wait message\n");

  HUDForm::Show(CppStringToNative(title), CppStringToNative(text), true);
}

//--------------------------------------------------------------------------------------------------

/**
 * Hides a previously shown wait message.
 */
bool UtilitiesWrapper::hide_wait_message() {
  logDebug("Hiding the wait message\n");

  bool result = HUDForm::IsVisible;
  if (result) {
    logDebug2("Wait message was visible, finishing it\n");
    HUDForm::Finish();
  } else
    logDebug2("Wait message was not visible, nothing to do\n");

  return result;
}

//--------------------------------------------------------------------------------------------------

ref class CallSlotDelegate {
private:
  const std::function<void()> *task_slot;
  const std::function<bool()> *cancel_slot;

public:
  CallSlotDelegate(const std::function<void()> *start, const std::function<bool()> *cancel)
    : task_slot(start), cancel_slot(cancel) {
  }

  void call_start() {
    if (*task_slot)
      (*task_slot)();
  }

  bool call_cancel() {
    if (*cancel_slot)
      return (*cancel_slot)();
    return true;
  }
};

//-------------------------------------------------------------------------------------------------

bool UtilitiesWrapper::run_cancelable_wait_message(const std::string &title, const std::string &text,
                                                   const std::function<void()> &signal_ready,
                                                   const std::function<bool()> &cancel_slot) {
  logDebug("Running a cancelable wait message\n");

  CallSlotDelegate ^ caller = gcnew CallSlotDelegate(&signal_ready, &cancel_slot);

  HUDForm::ReadyDelegate ^ readyDelegate = gcnew HUDForm::ReadyDelegate(caller, &CallSlotDelegate::call_start);
  HUDForm::CancelDelegate ^ cancelDelegate = gcnew HUDForm::CancelDelegate(caller, &CallSlotDelegate::call_cancel);

  logDebug2("Running the HUD window\n");
  System::Windows::Forms::DialogResult result =
    HUDForm::ShowModal(CppStringToNative(title), CppStringToNative(text), true, readyDelegate, cancelDelegate);
  logDebug2("HUD window returned with code: %i\n", result);

  // Abort is used if the window was forcibly closed (e.g. when showing another dialog, like password query).
  return (result == System::Windows::Forms::DialogResult::OK) ||
         (result == System::Windows::Forms::DialogResult::Abort);
}

//--------------------------------------------------------------------------------------------------

/**
 * Signals the operation being described in a previous run_cancelable_wait_message() call has
 * finished and the message panel should be taken down.
 */
void UtilitiesWrapper::stop_cancelable_wait_message() {
  logDebug("Explicit cancelation of the wait message\n");

  HUDForm::Finish();
}

//--------------------------------------------------------------------------------------------------

/**
 * Places the given string on the clipboard.
 *
 * @param content The text to be placed on the clipboard. It is assume its encoding is UTF-8.
 */
void UtilitiesWrapper::set_clipboard_text(const std::string &content) {
  logDebug("Setting clipboard text\n");

  if (!content.empty()) {
    String ^ text = CppStringToNative(content);
    try {
      Clipboard::Clear();

      // If the clipboard cannot be opened because another application uses it at this very moment
      // we try a second time with built-in retries. If that still fails we inform the user.
      Clipboard::SetText(text);

      logDebug2("Successful\n");
    } catch (ExternalException ^) {
      try {
        logDebug2("Got exception, second attempt\n");
        Clipboard::SetDataObject(text, false, 5, 200);
        logDebug2("Successful\n");
      } catch (ExternalException ^ e) {
        String ^ msg = e->Message + Environment::NewLine + Environment::NewLine;
        msg += "Caused by:" + Environment::NewLine;

        HWND window = GetOpenClipboardWindow();
        TCHAR window_text[501];
        GetWindowText(window, window_text, 500);
        String ^ converted_text = gcnew String(window_text);
        msg += converted_text;
        logDebug2("Error while setting the text: %s\n", NativeToCppString(msg).c_str());

        CustomMessageBox::Show(MessageType::MessageError, _("Error while copying to clipboard"), msg, "Close");
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the current text on the clipboard, if there is any.
 *
 * @result If there is text on the clipboard (ANSI or Unicode) it is returned as UTF-8 string.
 * @note The returned text gets all CRLF Windows line breaks converted to pure LF.
 */
std::string UtilitiesWrapper::get_clipboard_text() {
  logDebug("Reading clipboard text\n");

  String ^ unicode = (String ^)Clipboard::GetData(DataFormats::UnicodeText);
  if (unicode == nullptr)
    return "";

  return NativeToCppString(unicode);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns platform specific user folders, e.g. for the desktop, the user's documents etc.
 */
std::string UtilitiesWrapper::get_special_folder(mforms::FolderType type) {
  logDebug("Get special folder\n");

  Environment::SpecialFolder special_folder;
  switch (type) {
    case mforms::Desktop:
      special_folder = Environment::SpecialFolder::DesktopDirectory;
      break;
    case mforms::ApplicationData:
      special_folder = Environment::SpecialFolder::ApplicationData;
      break;
    case mforms::WinProgramFiles:
      special_folder = Environment::SpecialFolder::ProgramFiles;
      break;
    case mforms::WinProgramFilesX86:
      special_folder = Environment::SpecialFolder::ProgramFilesX86;
      break;
    case mforms::ApplicationSettings:
      special_folder = Environment::SpecialFolder::ApplicationData;
      break;
    default: // Documents
      special_folder = Environment::SpecialFolder::MyDocuments;
      break;
  };

  // Getting the 64 bit application folder works differently if we are a 32 bit application.
  // IntPtr has a size of 8 for 64 bit apps.
  if (IntPtr::Size == 4 && type == mforms::WinProgramFiles) {
    logDebug2("Getting the 64bit program path\n");

    WCHAR folder[MAX_PATH];
    ExpandEnvironmentStrings(L"%ProgramW6432%", folder, ARRAYSIZE(folder));
    return base::wstring_to_string(folder);
  }

  // ApplicationSettings is not a predefined folder, but our own application data folder.
  if (type == mforms::ApplicationSettings) {
    std::string folder = NativeToCppString(Environment::GetFolderPath(special_folder));
    return folder + "/MySQL/Workbench";
  }

  return NativeToCppString(Environment::GetFolderPath(special_folder));
}

//--------------------------------------------------------------------------------------------------

void UtilitiesWrapper::open_url(const std::string &url) {
  try {
    logDebug("Opening the URL: %s\n", url.c_str());

    System::Diagnostics::Process::Start(CppStringToNative(url));
  } catch (Exception ^ e) {
    String ^ message = e->Message->ToString();
    logDebug("Error opening the url: %s\n", NativeToCppString(message).c_str());

    MessageBox::Show(message, "Error Opening Browser", MessageBoxButtons::OK, MessageBoxIcon::Error,
                     MessageBoxDefaultButton::Button1);
  }
}

//--------------------------------------------------------------------------------------------------

bool UtilitiesWrapper::move_to_trash(const std::string &file_name) {
  logDebug("Moving file to trash: %s\n", file_name.c_str());

  SHFILEOPSTRUCT shf = {0};

  shf.wFunc = FO_DELETE;
  shf.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;

  std::wstring converted_filename = base::string_to_wstring(file_name);
  shf.pFrom = converted_filename.c_str();
  int result = SHFileOperation(&shf);

  return (result == 0) && !shf.fAnyOperationsAborted;
}

//--------------------------------------------------------------------------------------------------

void MySQL::Forms::UtilitiesWrapper::reveal_file(const std::string &path) {
  std::wstring native_path = base::string_to_wstring(path);
  PIDLIST_ABSOLUTE pidl = ILCreateFromPath(native_path.c_str());
  if (pidl != NULL) {
    if (SHOpenFolderAndSelectItems(pidl, 0, 0, 0) != S_OK)
      logDebug("Could not open Explorer with the path %s\n", path.c_str());

    ILFree(pidl);
  }
}

//--------------------------------------------------------------------------------------------------

// The password cache is a temporary storage and only used for a short time frame when looking up a password.
static base::Mutex password_mutex;
static std::map<std::string, std::string> password_cache;
typedef std::map<std::string, std::string>::iterator PasswordIterator;

#define DOMAIN_SEPARATOR (char)2
#define PASSWORD_SEPARATOR (char)3

/**
 * Loads encrypted passwords from disk. These are typically held only for a short moment.
 */
void UtilitiesWrapper::load_passwords() {
  logDebug("Loading password cache\n");

  // Load password cache from disk. Don't throw an error if the cache file doesn't exist yet, though.
  std::string file = get_special_folder(mforms::ApplicationData) + "/MySQL/Workbench/workbench_user_data.dat";
  std::string user_info =
    _(std::string("If loading the passwords fails repeatedly you should clear the vault. This will remove all ") +
      "passwords, hence you have to enter them again.");
  if (g_file_test(file.c_str(), G_FILE_TEST_EXISTS)) {
    gchar *content;
    gsize length;
    GError *error = NULL;
    bool result = g_file_get_contents(file.c_str(), &content, &length, &error) == TRUE;
    if (!result) {
      logError("Error loading password data: %s\n", error->message);
      int result = show_error("Password management error",
                              "Error while loading passwords: " + std::string(error->message) + "\n\n" + user_info,
                              _("Ignore"), "", _("Clear Vault"));
      if (result == mforms::ResultOther)
        base::remove(file);
      return;
    }

    DATA_BLOB data_in;
    DATA_BLOB data_out;

    data_in.pbData = (BYTE *)content;
    data_in.cbData = (DWORD)length;

    logDebug2("Decrypting password data\n");
    result = CryptUnprotectData(&data_in, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &data_out) == TRUE;

    g_free(content);

    if (!result) {
      logError("Could not decrypt password data\n");
      int result = show_error("Password management error", "Could not decrypt password cache.\n\n" + user_info,
                              _("Ignore"), "", _("Clear Vault"));
      if (result == mforms::ResultOther)
        base::remove(file);
      return;
    }

    try {
      logDebug2("Filling password cache\n");

      // Split the string into individual items and fill the password cache with them.
      std::stringstream ss((char *)data_out.pbData);
      std::string item;
      while (std::getline(ss, item, '\n')) {
        std::string::size_type pos = item.find_first_of(PASSWORD_SEPARATOR, 0);

        if (std::string::npos != pos)
          password_cache[item.substr(0, pos)] = item.substr(pos + 1, item.length());
      }
    } finally {
      LocalFree(data_out.pbData);
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Saves the password cache to disk (if store is true) and clears it so passwords aren't kept in
 * memory any longer than necessary.
 */
void UtilitiesWrapper::unload_passwords(bool store) {
  logDebug("Unloading password cache\n");

  // Store all passwords in a string for encryption.
  try {
    if (store) {
      logDebug("Storing password data\n");

      std::string plain_data;
      for (PasswordIterator iterator = password_cache.begin(); iterator != password_cache.end(); iterator++)
        plain_data += iterator->first + PASSWORD_SEPARATOR + iterator->second + "\n";

      DATA_BLOB data_in;
      DATA_BLOB data_out;

      data_in.pbData = (BYTE *)plain_data.c_str();
      data_in.cbData = (DWORD)plain_data.length() + 1;

      if (!CryptProtectData(&data_in, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &data_out)) {
        logError("Error encrypting password data\n");
        show_error("Password management error", "Could not encrypt password cache.", _("Close"), "", "");
        return;
      }

      // Now write the encrypted data to file.
      std::string file = get_special_folder(mforms::ApplicationData) + "/MySQL/Workbench/workbench_user_data.dat";
      GError *error = NULL;
      bool result = g_file_set_contents(file.c_str(), (gchar *)data_out.pbData, data_out.cbData, &error) == TRUE;

      LocalFree(data_out.pbData);

      if (!result) {
        logError("Error storing password data: %s\n", error->message);
        show_error("Password management error", "Error while storing passwords: " + std::string(error->message),
                   _("Close"), "", "");
      }
    }
  } finally {
    password_cache.clear();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * This function stores the given password in our password file, parameterized by the given service
 * and user name. The file is encrypted by the system for safety and can only be decrypted by the
 * same user who encrypted it.
 */
void UtilitiesWrapper::store_password(const std::string &service, const std::string &account,
                                      const std::string &password) {
  base::MutexLock lock(password_mutex);
  load_passwords();

  // Keep the password in our password cache and write the entire cache to file after that.
  password_cache[service + DOMAIN_SEPARATOR + account] = password;

  unload_passwords(true);
}

//--------------------------------------------------------------------------------------------------

/**
 * Return the plain text password for the given service and account.
 */
bool UtilitiesWrapper::find_password(const std::string &service, const std::string &account, std::string &password) {
  logDebug("Looking up password for service: %s, account: %s\n", service.c_str(), account.c_str());

  base::MutexLock lock(password_mutex);
  load_passwords();

  bool result = true;
  PasswordIterator iterator = password_cache.find(service + DOMAIN_SEPARATOR + account);
  if (iterator == password_cache.end())
    result = false;
  else
    password = iterator->second;

  unload_passwords(false);
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Remove the password for the given service and account if there is one.
 */
void UtilitiesWrapper::forget_password(const std::string &service, const std::string &account) {
  base::MutexLock lock(password_mutex);

  load_passwords();

  PasswordIterator iterator = password_cache.find(service + DOMAIN_SEPARATOR + account);
  if (iterator != password_cache.end()) {
    password_cache.erase(iterator);
    unload_passwords(true);
  } else
    unload_passwords(false);
}

//--------------------------------------------------------------------------------------------------

void *UtilitiesWrapper::perform_from_main_thread(const std::function<void *()> &slot, bool wait) {
  return dispatcher->RunOnMainThread(slot, wait);
}

//--------------------------------------------------------------------------------------------------
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
  DWORD dwType;     // Must be 0x1000.
  LPCSTR szName;    // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(DWORD dwThreadID, const char *threadName) {
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;

  __try {
    // Doesn't work at all outside the IDE. Crashes WB on start.
    // RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
  } __except (EXCEPTION_CONTINUE_EXECUTION) {
  }
}

void UtilitiesWrapper::set_thread_name(const std::string &name) {
#ifdef _DEBUG
  SetThreadName(-1, name.c_str());
#endif
}

//--------------------------------------------------------------------------------------------------

gcroot<Font ^> UtilitiesWrapper::last_font;
static std::string last_font_description;

double UtilitiesWrapper::get_text_width(const std::string &text, const std::string &font) {
  // We cache the last font we have used for the text width as it this computation is likely to be
  // done multiple times for the same font.
  if (last_font_description != font) {
    last_font_description = font;

    std::string font_name;
    float size;
    bool bold;
    bool italic;
    base::parse_font_description(font, font_name, size, bold, italic);

    Drawing::FontStyle style = FontStyle::Regular;
    if (bold)
      style = (FontStyle)(style | FontStyle::Bold);
    if (italic)
      style = (FontStyle)(style | FontStyle::Italic);

    last_font = ControlUtilities::GetFont(CppStringToNativeRaw(font_name), size, style);
  }

  Size size = TextRenderer::MeasureText(CppStringToNative(text), last_font);
  return size.Width;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the main form of the application.
 */
System::Windows::Forms::Form ^ UtilitiesWrapper::get_mainform() {
  logDebug2("Returning main form\n");

  return Application::OpenForms["MainForm"];
}

//--------------------------------------------------------------------------------------------------

static base::RecMutex timeout_mutex;

ref class TimerHandler {
  static mforms::TimeoutHandle last_timeout = 0;
  static Dictionary<mforms::TimeoutHandle, TimerHandler ^> ^ timeout_handles =
    gcnew Dictionary<mforms::TimeoutHandle, TimerHandler ^>();

public:
  TimerHandler(float interval, const std::function<bool()> &slot) {
    logDebug("Creating new TimerHandler\n");

    _timer = gcnew System::Windows::Forms::Timer();

    _slot = new std::function<bool()>(slot);

    _timer->Interval = (int)(interval * 1000);
    _timer->Tick += gcnew EventHandler(this, &TimerHandler::timer_tick);

    base::RecMutexLock lock(timeout_mutex);
    _handle = ++last_timeout;
    timeout_handles[_handle] = this;

    _timer->Start();
  }

  ~TimerHandler() {
    logDebug("Destructing TimerHandler instance\n");

    _timer->Stop();
    delete _timer;
    delete _slot;
  }

  static void cancel(mforms::TimeoutHandle handle) {
    base::RecMutexLock lock(timeout_mutex);
    if (timeout_handles->ContainsKey(handle)) {
      auto timerHandle = timeout_handles[handle];
      timeout_handles->Remove(handle);
      delete timerHandle;
    }
  }

  mforms::TimeoutHandle handle() {
    return _handle;
  }

private:
  std::function<bool()> *_slot;
  System::Windows::Forms::Timer ^ _timer;
  mforms::TimeoutHandle _handle;

  void timer_tick(Object ^ sender, System::EventArgs ^ e) {
    //    log_debug3("Timer tick\n");

    // Emulate behavior in MacOS X (timers aren't fired when in modal loops).
    // Also works around a deadlock of python when a timer is fired while inside a modal loop.
    if (!mforms::Utilities::in_modal_loop()) {
      _timer->Stop();
      // if callback returns true, then restart the timer
      if ((*_slot)())
        _timer->Enabled = true;
      else {
        {
          base::RecMutexLock lock(timeout_mutex);
          if (timeout_handles->ContainsKey(_handle)) {
            auto timerHandle = timeout_handles[_handle];
            timeout_handles->Remove(_handle);
            delete timerHandle;
          }
        }
      }
    }
  }
};

//-------------------------------------------------------------------------------------------------

mforms::TimeoutHandle UtilitiesWrapper::add_timeout(float interval, const std::function<bool()> &slot) {
  logDebug("Adding new timeout\n");

  TimerHandler ^ handler = gcnew TimerHandler(interval, slot);
  return handler->handle();
}

//-------------------------------------------------------------------------------------------------

void UtilitiesWrapper::cancel_timeout(mforms::TimeoutHandle h) {
  TimerHandler::cancel(h);
}

//-------------------------------------------------------------------------------------------------

gcroot<DispatchControl ^> UtilitiesWrapper::dispatcher;

void UtilitiesWrapper::init() {
  dispatcher = gcnew DispatchControl();
  dispatcher->Handle; // create window handle

  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_utilities_impl.beep = &UtilitiesWrapper::beep;
  f->_utilities_impl.show_message = &UtilitiesWrapper::show_message;
  f->_utilities_impl.show_error = &UtilitiesWrapper::show_error;
  f->_utilities_impl.show_warning = &UtilitiesWrapper::show_warning;
  f->_utilities_impl.show_message_with_checkbox = &UtilitiesWrapper::show_message_with_checkbox;
  f->_utilities_impl.show_wait_message = &UtilitiesWrapper::show_wait_message;
  f->_utilities_impl.hide_wait_message = &UtilitiesWrapper::hide_wait_message;
  f->_utilities_impl.run_cancelable_wait_message = &UtilitiesWrapper::run_cancelable_wait_message;
  f->_utilities_impl.stop_cancelable_wait_message = &UtilitiesWrapper::stop_cancelable_wait_message;

  f->_utilities_impl.set_clipboard_text = &UtilitiesWrapper::set_clipboard_text;
  f->_utilities_impl.get_clipboard_text = &UtilitiesWrapper::get_clipboard_text;

  f->_utilities_impl.open_url = &UtilitiesWrapper::open_url;
  f->_utilities_impl.move_to_trash = &UtilitiesWrapper::move_to_trash;
  f->_utilities_impl.add_timeout = &UtilitiesWrapper::add_timeout;
  f->_utilities_impl.cancel_timeout = &UtilitiesWrapper::cancel_timeout;
  f->_utilities_impl.get_special_folder = &UtilitiesWrapper::get_special_folder;
  f->_utilities_impl.store_password = &UtilitiesWrapper::store_password;
  f->_utilities_impl.find_password = &UtilitiesWrapper::find_password;
  f->_utilities_impl.forget_password = &UtilitiesWrapper::forget_password;
  f->_utilities_impl.perform_from_main_thread = &UtilitiesWrapper::perform_from_main_thread;
  f->_utilities_impl.reveal_file = &UtilitiesWrapper::reveal_file;
  f->_utilities_impl.set_thread_name = &UtilitiesWrapper::set_thread_name;

  f->_utilities_impl.get_text_width = &UtilitiesWrapper::get_text_width;
}

//-------------------------------------------------------------------------------------------------
