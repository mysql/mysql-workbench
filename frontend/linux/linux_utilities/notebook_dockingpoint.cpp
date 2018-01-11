/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "notebook_dockingpoint.h"
#include "active_label.h"
#include "gtk/lf_mforms.h"
#include "gtk/lf_view.h"
#include "base/log.h"

NotebookDockingPoint::NotebookDockingPoint(Gtk::Notebook *note, const std::string &type)
  : _notebook(note), _type(type) {
  if (_notebook)
    _notebook->signal_switch_page().connect(
      sigc::hide(sigc::hide(sigc::mem_fun(_dpoint, &mforms::DockingPoint::view_switched))));
}

void NotebookDockingPoint::set_notebook(Gtk::Notebook *note) {
  _notebook = note;
  _notebook->signal_switch_page().connect(
    sigc::hide(sigc::hide(sigc::mem_fun(_dpoint, &mforms::DockingPoint::view_switched))));
}

void NotebookDockingPoint::close_appview_page(mforms::AppView *view) {
  if (view->on_close())
    view->close();
}

bool NotebookDockingPoint::close_page(Gtk::Widget *w) {
  mforms::AppView *aview = dynamic_cast<mforms::AppView *>(mforms::gtk::ViewImpl::get_view_for_widget(w));
  if (aview) {
    if (aview->on_close()) {
      aview->close();
      return true;
    } else
      return false;
  }
  return true;
}

void NotebookDockingPoint::set_name(const std::string &name) {
  auto acc = _notebook->get_accessible();
  if (acc)
    acc->set_name(name);
}

void NotebookDockingPoint::dock_view(mforms::AppView *view, const std::string &arg1, int arg2) {
  Gtk::Widget *w = mforms::widget_for_view(view);
  if (w) {
    ActiveLabel *l = Gtk::manage(
      new ActiveLabel("mforms", sigc::bind(sigc::mem_fun(this, &NotebookDockingPoint::close_appview_page), view)));
    int i = _notebook->append_page(*w, *l);

    if (view->release_on_add())
      view->set_release_on_add(false);
    else
      view->retain();

    _notebook->set_current_page(i);
    w->set_data("NotebookDockingPoint:label", l);

    notebook_changed_signal.emit(true);
  }
}

bool NotebookDockingPoint::select_view(mforms::AppView *view) {
  Gtk::Widget *w = mforms::widget_for_view(view);
  if (w) {
    int p = _notebook->page_num(*w);
    if (p >= 0) {
      _notebook->set_current_page(p);
      return true;
    }
  }
  return false;
}

void NotebookDockingPoint::undock_view(mforms::AppView *view) {
  Gtk::Widget *w = mforms::widget_for_view(view);
  if (w) {
    // before remove, unset menu if it was set
    _notebook->remove_page(*w);
    notebook_changed_signal.emit(false);
    view->release();
  }
}

void NotebookDockingPoint::set_view_title(mforms::AppView *view, const std::string &title) {
  Gtk::Widget *w = mforms::widget_for_view(view);
  if (w) {
    int idx = _notebook->page_num(*w);
    if (idx >= 0) {
      Gtk::Widget *page = _notebook->get_nth_page(idx);
      if (page) {
        ActiveLabel *label = reinterpret_cast<ActiveLabel *>(page->get_data("NotebookDockingPoint:label"));
        if (label)
          label->set_text(title);
      }
    } else
      g_warning("Can't set title of unknown view to %s", title.c_str());
  }
}

std::pair<int, int> NotebookDockingPoint::get_size() {
  return std::pair<int, int>(_notebook->get_width(), _notebook->get_height());
}

mforms::AppView *NotebookDockingPoint::selected_view() {
  int i = _notebook->get_current_page();
  if (i >= 0)
    return view_at_index(i);
  return NULL;
}

int NotebookDockingPoint::view_count() {
  return _notebook->get_n_pages();
}

mforms::AppView *NotebookDockingPoint::view_at_index(int index) {
  Gtk::Widget *w = _notebook->get_nth_page(index);
  if (w)
    return dynamic_cast<mforms::AppView *>(mforms::gtk::ViewImpl::get_view_for_widget(w));
  return NULL;
}
