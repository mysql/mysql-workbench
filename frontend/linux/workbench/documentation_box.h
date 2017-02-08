/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DOCUMENTATION_BOX_H_
#define _DOCUMENTATION_BOX_H_

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/textview.h>
#include "workbench/wb_context_ui.h"
#include "text_list_columns_model.h"

class DocumentationBox : public Gtk::Box {
  Gtk::ComboBox _combo;
  TextListColumnsModel _comboModel;
  Gtk::TextView _text;
  sigc::connection _timer;
  bec::UIForm *_selected_form;
  grt::ListRef<GrtObject> _object_list;
  bool _multiple_items;
  bool _initializing;

  void text_key_press(GdkEventKey *event);
  void text_button_press(GdkEventButton *event);
  void combo_changed();
  void text_changed();
  void commit();

public:
  DocumentationBox();
  ~DocumentationBox();

  void update_for_form(bec::UIForm *form);
};

#endif /* _DOCUMENTATION_BOX_H_ */
