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

#include "wf_mforms.h"

#include "wf_base.h"
#include "wf_view.h"
#include "wf_button.h"
#include "wf_form.h"
#include "wf_checkbox.h"
#include "wf_textentry.h"
#include "wf_textbox.h"
#include "wf_imagebox.h"
#include "wf_label.h"
#include "wf_selector.h"
#include "wf_panel.h"
#include "wf_tabview.h"
#include "wf_box.h"
#include "wf_progressbar.h"
#include "wf_radiobutton.h"
#include "wf_table.h"
#include "wf_filechooser.h"
#include "wf_listbox.h"
#include "wf_wizard.h"
#include "wf_scrollpanel.h"
#include "wf_utilities.h"
#include "wf_drawbox.h"
#include "wf_splitter.h"
#include "wf_popup.h"
#include "wf_menu.h"
#include "wf_menubar.h"
#include "wf_toolbar.h"
#include "wf_code_editor.h"
#include "wf_hypertext.h"
#include "wf_popover.h"
#include "wf_treeview.h"
#include "wf_find_panel.h"
#include "wf_native.h"
#include "wf_canvas.h"
#include "wf_gridview.h"

#include "wf_appview.h"
#include "wf_app.h"

#include "base/log.h"

using namespace System::Threading;

using namespace MySQL;
using namespace MySQL::Forms;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

//--------------------------------------------------------------------------------------------------

Manager::Manager() {
  created = 0;
  destroyed = 0;

  logInfo("Initializing mforms wrapper\n");

  ViewWrapper::init();
  LabelWrapper::init();
  BoxWrapper::init();
  MenuBarWrapper::init();
  UtilitiesWrapper::init();
  AppWrapper::init();
  ToolBarWrapper::init();
  AppViewWrapper::init();
  FormWrapper::init();
  ButtonWrapper::init();
  CheckBoxWrapper::init();
  TextEntryWrapper::init();
  TextBoxWrapper::init();
  SelectorWrapper::init();
  PanelWrapper::init();
  TabViewWrapper::init();
  ProgressBarWrapper::init();
  ImageBoxWrapper::init();
  RadioButtonWrapper::init();
  TableWrapper::init();
  FileChooserWrapper::init();
  ListBoxWrapper::init();
  WizardWrapper::init();
  ScrollPanelWrapper::init();
  DrawBoxWrapper::init();
  SplitterWrapper::init();
  PopupWrapper::init();
  MenuWrapper::init();
  FindPanelWrapper::init();
  CodeEditorWrapper::init();
  HyperTextWrapper::init();
  PopoverWrapper::init();
  TreeViewWrapper::init();
  CanvasWrapper::init();
  //  mforms::ControlFactory::get_instance()->check_impl();
}

//--------------------------------------------------------------------------------------------------

Manager::~Manager() {
  logInfo("Shutting down mforms wrapper\n");
  /* Doesn't really work since on app shutdown many objects are just not regularly freed.
  base::Logger::log(base::Logger::LogDebug2, DOMAIN_MFORMS_WRAPPER,
    "Created %i wrapper objects, destroyed %i, leaking %i objects\n",
    created, destroyed, created - destroyed);
    */
}

//--------------------------------------------------------------------------------------------------

Manager ^ MySQL::Forms::Manager::get_instance() {
  // The manager is created very early in the app startup process from the main thread, so this
  // should never be a concurrency problem here.
  if (singleton == nullptr)
    singleton = gcnew Manager();
  return singleton;
}

//--------------------------------------------------------------------------------------------------

void Manager::instance_created() {
  // Atomic increment shouldn't be necessary as we always create and destroy wrappers in the
  // main thread. But better safe than sorry.
  Interlocked::Increment(created);
}

//--------------------------------------------------------------------------------------------------

void Manager::instance_destroyed() {
  Interlocked::Increment(destroyed);
}

//--------------------------------------------------------------------------------------------------
