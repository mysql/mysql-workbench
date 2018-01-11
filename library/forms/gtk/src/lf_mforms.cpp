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

#include "../lf_mforms.h"

#include "../lf_button.h"
#include "../lf_radiobutton.h"
#include "../lf_mforms.h"
#include "../lf_checkbox.h"
#include "../lf_textentry.h"
#include "../lf_textbox.h"
#include "../lf_imagebox.h"
#include "../lf_label.h"
#include "../lf_selector.h"
#include "../lf_listbox.h"
#include "../lf_tabview.h"
#include "../lf_form.h"
#include "../lf_panel.h"
#include "../lf_box.h"
#include "../lf_table.h"
#include "../lf_progressbar.h"
#include "../lf_filechooser.h"
#include "../lf_scrollpanel.h"
#include "../lf_treeview.h"
#include "../lf_wizard.h"
#include "../lf_utilities.h"
#include "../lf_drawbox.h"
#include "../lf_splitter.h"
#include "../lf_popup.h"
#include "../lf_menu.h"
#include "../lf_code_editor.h"
#include "../lf_menubar.h"
#include "../lf_toolbar.h"
#include "../lf_canvas.h"

extern void lf_findpanel_init();
namespace mforms {
  namespace gtk {
    bool force_sys_colors = false;

    extern void HyperText_init();
    extern void Popover_init();
  }
}
void ::mforms::gtk::init(bool force_sys_colors_) {
  force_sys_colors = force_sys_colors_;
  ViewImpl::init();
  BoxImpl::init();
  FormImpl::init();
  ButtonImpl::init();
  RadioButtonImpl::init();
  CheckBoxImpl::init();
  TextEntryImpl::init();
  TextBoxImpl::init();
  LabelImpl::init();
  ImageBoxImpl::init();
  SelectorImpl::init();
  ListBoxImpl::init();
  PanelImpl::init();
  TabViewImpl::init();
  FormImpl::init();
  FileChooserImpl::init();
  ProgressBarImpl::init();
  TableImpl::init();
  ScrollPanelImpl::init();
  TreeViewImpl::init();
  WizardImpl::init();
  UtilitiesImpl::init();
  DrawBoxImpl::init();
  SplitterImpl::init();
  PopupImpl::init();
  MenuImpl::init();
  CodeEditorImpl::init();
  lf_menubar_init();
  mforms::gtk::lf_toolbar_init();
  mforms::gtk::HyperText_init();
  mforms::gtk::Popover_init();
  lf_findpanel_init();
  CanvasImpl::init();
  //  GRTTreeViewImpl::init(); initialized in main program
}

void ::mforms::gtk::check() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();
  f->check_impl();
}
