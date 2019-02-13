/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "model_panel.h"
#include "overview_panel.h"
#include "model/wb_context_model.h"
#include "workbench/wb_context.h"
#include "gtk_helpers.h"
#include <gtkmm/alignment.h>
#include <gtkmm/notebook.h>
#include <gtk/gtkpaned.h>
#include "mforms/../gtk/lf_toolbar.h"
#include "mforms/../gtk/lf_menubar.h"
#include "mforms/../gtk/lf_view.h"
#include "documentation_box.h"

#define _(s) s

bool ModelPanel::on_close() {
  bool has_changes = wb::WBContextUI::get()->get_wb()->has_unsaved_changes();
  _overview->on_close();
  return !has_changes;
}

void ModelPanel::on_activate() {
  mforms::View *sidebar = wb::WBContextUI::get()->get_wb()->get_model_context()->shared_secondary_sidebar();
  Gtk::Widget *w = mforms::widget_for_view(sidebar);
  if (w->get_parent())
    gtk_reparent_realized(w, _secondary_sidebar);
  else
    _secondary_sidebar->add(*w);
  w->show();
}

ModelPanel *ModelPanel::create(wb::OverviewBE *overview) {
  Glib::RefPtr<Gtk::Builder> xml =
    Gtk::Builder::create_from_file(bec::GRTManager::get()->get_data_file_path("model_view.glade"));

  ModelPanel *panel = 0;
  xml->get_widget_derived("top_vbox", panel);
  panel->post_construct(overview);

  return panel;
}

ModelPanel::ModelPanel(GtkBox *cobject, const Glib::RefPtr<Gtk::Builder> &xml)
  : Gtk::Box(cobject),
    FormViewBase("ModelOverview"),
    _overview(0),
    _editor_paned(0),
    _sidebar(0),
    _secondary_sidebar(0),
    _history_tree(0),
    _usertypes_box(0),
    _documentation_box(0),
    _builder(xml) {
  _pending_rebuild_overview = false;
}

void ModelPanel::post_construct(wb::OverviewBE *overview) {
  _toolbar = overview->get_toolbar();
  {
    mforms::MenuBar *menubar = overview->get_menubar();
    if (menubar)
      pack_start(*mforms::widget_for_menubar(menubar), false, true);
    mforms::ToolBar *toolbar = overview->get_toolbar();
    if (toolbar)
      pack_start(*mforms::widget_for_toolbar(toolbar), false, true);
  }
  show_all();

  _builder->get_widget("model_pane", _sidebar1_pane);
  _builder->get_widget("model_hpane", _sidebar2_pane);

  _builder->get_widget("sidebar_frame", _secondary_sidebar);

  _overview = Gtk::manage(new OverviewPanel(overview));

  _builder->get_widget("editor_tab", _editor_note);
  _builder->get_widget("content", _editor_paned);

  _builder->get_widget("model_sidebar", _sidebar);

  Gtk::Box *placeholder;
  _builder->get_widget("overview_placeholder", placeholder);
  placeholder->pack_start(*_overview, true, true);

  _overview->show();

  _sidebar1_pane->property_position().signal_changed().connect(sigc::mem_fun(this, &ModelPanel::resize_overview));
  _sidebar2_pane->property_position().signal_changed().connect(sigc::mem_fun(this, &ModelPanel::resize_overview));

  Gtk::Notebook *note;
  Gtk::Label *label;

  _builder->get_widget("side_model_note0", note);
  _documentation_box = Gtk::manage(new DocumentationBox());
  label = Gtk::manage(new Gtk::Label(_("<small>Description</small>")));
  note->append_page(*_documentation_box, *label);
  label->set_use_markup(true);

  _builder->get_widget("side_model_note1", note);
  _usertypes_box = wb::WBContextUI::get()->get_wb()->get_model_context()->create_user_type_list();
  label = Gtk::manage(new Gtk::Label(_("<small>User Types</small>")));
  note->append_page(*mforms::widget_for_view(_usertypes_box), *label);
  label->set_use_markup(true);

  _history_tree = wb::WBContextUI::get()->get_wb()->get_model_context()->create_history_tree();
  label = Gtk::manage(new Gtk::Label(_("<small>History</small>")));
  note->append_page(*mforms::widget_for_view(_history_tree), *label);
  label->set_use_markup(true);

  _sig_restore_layout.disconnect();
  // restore widths of sidebars when shown
  _sig_restore_layout =
    Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun<void, ModelPanel, ModelPanel>(this, &ModelPanel::restore_sidebar_layout), false));
}

void ModelPanel::restore_sidebar_layout() {
  FormViewBase::restore_sidebar_layout(275);
  do_resize_overview();
}

ModelPanel::~ModelPanel() {
  _sig_restore_layout.disconnect();
  _sig_resize_overview.disconnect();
  delete _history_tree;
  delete _usertypes_box;
}

bool ModelPanel::do_resize_overview() {
  _overview->update_for_resize();
  _pending_rebuild_overview = false;
  return false;
}

void ModelPanel::resize_overview() {
  if (!_pending_rebuild_overview) {
    // this hack is needed to force the iconview to properly resize to accomodate its new
    // container size
    _sig_resize_overview.disconnect();
    _sig_resize_overview = Glib::signal_idle().connect(sigc::mem_fun(this, &ModelPanel::do_resize_overview));
    _pending_rebuild_overview = true;
  }
}

void ModelPanel::selection_changed() {
  _documentation_box->update_for_form(_overview->get_be());
}

bec::UIForm *ModelPanel::get_form() const {
  return (bec::UIForm *)_overview->get_be();
}

void ModelPanel::find_text(const std::string &text) {
  _last_found_node = _overview->get_be()->search_child_item_node_matching(bec::NodeId(), _last_found_node, text);

  if (_last_found_node.is_valid()) {
    get_overview()->select_node(_last_found_node);
  }
}
