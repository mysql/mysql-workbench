/*
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "model_panel.h"
#include "overview_panel.h"
#include "model/wb_context_model.h"
#include "workbench/wb_context.h"
#include "linux_utilities/gtk_helpers.h"
#include <gtkmm/alignment.h>
#include <gtkmm/notebook.h>
#include <gtk/gtkhpaned.h>
#include "mforms/../gtk/lf_toolbar.h"
#include "mforms/../gtk/lf_menubar.h"
#include "mforms/../gtk/lf_view.h"
#include "documentation_box.h"
#ifdef COMMERCIAL_CODE
#include "validation_panel.h"
#endif

#define _(s) s

bool ModelPanel::on_close()
{
  bool has_changes = _wb->get_wb()->has_unsaved_changes();
  _overview->on_close(); 
  return !has_changes;
}


void ModelPanel::on_activate()
{
  mforms::View *sidebar = _wb->get_wb()->get_model_context()->shared_secondary_sidebar();
  Gtk::Widget *w = mforms::widget_for_view(sidebar);
  if (w->get_parent())
    gtk_reparent_realized(w,_secondary_sidebar);
  else
    _secondary_sidebar->add(*w);
  w->show();
}


ModelPanel *ModelPanel::create(wb::WBContextUI *wb, wb::OverviewBE *overview)
{
  Glib::RefPtr<Gtk::Builder> xml= Gtk::Builder::create_from_file(wb->get_wb()->get_grt_manager()->get_data_file_path("model_view.glade"));

  ModelPanel *panel = 0;
  xml->get_widget_derived<ModelPanel>("top_vbox", panel);
  panel->post_construct(wb, overview, xml);
  
  return panel;
}


ModelPanel::ModelPanel(GtkVBox *paned, Glib::RefPtr<Gtk::Builder> xml)
  : Gtk::VBox(paned), FormViewBase("ModelOverview"), _wb(0)
#ifdef COMMERCIAL_CODE
  , _validation_panel(0)
#endif
{
  _pending_rebuild_overview = false;
}

void ModelPanel::post_construct(wb::WBContextUI *wb, wb::OverviewBE *overview, Glib::RefPtr<Gtk::Builder> xml)
{
  _wb= wb;
  _grtm= wb->get_wb()->get_grt_manager();
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

  xml->get_widget("model_pane", _sidebar1_pane);
  xml->get_widget("model_hpane", _sidebar2_pane);
 
  xml->get_widget("sidebar_frame", _secondary_sidebar);

  _overview = Gtk::manage(new OverviewPanel(wb, overview));
  
  xml->get_widget("editor_tab", _editor_note);
  xml->get_widget("content", _editor_paned);
  
  xml->get_widget("model_sidebar", _sidebar);
  
  Gtk::Alignment *placeholder;
  xml->get_widget("overview_placeholder", placeholder);
  placeholder->add(*_overview);
  _overview->show();
 
  _sidebar1_pane->property_position().signal_changed().connect(sigc::mem_fun(this, &ModelPanel::resize_overview));
  _sidebar2_pane->property_position().signal_changed().connect(sigc::mem_fun(this, &ModelPanel::resize_overview));

  Gtk::Notebook *note;
  Gtk::Label *label;

  xml->get_widget("side_model_note0", note);
  _documentation_box= Gtk::manage(new DocumentationBox(wb));
  label = Gtk::manage(new Gtk::Label(_("Description")));
  note->append_page(*_documentation_box, *label);
  label->set_use_markup(true);
  label->set_name("tab");

  xml->get_widget("side_model_note1", note);
  _usertypes_box= wb->get_wb()->get_model_context()->create_user_type_list();
  label = Gtk::manage(new Gtk::Label(_("User Types")));
  note->append_page(*mforms::widget_for_view(_usertypes_box), *label);
  label->set_use_markup(true);
  label->set_name("tab");

  _history_tree= wb->get_wb()->get_model_context()->create_history_tree();
  label = Gtk::manage(new Gtk::Label(_("History")));
  note->append_page(*mforms::widget_for_view(_history_tree), *label);
  label->set_use_markup(true);
  label->set_name("tab");

  #ifdef COMMERCIAL_CODE
  _validation_panel = Gtk::manage(new ValidationPanel());
  note->append_page(*_validation_panel, *_validation_panel->notebook_label());
  #endif

  _sig_restore_layout.disconnect();
  // restore widths of sidebars when shown
  _sig_restore_layout = Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(this, &ModelPanel::restore_sidebar_layout), false));
}


void ModelPanel::restore_sidebar_layout()
{
  FormViewBase::restore_sidebar_layout();
  do_resize_overview();
}


ModelPanel::~ModelPanel()
{
  _sig_restore_layout.disconnect();
  _sig_resize_overview.disconnect();
  delete _history_tree;
  delete _usertypes_box;
}


bool ModelPanel::do_resize_overview()
{
  _overview->update_for_resize();
  _pending_rebuild_overview = false;
  return false;
}


void ModelPanel::resize_overview()
{
  if (!_pending_rebuild_overview)
  {
    // this hack is needed to force the iconview to properly resize to accomodate its new
    // container size
    _sig_resize_overview.disconnect();
    _sig_resize_overview = Glib::signal_idle().connect(sigc::mem_fun(this, &ModelPanel::do_resize_overview));
    _pending_rebuild_overview = true;
  }
}

void ModelPanel::selection_changed()
{
  _documentation_box->update_for_form(_overview->get_be());
}


bec::UIForm *ModelPanel::get_form() const
{
  return (bec::UIForm*)_overview->get_be();
}


void ModelPanel::find_text(const std::string &text)
{
  _last_found_node = _overview->get_be()->search_child_item_node_matching(bec::NodeId()
                                                                          , _last_found_node
                                                                          ,text
                                                                          );

  if (_last_found_node.is_valid())
  {
    get_overview()->select_node(_last_found_node);
  }
}
