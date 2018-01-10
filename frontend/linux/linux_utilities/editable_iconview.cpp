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

#include "editable_iconview.h"
#include <gtk/gtk.h>

//------------------------------------------------------------------------------
EditableIconView::EditableIconView(const Glib::RefPtr<Gtk::TreeModel>& model) : Gtk::IconView(model) {
#if GTK_MINOR_VERSION >= 18
  gtk_icon_view_set_item_padding(gobj(), 4);
#endif
}

//------------------------------------------------------------------------------
EditableIconView::EditableIconView() : Gtk::IconView() {
}

//------------------------------------------------------------------------------
bool EditableIconView::on_button_press_event(GdkEventButton* event) {
  // Handle Gtk::IconView event first so it can do its logic before we are in
  const bool ret = Gtk::IconView::on_button_press_event(event);

  // Recheck here if we have correct model set for the IconView. It is possible
  // that we are dealing with Gtk::ListStore
  if (_model) {
    // Get the item at the mouse position, it is the one we need to decide on editing
    Gtk::TreeModel::Path path;

    if (get_item_at_pos((int)event->x, (int)event->y, path)) {
      const bec::NodeId node(_model->get_node_for_path(path));

      Gtk::CellRenderer* cell(0);
      get_item_at_pos((int)event->x, (int)event->y, path, cell);

      if (node.is_valid() && _model->get_be_model()->is_editable(node)) {
        // Check if it was the item selected before. We do not want to start edit
        // on newly clicked item
        if (_selected_path.gobj() && _selected_path.to_string() == path.to_string()) {
          if (cell && GTK_IS_CELL_RENDERER_TEXT(cell->gobj())) {
            Gtk::CellRendererText* rend = static_cast<Gtk::CellRendererText*>(cell);
            // Enable editing, without that we are not able to start editing as gtk's code checks
            // for cellrenderer to be editable
            rend->property_editable() = true;
            // connect signal so we can handle editing_done. Handling editing_done allows
            // us to update model, otherwise gtk won't pass new value to the model
            _start_conn = rend->signal_editing_started().connect(sigc::mem_fun(this, &EditableIconView::edit_started));
#if GTK_VERSION_GT(2, 14)
            set_cursor(path, *cell, true);
#else
            // gtkmm shipped with RHEL5.4 doesn't have set_cursor(), even tho gtk does
            gtk_icon_view_set_cursor(gobj(), path.gobj(), cell->gobj(), TRUE);
#endif
            // Disable editing, otherwise we will start edit on the next clicking on a different item
            rend->property_editable() = false;
          }
        }
      }
    }
    // Update selected path so we can detect second single click on the item
    _selected_path = path;
  }

  return ret;
}

//------------------------------------------------------------------------------
void EditableIconView::edit_started(Gtk::CellEditable* editable, const Glib::ustring& path) {
  _start_conn.disconnect();
  if (editable)
    _done_conn =
      editable->signal_editing_done().connect(sigc::bind(sigc::mem_fun(this, &EditableIconView::edit_done), editable));
}

//------------------------------------------------------------------------------
void EditableIconView::edit_done(Gtk::CellEditable* editable) {
  Gtk::Entry* entry = static_cast<Gtk::Entry*>(editable);
  if (entry) {
    Gtk::TreeModel::iterator iter = _model->get_iter(_selected_path);
    Gtk::TreeModel::Row row = *iter;
    if (row) {
      std::string data("");
      row.get_value(get_text_column(), data);
      if (data != entry->get_text())
        row.set_value(get_text_column(), entry->get_text());
    }
  }
  _done_conn.disconnect();
}
