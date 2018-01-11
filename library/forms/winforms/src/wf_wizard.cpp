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
#include "wf_form.h"
#include "wf_wizard.h"

#include "base/log.h"

using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

//----------------- BufferedPanel ------------------------------------------------------------------

ref class BufferedPanel : public FlowLayoutPanel {
public:
  BufferedPanel() : FlowLayoutPanel() {
    DoubleBuffered = true;
  }
};

//----------------- WizardForm ---------------------------------------------------------------------

ref class MySQL::Forms::WizardForm : public Form {
public:
  FlowLayoutPanel ^ sidebar;
  Label ^ title;
  Button ^ extraButton;
  TableLayoutPanel ^ footer;
  FlowLayoutPanel ^ buttonBar;
  Button ^ backButton;
  Button ^ nextButton;
  Button ^ cancelButton;
  Panel ^ content; // This is where the actual wizard content is docked to.
  Image ^ sidebarOverlay;

  mforms::Wizard *backend;

  //------------------------------------------------------------------------------------------------

  WizardForm() : Form() {
  }

  //------------------------------------------------------------------------------------------------

  void SetupLayout() {
    sidebar = gcnew BufferedPanel;
    title = gcnew Label;
    extraButton = gcnew Button;
    footer = gcnew TableLayoutPanel;
    buttonBar = gcnew FlowLayoutPanel;
    backButton = gcnew Button;
    nextButton = gcnew Button;
    cancelButton = gcnew Button;
    content = gcnew Panel;

    std::string icon_path = mforms::App::get()->get_resource_path("wb-wizard-vista-bg.png");
    String ^ native_path = gcnew String(icon_path.c_str());
    if (File::Exists(native_path))
      sidebarOverlay = Image::FromFile(native_path);
    else
      sidebarOverlay = nullptr;

    BackColor = Color::White;
    FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
    MaximizeBox = false;
    MinimizeBox = false;
    StartPosition = FormStartPosition::CenterScreen;
    CancelButton = cancelButton;
    Icon = gcnew Drawing::Icon("images/icons/MySQLWorkbench.ico", Drawing::Size(16, 16));

    // The footer with buttons.
    Controls->Add(footer);
    footer->RowCount = 1;
    footer->ColumnCount = 2;
    footer->AutoSize = true;
    footer->Dock = DockStyle::Bottom;
    footer->Padding = System::Windows::Forms::Padding(10);
    footer->Controls->Add(extraButton, 0, 0);

    buttonBar->FlowDirection = FlowDirection::RightToLeft;
    buttonBar->Controls->Add(cancelButton);
    buttonBar->Controls->Add(nextButton);
    buttonBar->Controls->Add(backButton);
    buttonBar->Dock = DockStyle::Right;
    buttonBar->AutoSize = true;
    buttonBar->Margin = System::Windows::Forms::Padding(0);
    footer->Controls->Add(buttonBar, 1, 0);

    // We need to keep a reference to the wizard in the buttons to be able
    // to notify it when the buttons are clicked.
    backButton->Text = "&Back";
    backButton->UseMnemonic = true;
    backButton->Click += gcnew EventHandler(this, &WizardForm::BackClick);
    backButton->FlatStyle = FlatStyle::System;

    nextButton->Text = "&Next";
    nextButton->UseMnemonic = true;
    nextButton->Click += gcnew EventHandler(this, &WizardForm::NextClick);
    nextButton->FlatStyle = FlatStyle::System;

    cancelButton->Text = "Cancel";
    cancelButton->Click += gcnew EventHandler(this, &WizardForm::CancelClick);
    cancelButton->FlatStyle = FlatStyle::System;

    extraButton->Text = "&Advanced";
    extraButton->UseMnemonic = true;
    extraButton->Click += gcnew EventHandler(this, &WizardForm::ExtraClick);
    extraButton->FlatStyle = FlatStyle::System;

    // The side bar with background painting and list of strings.
    Controls->Add(sidebar);
    sidebar->Dock = DockStyle::Left;
    sidebar->ForeColor = Color::White;
    sidebar->Paint += gcnew PaintEventHandler(this, &WizardForm::SidebarPaint);

    // Title
    Controls->Add(title);
    title->AutoSize = false;
    try {
      title->Font = gcnew Drawing::Font("Tahoma", 12.0, FontStyle::Bold, GraphicsUnit::Pixel);
    } catch (System::ArgumentException ^ e) {
      // Argument exception pops up when the system cannot find the Regular font style (corrupt font).
      logError("WizardWrapper::c_tor setting title font failed. %s\n", e->Message);
    }

    title->ForeColor = ColorTranslator::FromHtml("#003392");
    title->Location = Point(sidebar->Width, 0);
    title->Padding = System::Windows::Forms::Padding(15);

    // Other initialization.
    Controls->Add(content);

    // Resize as last action. Triggers layout of the elements.
    Size = Drawing::Size(800, 600);
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Used to draw a gradient background for the side bar.
   *
   * @param sender The control for which the paint event is triggered (the side bar).
   * @param arguments Additional data needed to paint the background.
   */
  void SidebarPaint(System::Object ^ sender, PaintEventArgs ^ args) {
    Graphics ^ g = args->Graphics;

    LinearGradientBrush gradientBrush(Point(10, 0), Point(10, sidebar->Height), Color::FromArgb(255, 66, 111, 166),
                                      Color::FromArgb(255, 103, 186, 104));

    g->FillRectangle(% gradientBrush, 0, 0, sidebar->Width, sidebar->Height);
    if (sidebarOverlay != nullptr)
      g->DrawImage(sidebarOverlay, 0, sidebar->Height - sidebarOverlay->Height);
  }

  //------------------------------------------------------------------------------------------------

  void NextClick(System::Object ^ sender, EventArgs ^ args) {
    backend->next_clicked();
  }

  //------------------------------------------------------------------------------------------------

  void BackClick(System::Object ^ sender, System::EventArgs ^ args) {
    backend->back_clicked();
  }

  //------------------------------------------------------------------------------------------------

  void CancelClick(System::Object ^ sender, EventArgs ^ args) {
    if (backend->_cancel_slot())
      //((Form ^) btn->TopLevelControl)->Close();
      Close();
  }

  //------------------------------------------------------------------------------------------------

  void ExtraClick(System::Object ^ sender, EventArgs ^ args) {
    backend->extra_clicked();
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnResize(EventArgs ^ args) override {
    __super ::OnResize(args);

    if (sidebar == nullptr)
      return;

    System::Drawing::Rectangle area = DisplayRectangle;
    title->Size = Drawing::Size(area.Width - sidebar->Width, 50);
    title->Location = Point(sidebar->Width, 0);

    content->Size = Drawing::Size(area.Width - sidebar->Width, area.Height - footer->Height - title->Height);
    content->Location = Point(sidebar->Width, title->Height);
  }

  //------------------------------------------------------------------------------------------------

  void SetContent(Control ^ control) {
    // Remove old stuff if there is some.
    if (content->Controls->Count > 0) {
      content->Controls[0]->Dock = DockStyle::None;
      content->Controls->Clear();
    }

    if (control != nullptr) {
      content->Controls->Add(control);
      control->Dock = DockStyle::Fill;

      // Focus the new page to allow mnemonics to works consistently.
      control->Focus();
    }
  }

  //------------------------------------------------------------------------------------------------

  void SetStepList(const std::vector<std::string> &steps) {
    SuspendLayout();
    try {
      sidebar->Controls->Clear();

      for each(std::string entry in steps) {
          Label ^ label = gcnew Label();
          label->Text = CppStringToNative(entry.substr(1));
          FontStyle style = FontStyle::Regular;
          switch (entry[0]) {
            case '*': // current task
              label->ForeColor = Color::White;
              style = FontStyle::Bold;
              break;

            case '.': // executed task
              label->ForeColor = Color::White;
              break;

            case '-': // open task
              label->ForeColor = Color::FromArgb(255, 192, 192, 192);
              break;
          }

          try {
            label->Font = gcnew Drawing::Font("Tahoma", 11.0, style, GraphicsUnit::Pixel);
          } catch (System::ArgumentException ^ e) {
            // Argument exception pops up when the system cannot find the Regular font style (corrupt font).
            logError("WizardWrapper::set_step_list setting label font failed. %s\n", e->Message);
          }

          label->BackColor = Drawing::Color::Transparent;
          label->Padding = System::Windows::Forms::Padding(7);
          label->Size = Drawing::Size(sidebar->Width, label->PreferredHeight);
          sidebar->Controls->Add(label);
        }
    } finally {
      ResumeLayout();
    }
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- WizardWrapper ------------------------------------------------------------------

WizardWrapper::WizardWrapper(mforms::Wizard *backend, mforms::Form *owner)
  : FormWrapper(backend, owner, mforms::FormDialogFrame) {
}

//--------------------------------------------------------------------------------------------------

bool WizardWrapper::create(mforms::Wizard *backend, mforms::Form *owner) {
  WizardWrapper *wrapper = new WizardWrapper(backend, owner);
  WizardForm ^ form = WizardWrapper::Create<WizardForm>(backend, wrapper);
  form->backend = backend;
  form->SetupLayout();

  return true;
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_title(mforms::Wizard *backend, const std::string &title) {
  Form ^ form = WizardWrapper::GetManagedObject<Form>(backend);
  form->Text = CppStringToNative(title);
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::run_modal(mforms::Wizard *backend) {
  WizardWrapper *wrapper = backend->get_data<WizardWrapper>();
  wrapper->GetManagedObject<Form>()->ShowDialog(wrapper->_owner);
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::close(mforms::Wizard *backend) {
  Form ^ form = WizardWrapper::GetManagedObject<Form>(backend);
  form->Close();
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_content(mforms::Wizard *backend, mforms::View *view) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  if (view == NULL)
    form->SetContent(nullptr);
  else {
    ViewWrapper *wrapper = view->get_data<ViewWrapper>();
    wrapper->set_resize_mode(AutoResizeMode::ResizeNone);
    form->SetContent(wrapper->GetControl());
  }
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_heading(mforms::Wizard *backend, const std::string &heading) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  form->title->Text = CppStringToNative(heading);
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_step_list(mforms::Wizard *backend, const std::vector<std::string> &steps) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  form->SetStepList(steps);
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_allow_cancel(mforms::Wizard *backend, bool flag) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  form->cancelButton->Enabled = flag;
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_allow_back(mforms::Wizard *backend, bool flag) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  form->backButton->Enabled = flag;
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_allow_next(mforms::Wizard *backend, bool flag) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  form->nextButton->Enabled = flag;
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_show_extra(mforms::Wizard *backend, bool flag) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  form->extraButton->Visible = flag;
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_extra_caption(mforms::Wizard *backend, const std::string &caption) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  form->extraButton->Text = CppStringToNative(caption);
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::set_next_caption(mforms::Wizard *backend, const std::string &caption) {
  WizardForm ^ form = WizardWrapper::GetManagedObject<WizardForm>(backend);
  if (caption.empty())
    form->nextButton->Text = "&Next";
  else
    form->nextButton->Text = CppStringToNative(caption)->Replace("_", "&");
}

//--------------------------------------------------------------------------------------------------

void WizardWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_wizard_impl.create = &WizardWrapper::create;
  f->_wizard_impl.set_title = &WizardWrapper::set_title;
  f->_wizard_impl.run_modal = &WizardWrapper::run_modal;
  f->_wizard_impl.close = &WizardWrapper::close;
  f->_wizard_impl.set_content = &WizardWrapper::set_content;
  f->_wizard_impl.set_heading = &WizardWrapper::set_heading;
  f->_wizard_impl.set_step_list = &WizardWrapper::set_step_list;
  f->_wizard_impl.set_allow_cancel = &WizardWrapper::set_allow_cancel;
  f->_wizard_impl.set_allow_back = &WizardWrapper::set_allow_back;
  f->_wizard_impl.set_allow_next = &WizardWrapper::set_allow_next;
  f->_wizard_impl.set_show_extra = &WizardWrapper::set_show_extra;
  f->_wizard_impl.set_extra_caption = &WizardWrapper::set_extra_caption;
  f->_wizard_impl.set_next_caption = &WizardWrapper::set_next_caption;
}

//--------------------------------------------------------------------------------------------------
