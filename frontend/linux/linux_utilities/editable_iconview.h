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

#ifndef __EDITABLE_ICONVIEW_H__
#define __EDITABLE_ICONVIEW_H__

#include <gtkmm/iconview.h>
#include "treemodel_wrapper.h"

//!
//! \addtogroup linuxui Linux UI
//! @{
//!

//! Turns on editing capabilities of the GtkIconView.
//! Default Gtkmm IconView does not support editing of the items. Actually
//! it supports editing, but it is not turned on by default.
//! The code for editing is in there, but it seems that the changes are not
//! passed to the TreeModel. So we need to catch the editing events and feed
//! data to the TreeModel. To detect that editing is done we need to connect
//! to Gtk::CellEditable's signal: editing_done. We can obtain pointer to the
//! CellEditable from the CellRendererText's signal editing_started.
//! For details see comments in the implementation of on_button_press_event
class EditableIconView : public Gtk::IconView {
public:
  EditableIconView();

  void set_model(const Glib::RefPtr<ListModelWrapper>& model) {
    Gtk::IconView::set_model(model);
    _model = model;
  }

protected:
  virtual bool on_button_press_event(GdkEventButton* event);

private:
  EditableIconView(const Glib::RefPtr<Gtk::TreeModel>& model);

  void edit_started(Gtk::CellEditable* editable, const Glib::ustring& path);
  void edit_done(Gtk::CellEditable* editable);

  Gtk::TreeModel::Path _selected_path;   //!< To detect that the click was on already selected item
  sigc::connection _start_conn;          //!< To free signal/slot
  sigc::connection _done_conn;           //!< To free signal/slot
  Glib::RefPtr<ListModelWrapper> _model; //!< To store model in order to detect if an item can be edited
};

//!
//! @}
//!

#endif
