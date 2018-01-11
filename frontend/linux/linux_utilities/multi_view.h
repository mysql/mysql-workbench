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

//!
//! \addtogroup linuxui Linux UI
//! @{
//!

#ifndef _MULTIVIEW_H_
#define _MULTIVIEW_H_

#include <gtkmm/grid.h>
#include "treemodel_wrapper.h"
#include "grt/tree_model.h"
#include "editable_iconview.h"

// Because of Gtk3 bug: https://bugzilla.gnome.org/show_bug.cgi?id=749575
// we need to use Gtk::Grid instead of Gtk::Box parent of MultiView

class MultiView : public Gtk::Grid {
  Gtk::TreeView *_tree_view;
  EditableIconView *_icon_view;
  Glib::RefPtr<TreeModelWrapper> _tv_model;
  Glib::RefPtr<TreeModelWrapper> _iv_model;
  Glib::RefPtr<Gtk::TreeSelection> _selection;

  sigc::signal<void, const std::vector<bec::NodeId> &> _selection_changed;
  sigc::signal<void, Gtk::TreeModel::Path, guint32> _popup_menu;
  sigc::signal<void, Gtk::TreeModel::Path> _activate_item;

  void tree_row_activated(const Gtk::TreeModel::Path &path, const Gtk::TreeViewColumn *column);
  void icon_activated(const Gtk::TreeModel::Path &path);
  void icon_button_release_event(GdkEventButton *event);
  void tree_button_release_event(GdkEventButton *event);
  void icon_selection_changed();
  void tree_selection_changed();

protected:
  virtual void on_selection_changed(const std::vector<bec::NodeId> &sel);

public:
  MultiView(bool tree_view, bool icon_view);
  virtual ~MultiView();

  Gtk::TreeView *get_tree_view() const {
    return _tree_view;
  }
  Gtk::IconView *get_icon_view() const {
    return _icon_view;
  }

  virtual void refresh();
  void set_tree_model(const Glib::RefPtr<TreeModelWrapper> &model);
  void set_icon_model(const Glib::RefPtr<TreeModelWrapper> &model);
  void unset_models();
  Glib::RefPtr<TreeModelWrapper> get_tree_model() {
    return _tv_model;
  }
  Glib::RefPtr<TreeModelWrapper> get_icon_model() {
    return _iv_model;
  }

  void set_icon_mode(bool flag, bool horizontal_icons = false);

  Gtk::TreeModel::Path get_selected();
  void select_node(const bec::NodeId &node);

  sigc::signal<void, const std::vector<bec::NodeId> &> signal_selection_changed() {
    return _selection_changed;
  }
  sigc::signal<void, Gtk::TreeModel::Path, guint32> signal_popup_menu() {
    return _popup_menu;
  }
  sigc::signal<void, Gtk::TreeModel::Path> signal_activate_item() {
    return _activate_item;
  }
};

#endif /* _MULTIVIEW_H_ */

//!
//! @}
//!
