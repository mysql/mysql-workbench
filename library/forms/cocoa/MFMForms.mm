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

#import "MFMForms.h"
#include "mforms.h"

extern void cf_view_init();
extern void cf_form_init();
extern void cf_button_init();
extern void cf_box_init();
extern void cf_table_init();
extern void cf_textentry_init();
extern void cf_label_init();
extern void cf_textbox_init();
extern void cf_tabview_init();
extern void cf_checkbox_init();
extern void cf_panel_init();
extern void cf_selector_init();
extern void cf_radiobutton_init();
extern void cf_imagebox_init();
extern void cf_progressbar_init();
extern void cf_filechooser_init();
extern void cf_scrollpanel_init();
extern void cf_listbox_init();
extern void cf_treeview_init();
extern void cf_wizard_init();
extern void cf_util_init();
extern void cf_drawbox_init();
extern void cf_splitter_init();
extern void cf_popup_init();
extern void cf_menu_init();
extern void cf_codeeditor_init();
extern void cf_menubar_init();
extern void cf_toolbar_init();
extern void cf_hypertext_init();
extern void cf_popover_init();
extern void cf_findpanel_init();
extern void cf_canvas_init();

extern "C" {

void mforms_cocoa_init() {
  cf_view_init();
  cf_form_init();
  cf_button_init();
  cf_box_init();
  cf_table_init();
  cf_textentry_init();
  cf_label_init();
  cf_textbox_init();
  cf_tabview_init();
  cf_checkbox_init();
  cf_panel_init();
  cf_selector_init();
  cf_radiobutton_init();
  cf_imagebox_init();
  cf_progressbar_init();
  cf_filechooser_init();
  cf_scrollpanel_init();
  cf_listbox_init();
  cf_splitter_init();
  cf_drawbox_init();
  cf_popup_init();
  cf_menu_init();
  cf_codeeditor_init();

  cf_wizard_init();
  
  cf_util_init();
  cf_menubar_init();
  cf_toolbar_init();
  cf_hypertext_init();
  cf_popover_init();
  
  cf_treeview_init();
  cf_findpanel_init();

  cf_canvas_init();
}
  
void mforms_cocoa_check() {
  mforms::ControlFactory::get_instance()->check_impl();
}
  
};
