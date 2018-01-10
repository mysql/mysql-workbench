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

#include "multi_view.h"

#define HORIZONTAL_LAYOUT_ITEM_WIDTH 200
#define VERTICAL_LAYOUT_ITEM_WIDTH 100

MultiView::MultiView(bool tree_view, bool icon_view) : Gtk::Grid(), _tree_view(0), _icon_view(0) {
  set_orientation(Gtk::ORIENTATION_VERTICAL);
  if (tree_view) {
    _tree_view = Gtk::manage(new Gtk::TreeView());
    add(*_tree_view);
    _tree_view->set_hexpand(true);
    _tree_view->signal_row_activated().connect(sigc::mem_fun(this, &MultiView::tree_row_activated));
    _tree_view->signal_button_release_event().connect_notify(
      sigc::mem_fun(*this, &MultiView::tree_button_release_event));

    _selection = _tree_view->get_selection();
    _selection->signal_changed().connect(sigc::mem_fun(this, &MultiView::tree_selection_changed));
    // TODO: There was a possible leak when connecting signal by
    // _tree_view->get_selection()->signal_changed().connect(sigc::mem_fun(this, &MultiView::tree_selection_changed));
    // TODO: Check if it still leaks after selection is moved to explicit member variable
    // TODO: Try to disconnect signal in dtor
  }

  if (icon_view) {
    _icon_view = Gtk::manage(new EditableIconView());
    add(*_icon_view);
    _icon_view->set_hexpand(true);
    _icon_view->set_item_orientation(Gtk::ORIENTATION_HORIZONTAL);
    _icon_view->set_selection_mode(Gtk::SELECTION_MULTIPLE);
    _icon_view->set_item_width(HORIZONTAL_LAYOUT_ITEM_WIDTH);
    _icon_view->set_row_spacing(0);

    _icon_view->signal_item_activated().connect(sigc::mem_fun(this, &MultiView::icon_activated));
    _icon_view->signal_button_release_event().connect_notify(
      sigc::mem_fun(*this, &MultiView::icon_button_release_event));
    _icon_view->signal_selection_changed().connect(sigc::mem_fun(this, &MultiView::icon_selection_changed));
  }

  if (tree_view)
    //_tree_view->show();
    set_icon_mode(false);
  else if (icon_view)
    set_icon_mode(true);
  //_icon_view->show();
}

MultiView::~MultiView() {
}

void MultiView::set_icon_mode(bool flag, bool horizontal_icons) {
  if (_tree_view && _icon_view) {
    if (flag) {
      _tree_view->hide();
      _icon_view->show();

      _icon_view->set_item_orientation(horizontal_icons ? Gtk::ORIENTATION_HORIZONTAL : Gtk::ORIENTATION_VERTICAL);

      if (horizontal_icons)
        _icon_view->set_item_width(HORIZONTAL_LAYOUT_ITEM_WIDTH);
      else
        _icon_view->set_item_width(VERTICAL_LAYOUT_ITEM_WIDTH);
    } else {
      _tree_view->show();
      _icon_view->hide();
    }
  } else if (_icon_view)
    _icon_view->set_item_orientation(horizontal_icons ? Gtk::ORIENTATION_HORIZONTAL : Gtk::ORIENTATION_VERTICAL);
}

void MultiView::set_tree_model(const Glib::RefPtr<TreeModelWrapper>& model) {
  _tv_model = model;

  if (_tree_view)
    _tree_view->set_model(model);
}

void MultiView::set_icon_model(const Glib::RefPtr<TreeModelWrapper>& model) {
  _iv_model = model;

  if (_icon_view)
    _icon_view->set_model(model);
}

void MultiView::unset_models() {
  if (_icon_view)
    _icon_view->unset_model();
  if (_tree_view)
    _tree_view->unset_model();
}

