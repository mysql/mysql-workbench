/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "form_view_base.h"
#include "plugin_editor_base.h"
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/window.h>
#include <glibmm/main.h>
#include "active_label.h"
#include "grt/grt_manager.h"
#include "mforms/toolbar.h"

bool FormViewBase::close_plugin_tab(PluginEditorBase *editor) {
  if (editor->can_close()) {
    _close_editor(editor);
    remove_plugin_tab(editor);
  } else
    return false;

  bool has_visible_tabs = false;
  for (int c = _editor_note->get_n_pages(), i = 0; i < c; i++) {
    if (_editor_note->get_nth_page(i)->is_visible()) {
      has_visible_tabs = true;
      break;
    }
  }
  if (!has_visible_tabs)
    _editor_note->hide();

  return true;
}

void FormViewBase::set_close_editor_callback(const sigc::slot<void, PluginEditorBase *> &handler) {
  _close_editor = handler;
}

void FormViewBase::add_plugin_tab(PluginEditorBase *plugin) {
  if (_editor_note) {
    ActiveLabel *label = Gtk::manage(
      new ActiveLabel(plugin->get_title(),
                      sigc::hide_return(sigc::bind(sigc::mem_fun(this, &FormViewBase::close_plugin_tab), plugin))));

    int page_num = _editor_note->append_page(*plugin, *label);

    plugin->signal_title_changed().connect(sigc::mem_fun(label, &ActiveLabel::set_text));

    if (!_editor_note->is_visible()) {
      _editor_note->show();
      reset_layout();
    }

    plugin_tab_added(plugin);

    Glib::signal_idle().connect_once(
      sigc::bind(sigc::mem_fun(_editor_note, &Gtk::Notebook::set_current_page), page_num));

  } else
    g_warning("active form doesn't support editor tabs");
}

void FormViewBase::remove_plugin_tab(PluginEditorBase *plugin) {
  if (_editor_note) {
    _editor_note->remove_page(*plugin);

    //  Make the plugin manager forget about this plugin
    GUIPluginBase *base = dynamic_cast<GUIPluginBase *>(plugin);
    bec::GRTManager::get()->get_plugin_manager()->close_and_forget_gui_plugin(reinterpret_cast<NativeHandle>(base));

    if (_editor_note->get_n_pages() == 0)
      _editor_note->hide();
  }
}

bool FormViewBase::close_editors_for_object(const std::string &id) {
  for (int i = _editor_note->get_n_pages() - 1; i >= 0; --i) {
    Gtk::Widget *panel = _editor_note->get_nth_page(i);
    PluginEditorBase *editor;
    if ((editor = dynamic_cast<PluginEditorBase *>(panel)) && (id.empty() || editor->should_close_on_delete_of(id))) {
      remove_plugin_tab(editor);
      return true;
    }
  }

  return false;
}

PluginEditorBase *FormViewBase::get_focused_plugin_tab() {
  if (_editor_note) {
    Gtk::Widget *focused = dynamic_cast<Gtk::Window *>(_editor_note->get_toplevel())->get_focus();

    // go up the hierarchy to see if the focused widget is inside _editor_note
    while (focused && focused != _editor_note)
      focused = focused->get_parent();

    if (focused) {
      int page = _editor_note->get_current_page();
      if (page >= 0) {
        Gtk::Widget *tab = _editor_note->get_nth_page(page);
        return dynamic_cast<PluginEditorBase *>(tab);
      }
    }
  }

  return 0;
}

bool FormViewBase::close_focused_tab() {
  PluginEditorBase *active = get_focused_plugin_tab();

  if (active) {
    close_plugin_tab(active);
    return true;
  }

  return false;
}

void FormViewBase::restore_sidebar_layout(const int firstSidebarDefaultWidth, const int secondSidebarDefaultWidt) {
  if (_sidebar1_pane) {
    int w = bec::GRTManager::get()->get_app_option_int(_panel_savename + ":SidebarWidth", firstSidebarDefaultWidth);
    _sidebar1_pane->set_position(w);
    if (bec::GRTManager::get()->get_app_option_int(_panel_savename + ":SidebarHidden", 0)) {
      _toolbar->set_item_checked("wb.toggleSidebar", false);
      _sidebar1_pane->get_child1()->hide();
    } else
      _toolbar->set_item_checked("wb.toggleSidebar", true);

    _sidebar1_pane->property_position().signal_changed().connect(
      sigc::bind(sigc::mem_fun(this, &FormViewBase::sidebar_resized), true));
  }

  if (_sidebar2_pane) {
    int w = bec::GRTManager::get()->get_app_option_int(_panel_savename + ":SecondarySidebarWidth", secondSidebarDefaultWidt);
    _sidebar2_pane->set_position(_sidebar2_pane->get_width() - w);
    if (bec::GRTManager::get()->get_app_option_int(_panel_savename + ":SecondarySidebarHidden", 0)) {
      _toolbar->set_item_checked("wb.toggleSecondarySidebar", false);
      _sidebar2_pane->get_child2()->hide();
    } else
      _toolbar->set_item_checked("wb.toggleSecondarySidebar", true);

    _sidebar2_pane->property_position().signal_changed().connect(
      sigc::bind(sigc::mem_fun(this, &FormViewBase::sidebar_resized), false));
  }
}

void FormViewBase::toggle_sidebar(bool show) {
  if (_sidebar1_pane) {
    Gtk::Widget *w = _sidebar1_pane->get_child1();
    if (show)
      w->show();
    else
      w->hide();
  }
}

void FormViewBase::toggle_secondary_sidebar(bool show) {
  if (_sidebar2_pane) {
    Gtk::Widget *w = _sidebar2_pane->get_child2();
    if (show)
      w->show();
    else
      w->hide();
  }
}

void FormViewBase::sidebar_resized(bool primary) {
  if (primary)
    bec::GRTManager::get()->set_app_option(_panel_savename + ":SidebarWidth",
                                           grt::IntegerRef(_sidebar1_pane->get_position()));
  else
    bec::GRTManager::get()->set_app_option(
      _panel_savename + ":SecondarySidebarWidth",
      grt::IntegerRef(_sidebar2_pane->get_width() - _sidebar2_pane->get_position()));
}

bool FormViewBase::perform_command(const std::string &cmd) {
  if (cmd == "wb.toggleSidebar") {
    bool hidden = !_toolbar->get_item_checked(cmd);
    bec::GRTManager::get()->set_app_option(_panel_savename + ":SidebarHidden", grt::IntegerRef(hidden));
    toggle_sidebar(!hidden);
    return true;
  } else if (cmd == "wb.toggleSecondarySidebar") {
    bool hidden = !_toolbar->get_item_checked(cmd);
    bec::GRTManager::get()->set_app_option(_panel_savename + ":SecondarySidebarHidden", grt::IntegerRef(hidden));
    toggle_secondary_sidebar(!hidden);
    return true;
  }
  return false;
}
