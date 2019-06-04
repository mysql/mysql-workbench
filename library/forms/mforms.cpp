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

#include "base/log.h"

#include "mforms/mforms.h"

using namespace mforms;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE);

extern GThread *_mforms_main_thread;

// The first time this method is called must be from the main thread, during startup.
ControlFactory *ControlFactory::get_instance() {
  static ControlFactory *instance = NULL;

  if (!instance) {
    logDebug2("Initializing mforms factory\n");

    // Do some one time initializations.
    _mforms_main_thread = g_thread_self();

    instance = new ControlFactory();
  }

  return instance;
}

ControlFactory::ControlFactory() {
  memset(&_view_impl, 0, sizeof(_view_impl));
  memset(&_form_impl, 0, sizeof(_form_impl));
  memset(&_box_impl, 0, sizeof(_box_impl));
  memset(&_button_impl, 0, sizeof(_button_impl));
  memset(&_checkbox_impl, 0, sizeof(_checkbox_impl));
  memset(&_textentry_impl, 0, sizeof(_textentry_impl));
  memset(&_textbox_impl, 0, sizeof(_textbox_impl));
  memset(&_label_impl, 0, sizeof(_label_impl));
  memset(&_selector_impl, 0, sizeof(_selector_impl));
  memset(&_listbox_impl, 0, sizeof(_listbox_impl));
  memset(&_tabview_impl, 0, sizeof(_tabview_impl));
  memset(&_panel_impl, 0, sizeof(_panel_impl));
  memset(&_filechooser_impl, 0, sizeof(_filechooser_impl));
  memset(&_radio_impl, 0, sizeof(_radio_impl));
  memset(&_imagebox_impl, 0, sizeof(_imagebox_impl));
  memset(&_progressbar_impl, 0, sizeof(_progressbar_impl));
  memset(&_table_impl, 0, sizeof(_table_impl));
  memset(&_spanel_impl, 0, sizeof(_spanel_impl));
  memset(&_wizard_impl, 0, sizeof(_wizard_impl));
  memset(&_utilities_impl, 0, sizeof(_utilities_impl));
  memset(&_app_impl, 0, sizeof(_app_impl));
  memset(&_drawbox_impl, 0, sizeof(_drawbox_impl));
  memset(&_app_view_impl, 0, sizeof(_app_view_impl));
  memset(&_menu_impl, 0, sizeof(_menu_impl));
  memset(&_splitter_impl, 0, sizeof(_splitter_impl));
  memset(&_menu_item_impl, 0, sizeof(_menu_item_impl));
  memset(&_tool_bar_impl, 0, sizeof(_tool_bar_impl));
  memset(&_code_editor_impl, 0, sizeof(_code_editor_impl));
  memset(&_hypertext_impl, 0, sizeof(_hypertext_impl));
  memset(&_popover_impl, 0, sizeof(_popover_impl));
  memset(&_treeview_impl, 0, sizeof(_treeview_impl));
  memset(&_findpanel_impl, 0, sizeof(_findpanel_impl));
  memset(&_popup_impl, 0, sizeof(_popup_impl));
  memset(&_canvas_impl, 0, sizeof(_canvas_impl));
}

// perform a check on the function pointer table to see if there's any NULL ptrs
#define CHECKPTRS(v)                                                \
  {                                                                 \
    void **ptrs = (void **)&v;                                      \
    for (unsigned int i = 0; i < sizeof(v) / sizeof(void *); i++) { \
      if (ptrs[i] == 0)                                             \
        logError("%s has NULL ptr at %i\n", #v, i);                 \
    }                                                               \
  }

//--------------------------------------------------------------------------------------------------

void ControlFactory::check_impl() {
#if defined(_DEBUG) || defined(ENABLE_DEBUG)
  CHECKPTRS(_view_impl);
  CHECKPTRS(_form_impl);
  CHECKPTRS(_box_impl);
  CHECKPTRS(_button_impl);
  CHECKPTRS(_checkbox_impl);
  CHECKPTRS(_textentry_impl);
  CHECKPTRS(_textbox_impl);
  CHECKPTRS(_label_impl);
  CHECKPTRS(_selector_impl);
  CHECKPTRS(_listbox_impl);
  CHECKPTRS(_tabview_impl);
  CHECKPTRS(_panel_impl);
  CHECKPTRS(_filechooser_impl);
  CHECKPTRS(_radio_impl);
  CHECKPTRS(_imagebox_impl);
  CHECKPTRS(_progressbar_impl);
  CHECKPTRS(_table_impl);
  CHECKPTRS(_spanel_impl);
  CHECKPTRS(_treeview_impl);
  CHECKPTRS(_wizard_impl);
  CHECKPTRS(_utilities_impl);
  CHECKPTRS(_drawbox_impl);
  CHECKPTRS(_app_impl);
  CHECKPTRS(_splitter_impl);
  CHECKPTRS(_menu_impl);
  CHECKPTRS(_menu_item_impl);
  CHECKPTRS(_tool_bar_impl);
  CHECKPTRS(_hypertext_impl);
  CHECKPTRS(_popover_impl);
  CHECKPTRS(_findpanel_impl);
  CHECKPTRS(_canvas_impl);
#endif
}

//--------------------------------------------------------------------------------------------------

ControlFactory::~ControlFactory() {
  logInfo("Shutting down mforms backend\n");
}

//--------------------------------------------------------------------------------------------------