void MultiView::select_node(const bec::NodeId& node) {
  if (!node.is_valid()) {
    if (_tree_view)
      _tree_view->get_selection()->unselect_all();
    if (_icon_view)
      _icon_view->unselect_all();
    return;
  }

  Gtk::TreeModel::Path path = node2path(node);

  if (_tree_view)
    _tree_view->set_cursor(path);
  if (_icon_view)
    _icon_view->select_path(path);
}

void MultiView::refresh() {
  if (_tree_view) {
    bec::ListModel* m = _tv_model->get_be_model();
    _tv_model->set_be_model(0);
    _tree_view->unset_model();
    _tree_view->set_model(_tv_model);

    _tv_model->set_be_model(m);
    //_model->refresh(); // TODO: this call is useless as OverviewBE::refresh is empty/commented out
    _tree_view->unset_model();
    _tree_view->set_model(_tv_model);
  }
  if (_icon_view) {
    _icon_view->set_model(Glib::RefPtr<ListModelWrapper>());
    _icon_view->set_model(_iv_model);
  }
}

void MultiView::tree_row_activated(const Gtk::TreeModel::Path& path, const Gtk::TreeViewColumn* column) {
  _activate_item(path);
}

void MultiView::icon_activated(const Gtk::TreeModel::Path& path) {
  _activate_item(path);
}

void MultiView::icon_button_release_event(GdkEventButton* event) {
  if (GDK_BUTTON_RELEASE == event->type && 3 == event->button) {
    Gtk::TreeModel::Path path;

    if (_icon_view->get_item_at_pos(event->x, event->y, path))
      _icon_view->select_path(path);

    std::vector<Gtk::TreeModel::Path> selected_items = _icon_view->get_selected_items();
    if (selected_items.size() > 0) {
      _popup_menu.emit(selected_items[0], event->time);
      return;
    }

    _popup_menu.emit(Gtk::TreeModel::Path(), event->time);
  }
}

void MultiView::tree_button_release_event(GdkEventButton* event) {
  if (GDK_BUTTON_RELEASE == event->type && 3 == event->button) {
    Glib::RefPtr<Gtk::TreeView::Selection> selection = _tree_view->get_selection();

    if (selection->count_selected_rows() > 0) {
      std::vector<Gtk::TreeModel::Path> selected = selection->get_selected_rows();
      _popup_menu.emit(selected[0], event->time);
    }
    _popup_menu.emit(Gtk::TreeModel::Path(), event->time);
  }
}

void MultiView::on_selection_changed(const std::vector<bec::NodeId>& sel) {
}

void MultiView::icon_selection_changed() {
  const std::vector<Gtk::TreeModel::Path> paths = _icon_view->get_selected_items();
  std::vector<bec::NodeId> nodes;

  const int paths_size = paths.size();
  for (int i = 0; i < paths_size; ++i)
    nodes.push_back(_iv_model->get_node_for_path(paths[i]));

  on_selection_changed(nodes);
  _selection_changed.emit(nodes);
}

void MultiView::tree_selection_changed() {
  const std::vector<Gtk::TreeModel::Path> paths = _tree_view->get_selection()->get_selected_rows();
  std::vector<bec::NodeId> nodes;

  const int paths_size = paths.size();
  for (int i = 0; i < paths_size; ++i)
    nodes.push_back(_tv_model->get_node_for_path(paths[i]));

  on_selection_changed(nodes);
  _selection_changed.emit(nodes);
}

Gtk::TreeModel::Path MultiView::get_selected() {
  if (_icon_view && _icon_view->is_visible()) {
    std::vector<Gtk::TreeModel::Path> selected_items = _icon_view->get_selected_items();
    if (selected_items.size() > 0)
      return selected_items[0];
  }

  if (_tree_view && _tree_view->is_visible()) {
    Glib::RefPtr<Gtk::TreeView::Selection> selection = _tree_view->get_selection();

    if (selection->count_selected_rows() > 0) {
      std::vector<Gtk::TreeModel::Path> selected = selection->get_selected_rows();
      return selected[0];
    }
  }

  return Gtk::TreeModel::Path();
}
