/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_box.h"
#include "wf_utilities.h"

using namespace System;
using namespace System::IO;
using namespace Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;

//----------------- FillForm -----------------------------------------------------------------------

ref class FillForm : public System::Windows::Forms::Form {
private:
  FormFillLayout ^ layoutEngine;

public:
  FormWrapper *wrapper;
  delegate System::Windows::Forms::DialogResult ShowModalDelegate(System::Windows::Forms::IWin32Window ^ parent);

  //------------------------------------------------------------------------------------------------

  /**
   * Computes the entire layout of the form.
   *
   * @param proposedSize The size to start from layouting. Since super ordinated controls may impose
   *                     a layout size we need to honor that (especially important for auto wrapping
   *                     labels).
   * @param resizeChildren Tells the function whether the computed client control bounds should be applied
   *                      (when doing a relayout) or not (when computing the preferred size).
   * @return The resulting size of the table.
   */
  System::Drawing::Size FillForm::ComputeLayout(System::Drawing::Size proposedSize, bool resizeChildren) {
    // This layout is actually very simple. Resize the first (and only) child control so that
    // it fills the entire client area of the container. If enabled resize the container to fit the
    // (preferred) size of the content.

    // Exclude any space needed to draw decoration (e.g. border) from layout processing.
    System::Drawing::Rectangle inner = DisplayRectangle;
    System::Drawing::Size current_size = Size;
    int horizontal_padding = current_size.Width - inner.Width;
    int vertical_padding = current_size.Height - inner.Height;

    if (Controls->Count > 0) {
      Control ^ content = Controls[0];

      // Compute the proposed size for the content out of the size for the form.
      proposedSize.Width -= horizontal_padding;
      proposedSize.Height -= vertical_padding;

      ViewWrapper::set_full_auto_resize(content);
      System::Drawing::Size contentSize = content->GetPreferredSize(proposedSize);

      if (ViewWrapper::use_min_width_for_layout(content))
        contentSize.Width = content->MinimumSize.Width;
      if (ViewWrapper::use_min_height_for_layout(content))
        contentSize.Height = content->MinimumSize.Height;

      // Adjust width of the container if it is too small or auto resizing is enabled.
      // Consider minimum sizes.
      bool auto_resize = ViewWrapper::can_auto_resize_horizontally(this);
      if (proposedSize.Width < contentSize.Width || auto_resize) {
        proposedSize.Width = contentSize.Width;
        if (proposedSize.Width < MinimumSize.Width - horizontal_padding)
          proposedSize.Width = MinimumSize.Width - horizontal_padding;
      }

      // Adjust height of the container if it is too small or auto resizing is enabled.
      auto_resize = ViewWrapper::can_auto_resize_vertically(this);
      if (proposedSize.Height < contentSize.Height || auto_resize) {
        proposedSize.Height = contentSize.Height;
        if (proposedSize.Height < MinimumSize.Height - vertical_padding)
          proposedSize.Height = MinimumSize.Height - vertical_padding;
      }

      if (resizeChildren) {
        // Now stretch the client control to fill the entire display area.
        ViewWrapper::remove_auto_resize(content, AutoResizeMode::ResizeBoth);
        content->Bounds = System::Drawing::Rectangle(inner.Location, proposedSize);
      }

      // Convert resulting content size back to overall form size.
      proposedSize.Width += horizontal_padding;
      proposedSize.Height += vertical_padding;
    }

    return proposedSize;
  }

  //------------------------------------------------------------------------------------------------

  virtual System::Drawing::Size FillForm::GetPreferredSize(System::Drawing::Size proposedSize) override {
    return layoutEngine->GetPreferredSize(this, proposedSize);
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Called by the OS if the form is closed by the user (e.g. via red cross close button).
   */
  virtual void OnFormClosing(FormClosingEventArgs ^ args) override {
    __super ::OnFormClosing(args);

    mforms::Form *backend = FormWrapper::GetBackend<mforms::Form>(this);
    if (!backend->can_close())
      args->Cancel = true;
    else {
      if (wrapper->hide_on_close()) {
        args->Cancel = true;
        Hide();
      }

      backend->was_closed();
    }

    // Do nothing if hiding is not requested. In this case Windows will dispose of the form implicitly.
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnActivated(EventArgs ^ args) override {
    __super ::OnActivated(args);

    mforms::Form *backend = FormWrapper::GetBackend<mforms::Form>(this);
    backend->activated();
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnDeactivate(EventArgs ^ args) override {
    __super ::OnDeactivate(args);

    mforms::Form *backend = FormWrapper::GetBackend<mforms::Form>(this);
    backend->deactivated();
  }

  //------------------------------------------------------------------------------------------------

  virtual property System::Windows::Forms::Layout::LayoutEngine ^ LayoutEngine {
    System::Windows::Forms::Layout::LayoutEngine ^ get() override {
      if (layoutEngine == nullptr)
        layoutEngine = gcnew FormFillLayout();

      return layoutEngine;
    }
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- FormFillLayout -----------------------------------------------------------------

bool FormFillLayout::Layout(Object ^ container, LayoutEventArgs ^ arguments) {
  // Not using the can_layout check here, as a form is a bit special.
  String ^ reason = arguments->AffectedProperty;
  if (reason != "Bounds" && reason != "Padding" && reason != "Visible")
    return false;

  FillForm ^ form = (FillForm ^)container;
  if (!form->IsHandleCreated)
    return false;

  mforms::View *backend = FormWrapper::GetBackend<mforms::View>(form);
  if (backend->is_destroying())
    return false;

  if (form->FormBorderStyle != FormBorderStyle::Sizable)
    ViewWrapper::set_full_auto_resize(form);
  ViewWrapper::adjust_auto_resize_from_docking(form);
  System::Drawing::Size newSize = form->ComputeLayout(form->Size, true);

  if (newSize.Width < form->MinimumSize.Width)
    newSize.Width = form->MinimumSize.Width;
  if (newSize.Height < form->MinimumSize.Height)
    newSize.Height = form->MinimumSize.Height;

  // Finally adjust the container.
  bool parentLayoutNeeded = !form->Size.Equals(newSize);
  if (parentLayoutNeeded)
    ViewWrapper::resize_with_docking(form, newSize);

  ViewWrapper::remove_auto_resize(form, AutoResizeMode::ResizeBoth);

  return parentLayoutNeeded;
}

//--------------------------------------------------------------------------------------------------

System::Drawing::Size FormFillLayout::GetPreferredSize(Control ^ container, System::Drawing::Size proposedSize) {
  FillForm ^ form = (FillForm ^)container;
  return form->ComputeLayout(proposedSize, false);
}

//----------------- FormWrapper --------------------------------------------------------------------

FormWrapper::FormWrapper(mforms::Form *form, mforms::Form *aOwner, mforms::FormFlag flag) : ViewWrapper(form) {
  if (aOwner != NULL) {
    // Meant is the window parent here, not the real owner.
    if (aOwner == mforms::Form::main_form())
      _owner = Application::OpenForms[0];
    else
      _owner = FormWrapper::GetManagedObject<Form>(aOwner);
  } else
    _owner = nullptr;
}

//--------------------------------------------------------------------------------------------------

bool FormWrapper::create(mforms::Form *backend, mforms::Form *aOwner, mforms::FormFlag flag) {
  FormWrapper *wrapper = new FormWrapper(backend, aOwner, flag);
  FillForm ^ form = FormWrapper::Create<FillForm>(backend, wrapper);
  form->wrapper = wrapper;

  if (aOwner != NULL)
    form->StartPosition = FormStartPosition::CenterParent;
  else
    form->StartPosition = FormStartPosition::Manual;

  if (File::Exists("images/icons/MySQLWorkbench.ico"))
    form->Icon = gcnew Icon("images/icons/MySQLWorkbench.ico", Size(16, 16));

  if ((flag & mforms::FormToolWindow) != 0) {
    if ((flag & mforms::FormResizable) != 0)
      form->FormBorderStyle = FormBorderStyle::SizableToolWindow;
    else
      form->FormBorderStyle = FormBorderStyle::FixedToolWindow;
  } else if ((flag & mforms::FormSingleFrame) != 0)
    form->FormBorderStyle = FormBorderStyle::FixedSingle;
  else if ((flag & mforms::FormDialogFrame) != 0)
    form->FormBorderStyle = FormBorderStyle::FixedDialog;
  else if ((flag & mforms::FormResizable) != 0)
    form->FormBorderStyle = FormBorderStyle::Sizable;
  else
    form->FormBorderStyle = FormBorderStyle::None;

  form->MinimizeBox = (flag & mforms::FormMinimizable) != 0;
  form->MaximizeBox = (flag & mforms::FormMinimizable) != 0;

  if ((flag & mforms::FormStayOnTop) != 0)
    form->Owner = UtilitiesWrapper::get_mainform();

  wrapper->hideOnClose = (flag & mforms::FormHideOnClose) != 0;
  ViewWrapper::remove_auto_resize(form, AutoResizeMode::ResizeBoth);

  return true;
}

//--------------------------------------------------------------------------------------------------

void FormWrapper::set_title(mforms::Form *backend, const std::string &title) {
  FormWrapper::GetControl(backend)->Text = CppStringToNativeRaw(title.c_str());
}

//--------------------------------------------------------------------------------------------------

void FormWrapper::show_modal(mforms::Form *backend, mforms::Button *accept, mforms::Button *cancel) {
  FillForm ^ form = FormWrapper::GetManagedObject<FillForm>(backend);

  if (accept != NULL) {
    form->AcceptButton = FormWrapper::GetManagedObject<System::Windows::Forms::Button>(accept);
    form->AcceptButton->DialogResult = System::Windows::Forms::DialogResult::OK;
  }

  if (cancel != NULL) {
    form->CancelButton = FormWrapper::GetManagedObject<System::Windows::Forms::Button>(cancel);
    form->CancelButton->DialogResult = System::Windows::Forms::DialogResult::Cancel;
  }

  // Make the window top most (so it stays above all others), but non-blocking.
  form->Owner = UtilitiesWrapper::get_mainform();

  // Initially make it resize to its minimum size regardless of its normal sizing behavior.
  // Later auto resizing only happens if the form is not set to allow manual sizing.
  ViewWrapper::set_full_auto_resize(form);
  if (form->InvokeRequired)
    form->BeginInvoke(gcnew MethodInvoker(form, &Control::Show));
  else
    form->Show();
}

//--------------------------------------------------------------------------------------------------

bool FormWrapper::run_modal(mforms::Form *backend, mforms::Button *accept, mforms::Button *cancel) {
  FormWrapper *wrapper = backend->get_data<FormWrapper>();
  FillForm ^ form = wrapper->GetManagedObject<FillForm>();

  if (accept) {
    form->AcceptButton = FormWrapper::GetManagedObject<System::Windows::Forms::Button>(accept);
    form->AcceptButton->DialogResult = System::Windows::Forms::DialogResult::OK;
  }

  if (cancel) {
    form->CancelButton = FormWrapper::GetManagedObject<System::Windows::Forms::Button>(cancel);
    form->CancelButton->DialogResult = System::Windows::Forms::DialogResult::Cancel;
  }

  ViewWrapper::set_full_auto_resize(form);

  System::Windows::Forms::DialogResult dialog_result;
  if (form->InvokeRequired) {
    Object ^ invocation_result =
      form->Invoke(gcnew FillForm::ShowModalDelegate(form, &System::Windows::Forms::Form::ShowDialog),
                   gcnew array<Object ^>{wrapper->_owner});
    dialog_result = *(System::Windows::Forms::DialogResult ^)(invocation_result);
  } else {
    // If there is currently no active form then we are being called while the application
    // is not active. Focus the accept button if possible in that case to avoid having any
    // text field with keyboard focus, which might appear as if they were active (but are not in fact).
    System::Windows::Forms::Form ^ active_form = System::Windows::Forms::Form::ActiveForm;
    if (active_form == nullptr) {
      if (form->AcceptButton != nullptr)
        form->ActiveControl = (Button ^)form->AcceptButton;
      else if (form->CancelButton != nullptr)
        form->ActiveControl = (Button ^)form->CancelButton;
    }

    mforms::Utilities::enter_modal_loop();
    System::Windows::Forms::Form ^ owner = (System::Windows::Forms::Form ^)wrapper->_owner;
    dialog_result = form->ShowDialog(owner);
    mforms::Utilities::leave_modal_loop();
  }

  bool result = (dialog_result == System::Windows::Forms::DialogResult::OK);

  form->Hide();

  return result;
}

//--------------------------------------------------------------------------------------------------

void FormWrapper::end_modal(mforms::Form *backend, bool result) {
  FormWrapper::GetManagedObject<Form>(backend)->DialogResult =
    result ? System::Windows::Forms::DialogResult::OK : System::Windows::Forms::DialogResult::Cancel;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the backend when a form is closed from the application side.
 */
void FormWrapper::close(mforms::Form *backend) {
  FormWrapper::GetManagedObject<Form>(backend)->Close();
}

//--------------------------------------------------------------------------------------------------

void FormWrapper::set_content(mforms::Form *backend, mforms::View *view) {
  Control ^ child = FormWrapper::GetControl(view);
  FormWrapper::GetControl(backend)->Controls->Add(child);
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the startup position of the form so that it is centered over its parent when displayed.
 * If the form has no parent the desktop is used. This is also the default setting.
 */
void FormWrapper::center(mforms::Form *backend) {
  FormWrapper::GetManagedObject<Form>(backend)->StartPosition = FormStartPosition::CenterParent;
}

//--------------------------------------------------------------------------------------------------

void FormWrapper::flush_events(mforms::Form *backend) {
  Application::DoEvents();
}

//--------------------------------------------------------------------------------------------------

bool FormWrapper::hide_on_close() {
  return hideOnClose;
}

//--------------------------------------------------------------------------------------------------\

void FormWrapper::set_menubar(mforms::Form *backend, mforms::MenuBar *menubar) {
  mforms::Box *content = dynamic_cast<mforms::Box *>(backend->get_content());
  if (!content)
    throw std::invalid_argument(
      "set_menubar() must be called after a toplevel content box has been added to the window");

  MySQL::Controls::DrawablePanel ^ menuPanel;
  {
    // wrap menubar inside a container that will give it a background color
    Control ^ control = BoxWrapper::GetControl(menubar);

    menuPanel = gcnew MySQL::Controls::DrawablePanel();
    menuPanel->BackColor = Color::Transparent;
    menuPanel->CustomBackground = true;
    menuPanel->Dock = DockStyle::Top;
    menuPanel->AutoSize = true;

    control->BackColor = Conversions::GetApplicationColor(ApplicationColor::AppColorMainTab, false);

    menuPanel->Controls->Add(control);
    control->Dock = DockStyle::Top;
    control->AutoSize = true;
  }

  LayoutBox ^ box = BoxWrapper::GetManagedObject<LayoutBox>(content);
  box->Add(menuPanel, false, true);
  // TODO reorder the menuPanel to the top
  content->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void FormWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_form_impl.create = &FormWrapper::create;
  f->_form_impl.close = &FormWrapper::close;
  f->_form_impl.set_content = &FormWrapper::set_content;
  f->_form_impl.set_title = &FormWrapper::set_title;
  f->_form_impl.run_modal = &FormWrapper::run_modal;
  f->_form_impl.show_modal = &FormWrapper::show_modal;
  f->_form_impl.end_modal = &FormWrapper::end_modal;
  f->_form_impl.center = &FormWrapper::center;
  f->_form_impl.flush_events = &FormWrapper::flush_events;
  f->_form_impl.set_menubar = &FormWrapper::set_menubar;
}
