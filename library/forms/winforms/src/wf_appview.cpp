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

#include "base/drawing.h"

#include "wf_base.h"
#include "wf_view.h"
#include "wf_box.h"
#include "wf_appview.h"
#include "wf_toolbar.h"

using namespace System::Drawing::Drawing2D;
using namespace System::Windows::Forms;

using namespace MySQL::Forms;
using namespace MySQL::Controls;

//----------------- AppViewDockContent -------------------------------------------------------------

AppViewDockContent::AppViewDockContent() {
  appview = NULL;
};

//--------------------------------------------------------------------------------------------------

AppViewDockContent::~AppViewDockContent() {
  // if (appview != NULL)
  //   appview->release();
}

//--------------------------------------------------------------------------------------------------

void AppViewDockContent::SetBackend(mforms::AppView *backend) {
  appview = backend;
  // Don't hold a reference, the wrapper should be deleted when the backend object is deleted,
  // not the other way around.. this would cause a circular reference and leak
  // appview->retain();
}

//--------------------------------------------------------------------------------------------------

mforms::AppView *AppViewDockContent::GetBackend() {
  return appview;
}

//--------------------------------------------------------------------------------------------------

String ^ AppViewDockContent::GetAppViewIdentifier() {
  return CppStringToNative(appview->identifier());
}

//--------------------------------------------------------------------------------------------------

String ^ MySQL::Forms::AppViewDockContent::GetContextName() {
  return CppStringToNative(appview->get_form_context_name());
}

//--------------------------------------------------------------------------------------------------

MenuStrip ^ AppViewDockContent::GetMenuBar() {
  mforms::MenuBar *menu = appview->get_menubar();
  if (menu == NULL)
    return nullptr;

  return AppViewWrapper::GetManagedObject<MenuStrip>(menu);
}

//--------------------------------------------------------------------------------------------------

ToolStrip ^ AppViewDockContent::GetToolBar() {
  mforms::ToolBar *toolbar = appview->get_toolbar();
  if (toolbar == NULL)
    return nullptr;

  return AppViewWrapper::GetManagedObject<ToolStrip>(toolbar);
}

//--------------------------------------------------------------------------------------------------

bool AppViewDockContent::CanCloseDocument() {
  return appview->on_close();
}

//--------------------------------------------------------------------------------------------------

void AppViewDockContent::CloseDocument() {
  appview->close();
}

//--------------------------------------------------------------------------------------------------

String ^ AppViewDockContent::GetTitle() {
  return CppStringToNativeRaw(appview->get_title());
}

//--------------------------------------------------------------------------------------------------

void AppViewDockContent::SetTitle(String ^ title) {
  appview->set_title(NativeToCppStringRaw(title));
}

//--------------------------------------------------------------------------------------------------

void AppViewDockContent::UpdateColors() {
  // Change our own background or that of only child, if our content was embedded into a DrawablePanel
  // to implement a design with embedded menu/toolbar)
  if (Controls->Count > 0 && is<DrawablePanel>(Controls[0]))
    Controls[0]->BackColor = Conversions::GetApplicationColor(ApplicationColor::AppColorMainBackground, false);
  else
    BackColor = Conversions::GetApplicationColor(ApplicationColor::AppColorMainBackground, false);

  MenuStrip ^ menuStrip = GetMenuBar();
  if (menuStrip != nullptr) {
    menuStrip->BackColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelToolbar, false);
    menuStrip->ForeColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelToolbar, true);
    if (Conversions::UseWin8Drawing())
      menuStrip->Renderer = gcnew Win8MenuStripRenderer();
    else
      menuStrip->Renderer = gcnew TransparentMenuStripRenderer();
  }

  ToolStrip ^ toolStrip = GetToolBar();
  if (toolStrip != nullptr) {
    toolStrip->BackColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelToolbar, false);
    toolStrip->ForeColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelToolbar, true);
  }
}

//----------------- AppViewWrapper -----------------------------------------------------------------

AppViewWrapper::AppViewWrapper(mforms::AppView *app) : BoxWrapper(app), appview(app) {
}

//--------------------------------------------------------------------------------------------------

bool AppViewWrapper::create(mforms::AppView *backend, bool horizontal) {
  AppViewWrapper *wrapper = new AppViewWrapper(backend);
  wrapper->set_resize_mode(AutoResizeMode::ResizeNone);

  // In order to ease maintenance we create a special document host for our content.
  // This adds another nesting level, however.
  LayoutBox ^ box = Create<LayoutBox>(backend, wrapper);
  box->Horizontal = horizontal;

  wrapper->host = gcnew AppViewDockContent();
  wrapper->host->SetBackend(backend);

  box->BackColor = Drawing::Color::Transparent;
  wrapper->host->Controls->Add(box);
  box->Dock = DockStyle::Fill;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when this app view is about to be docked in a host container. Create the frontend
 * tab document if not yet done and return it to the caller.
 */
AppViewDockContent ^ AppViewWrapper::GetHost() {
  return host;
}

//--------------------------------------------------------------------------------------------------

void AppViewWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();
  f->_app_view_impl.create = &AppViewWrapper::create;
}

//--------------------------------------------------------------------------------------------------

AppViewWrapper::~AppViewWrapper() {
  delete host;
  host = nullptr;
}

//--------------------------------------------------------------------------------------------------
