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


#ifndef _MODEL_PANEL_H_
#define _MODEL_PANEL_H_

#include <map>
#include <gtkmm/paned.h>
#include <gtkmm/notebook.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/builder.h>
#include "form_view_base.h"

#include "workbench/wb_overview.h"

class OverviewPanel;
class DocumentationBox;
#ifdef COMMERCIAL_CODE
class ValidationPanel;
#endif

namespace mforms {
  class TreeView;
};

class ModelPanel : public Gtk::Box, public FormViewBase {
public:
  static ModelPanel *create(wb::OverviewBE *overview);
  ~ModelPanel();

  virtual bool on_close() override;
  virtual void on_activate() override;

  virtual Gtk::Widget *get_panel() override {
    return this;
  }
  virtual bec::UIForm *get_form() const override;

  virtual void reset_layout() override {
    _editor_paned->set_position(_editor_paned->get_height() - 300);
  }
  OverviewPanel *get_overview() {
    return _overview;
  }

  void selection_changed();

  virtual void find_text(const std::string &text) override;
  using FormViewBase::restore_sidebar_layout;
  virtual void restore_sidebar_layout();

private:
  OverviewPanel *_overview;
  Gtk::Paned *_editor_paned;
  Gtk::Widget *_sidebar;
  Gtk::Frame *_secondary_sidebar;
  bec::NodeId _last_found_node;

  mforms::TreeView *_history_tree;
  mforms::TreeView *_usertypes_box;
  DocumentationBox *_documentation_box;
  Glib::RefPtr<Gtk::Builder> _builder;
  bool _pending_rebuild_overview;

  friend class Gtk::Builder;
  ModelPanel(GtkBox *cobject, const Glib::RefPtr<Gtk::Builder> &xml);
  void post_construct(wb::OverviewBE *overview);

  void resize_overview();
  bool do_resize_overview();

  sigc::connection _sig_restore_layout;
  sigc::connection _sig_resize_overview;
};

#endif /* _MODEL_PANEL_H_ */
