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

#include "grt/editor_base.h"
#include "gtk/lf_mforms.h"
#include "sqlide_form.h"
#include "gtk_helpers.h"
#include "base/util_functions.h"
#include "base/wb_iterators.h"
#include "base/log.h"
#include "mforms/../gtk/lf_view.h"
#include "mforms/../gtk/lf_menubar.h"
#include "mforms/../gtk/lf_toolbar.h"
#include "objimpl/wrapper/mforms_ObjectReference_impl.h"
#include "sqlide/query_side_palette.h"
#include "sqlide/wb_sql_editor_panel.h"
#include "workbench/wb_context.h"
#include <glib.h>
#include "grt/common.h"
#include "widget_saver.h"
#include "plugin_editor_base.h"

using base::strfmt;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DbSqlEditorView *DbSqlEditorView::create(SqlEditorForm::Ref editor_be) {
  return Gtk::manage(new DbSqlEditorView(editor_be));
}

//------------------------------------------------------------------------------
DbSqlEditorView::DbSqlEditorView(SqlEditorForm::Ref editor_be)
  : Gtk::Box(Gtk::ORIENTATION_VERTICAL),
    FormViewBase("DbSqlEditor"),
    _be(editor_be),
    _top_pane(Gtk::ORIENTATION_HORIZONTAL),
    _top_right_pane(Gtk::ORIENTATION_HORIZONTAL),
    _main_pane(Gtk::ORIENTATION_VERTICAL),
    _output(_be, this),
    _side_palette(mforms::gtk::ViewImpl::get_widget_for_view(_be->get_side_palette())),
    _dock_delegate(NULL, MAIN_DOCKING_POINT),
    _busy_tab(NULL),
    _right_aligned(wb::WBContextUI::get()->get_wb()->get_wb_options().get_int("Sidebar:RightAligned", 0)),
    _editor_maximized(false) {
  _dpoint = mforms::manage(new mforms::DockingPoint(&_dock_delegate, false));

  _top_pane.set_name("sqled.top_pane" + std::string(_right_aligned ? ".right_aligned" : ".left_aligned"));
  _main_pane.set_name("sqled.main_pane" + std::string(_right_aligned ? ".right_aligned" : ".left_aligned"));
  _top_right_pane.set_name("sqled.top_right_pane" + std::string(_right_aligned ? ".right_aligned" : ".left_aligned"));

  _be = editor_be;
  _be->set_frontend_data(dynamic_cast<FormViewBase *>(this));
  _toolbar = _be->get_toolbar();

  _be->set_busy_tab = std::bind(&DbSqlEditorView::set_busy_tab, this, std::placeholders::_1);

  mforms::View *sbview = _be->get_sidebar();
  Gtk::Widget *sidebar = mforms::gtk::ViewImpl::get_widget_for_view(sbview);

  _editor_note = Gtk::manage(new Gtk::Notebook()); // init FormViewBase::_editor_note
  _editor_note->show();
  _editor_note->set_scrollable(true);
  _editor_note->set_show_border(false);

  auto acc = _main_pane.get_accessible();
  if (acc)
    acc->set_name("SQL IDE Query Area");

  acc = _top_pane.get_accessible();
  if (acc)
    acc->set_name("SQL IDE View");

  acc = _editor_note->get_accessible();
  if (acc)
    acc->set_name("Query Area");

  _main_pane.pack1(_top_right_pane, true, false);
  _main_pane.pack2(_output.get_outer(), false, true);

  sidebar->set_no_show_all(true);
  if (_right_aligned) {
    _top_pane.add1(_main_pane);
    _top_pane.add2(*sidebar);

    _top_right_pane.pack2(*_editor_note, true, false);
    if (_side_palette)
      _top_right_pane.pack1(*_side_palette, false, true);
  } else {
    _top_pane.add1(*sidebar);
    _top_pane.add2(_main_pane);

    _top_right_pane.pack1(*_editor_note, true, false);
    if (_side_palette)
      _top_right_pane.pack2(*_side_palette, false, true);
  }

  _sidebar1_pane = &_top_pane;
  _sidebar2_pane = &_top_right_pane;
  PanedConstrainer::make_constrainer(_sidebar1_pane, 120, 0);
  PanedConstrainer::make_constrainer(_sidebar2_pane, 0, 120);

  _top_pane.show_all();
  _top_right_pane.show_all();
  _main_pane.show_all();

  // Add main menu
  Gtk::Widget *w = mforms::widget_for_menubar(_be->get_menubar());
  if (w)
    pack_start(*w, false, true);

  // Add main toolbar
  w = mforms::widget_for_toolbar(_be->get_toolbar());
  if (w) {
    w->show();
    pack_start(*w, false, true);
  }

  pack_start(_top_pane, true, true);
  show_all();

  // Connect signals
  _be->post_query_slot = std::bind(&DbSqlEditorView::on_exec_sql_done, this);
  _be->output_text_slot = std::bind(&DbSqlEditorView::output_text, this, std::placeholders::_1, std::placeholders::_2);

  _editor_note->signal_switch_page().connect(sigc::mem_fun(this, &DbSqlEditorView::editor_page_switched));
  _editor_note->signal_page_reordered().connect(sigc::mem_fun(this, &DbSqlEditorView::editor_page_reordered));
  _editor_note->signal_page_added().connect(sigc::mem_fun(this, &DbSqlEditorView::editor_page_added));
  _editor_note->signal_page_removed().connect(sigc::mem_fun(this, &DbSqlEditorView::editor_page_removed));

  _polish_conn = signal_map().connect(sigc::mem_fun(this, &DbSqlEditorView::polish));

  // restore state of toolbar
  {
    mforms::ToolBar *toolbar = _be->get_toolbar();
    bool flag;

    toolbar->set_item_checked("wb.toggleOutputArea",
                              flag = !bec::GRTManager::get()->get_app_option_int("DbSqlEditor:OutputAreaHidden", 0));
    if (flag)
      _main_pane.get_child2()->show();
    else
      _main_pane.get_child2()->hide();
  }

  utils::gtk::load_settings(
    &_main_pane, sigc::bind(sigc::ptr_fun(gtk_paned_set_pos_ratio), &_main_pane, _right_aligned ? 0.2 : 0.8), false);

  _main_pane.property_position().signal_changed().connect(
    sigc::bind(sigc::ptr_fun(utils::gtk::save_settings), &_main_pane, false));

  _sig_restore_sidebar =
    Glib::signal_idle().connect(sigc::bind_return(sigc::bind(sigc::mem_fun(this, &FormViewBase::restore_sidebar_layout), 200, 200), false));

  // setup dockingPoint
  {
    db_query_EditorRef editor(_be->wbsql()->get_grt_editor_object(_be.get()));
    editor->dockingPoint(mforms_to_grt(_dpoint));
  }
  _dock_delegate.set_notebook(_editor_note);
  _be->set_tab_dock(_dpoint);
}
//------------------------------------------------------------------------------

bool DbSqlEditorView::perform_command(const std::string &cmd) {
  if (cmd == "wb.toggleOutputArea") {
    Gtk::Widget *w = _main_pane.get_child2();
    bool hidden = !be()->get_toolbar()->get_item_checked("wb.toggleOutputArea");
    if (!hidden)
      w->show();
    else
      w->hide();
    bec::GRTManager::get()->set_app_option("DbSqlEditor:OutputAreaHidden", grt::IntegerRef(hidden));
  } else if (cmd == "wb.next_query_tab") {
    int i = _editor_note->get_current_page();
    _editor_note->next_page();
    if (i == _editor_note->get_current_page())
      _editor_note->set_current_page(0);
  } else if (cmd == "wb.back_query_tab") {
    int i = _editor_note->get_current_page();
    _editor_note->prev_page();
    if (i == _editor_note->get_current_page())
      _editor_note->set_current_page(_editor_note->get_current_page() - 1);
  } else
    return FormViewBase::perform_command(cmd);
  return true;
}

//------------------------------------------------------------------------------
void DbSqlEditorView::polish() {
  gtk_paned_set_pos_ratio(&_main_pane, 0.7);
  _polish_conn.disconnect();
}

//------------------------------------------------------------------------------
DbSqlEditorView::~DbSqlEditorView() {
  // state is saved when it changes. saving again on quit, will give unexpected results
  //  utils::gtk::save_settings(_grtm, &_top_pane, false);
  //  utils::gtk::save_settings(_grtm, &_main_pane, false);
  //  utils::gtk::save_settings(_grtm, &_top_right_pane, true);

  _sig_restore_sidebar.disconnect();

  if (_side_palette)
    _side_palette->hide();

  if (_dpoint)
    _dpoint->release();

  if (_busy_tab)
    _busy_tab->unreference();

  dispose();
}

//------------------------------------------------------------------------------
void DbSqlEditorView::dispose() {
  if (_be) {
    _be->close();
    _be.reset();
  }
}

//------------------------------------------------------------------------------
void DbSqlEditorView::init() {
}

//------------------------------------------------------------------------------
mforms::Menu *DbSqlEditorView::init_tab_menu(Gtk::Widget *w) {
  {
    mforms::Menu *m = new mforms::Menu();
    m->add_item("New Tab", "new_tab");
    m->add_item("Save Tab", "save_tab");
    m->add_separator();
    m->add_item("Copy Path to Clipboard", "copy_path");
    m->add_separator();
    m->add_item("Close Tab", "close_tab");
    m->add_item("Close Other Tabs", "close_other_tabs");
    return m;
  }
}

void DbSqlEditorView::tab_menu_handler(const std::string &action, ActiveLabel *sender, Gtk::Widget *widget) {
  if (!_be)
    return;

  PluginEditorBase *_pluginView = dynamic_cast<PluginEditorBase *>(widget);
  if (_pluginView) { // We have to handle this on our own,
    if (action == "close_tab") {
      bec::GRTManager::get()->run_once_when_idle(std::bind(&FormViewBase::close_plugin_tab, this, _pluginView));
      return; // We stop here
    }  else if (action == "close_other_tabs") { // and clear up what we can, everything else will be handled by the backend.
      for (auto i = 0; i < _editor_note->get_n_pages(); i++) {
        auto *plugin = dynamic_cast<PluginEditorBase *>(_editor_note->get_nth_page(i));
        if (plugin != nullptr && plugin != widget) {
          close_plugin_tab(plugin);
        }
      }
    }
  }

  // Once we're done with platform code, pass handling to the backend
  if (widget)
  {
    int page = _editor_note->page_num(*widget);

    _be->handle_tab_menu_action(action, page);
  }
}
//------------------------------------------------------------------------------
void DbSqlEditorView::reenable_items_in_tab_menus() {
  const int size = _editor_note->get_n_pages();

  for (int i = 0; i < size; ++i) {
    Gtk::Widget *page = _editor_note->get_nth_page(i);
    ActiveLabel *const al = dynamic_cast<ActiveLabel *>(_editor_note->get_tab_label(*page));
    if (al) {
      mforms::Menu *const menu = al->get_menu();
      if (menu) {
        const int index = menu->get_item_index("close_other_tabs");
        if (index >= 0)
          menu->set_item_enabled(index, size > 1);
      }
    }
  }
}

void DbSqlEditorView::set_busy_tab(int tab) {
  if (_busy_tab) {
    _busy_tab->stop_busy();
    _busy_tab->unreference();
    _busy_tab = NULL;
  }
  if (tab >= 0) {
    Gtk::Widget *page = _editor_note->get_nth_page(tab);
    ActiveLabel *const al = dynamic_cast<ActiveLabel *>(_editor_note->get_tab_label(*page));
    if (al) {
      al->start_busy();
      _busy_tab = al;
      _busy_tab->reference();
    }
  }
}

void DbSqlEditorView::editor_page_added(Gtk::Widget *page, guint index) {
  // first check if new tab has menu, if not.. connect it ;)
  ActiveLabel *const al = dynamic_cast<ActiveLabel *>(_editor_note->get_tab_label(*page));
  if (al && !al->has_menu()) {
    mforms::Menu *menu = init_tab_menu(page);
    menu->set_handler(sigc::bind(sigc::mem_fun(this, &DbSqlEditorView::tab_menu_handler), al, page));
    al->set_menu(menu, true);
  }

  _editor_note->set_tab_reorderable(*page, true);

  reenable_items_in_tab_menus();
}

void DbSqlEditorView::editor_page_reordered(Gtk::Widget *page, guint index) {
  SqlEditorPanel *panel = be()->sql_editor_panel(index);
  if (panel)
    be()->sql_editor_reordered(panel, index);
}

void DbSqlEditorView::editor_page_removed(Gtk::Widget *page, guint index) {
  reenable_items_in_tab_menus();
}

//------------------------------------------------------------------------------
void DbSqlEditorView::plugin_tab_added(PluginEditorBase *plugin) {
  const int page_num = _editor_note->page_num(*(static_cast<Gtk::Widget *>(plugin)));
  if (page_num >= 0)
    _editor_note->set_current_page(page_num);
}

void DbSqlEditorView::close_appview_tab(mforms::AppView *aview) {
  if (aview)
    _dock_delegate.close_appview_page(aview);
}

//------------------------------------------------------------------------------

bool DbSqlEditorView::close_focused_tab() {
  {
    Gtk::Widget *content = _editor_note->get_nth_page(_editor_note->get_current_page());
    Gtk::Widget *label = _editor_note->get_tab_label(*content);

    ActiveLabel *const al = dynamic_cast<ActiveLabel *>(label);
    if (al) {
      al->call_close();
      //      _editor_note->remove_page(*content);
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void DbSqlEditorView::editor_page_switched(Gtk::Widget *page, guint index) {
  if (_be) {
    _dpoint->view_switched();

    mforms::AppView *aview = _dpoint->view_at_index(index);
    if (aview && aview->get_form_context_name() == "administrator")
      set_maximized_editor_mode(true);
    else {
      if (!_be->connectionIsValid())
        set_maximized_editor_mode(true, true);
      else
        set_maximized_editor_mode(false);
    }
  }
}

//------------------------------------------------------------------------------
void DbSqlEditorView::set_maximized_editor_mode(bool flag, bool hide_schemas) {
  if (_editor_maximized != flag) {
    _editor_maximized = flag;
    if (flag) {
      be()->get_toolbar()->set_item_checked("wb.toggleOutputArea", false);
      be()->get_toolbar()->set_item_checked("wb.toggleSecondarySidebar", false);
      _main_pane.get_child2()->hide();
      _top_right_pane.get_child2()->hide();

      if (hide_schemas) {
        be()->get_toolbar()->set_item_checked("wb.toggleSidebar", false);
        _sidebar1_pane->get_child1()->hide();
      }
    } else {
      be()->get_toolbar()->set_item_checked("wb.toggleSidebar", true);
      be()->get_toolbar()->set_item_checked(
        "wb.toggleOutputArea", !bec::GRTManager::get()->get_app_option_int("DbSqlEditor:OutputAreaHidden"));
      be()->get_toolbar()->set_item_checked(
        "wb.toggleSecondarySidebar", !bec::GRTManager::get()->get_app_option_int("DbSqlEditor:SecondarySidebarHidden"));
      _sidebar1_pane->get_child1()->show();
      perform_command("wb.toggleOutputArea");
      perform_command("wb.toggleSecondarySidebar");
    }
  }
}

//------------------------------------------------------------------------------
bool DbSqlEditorView::on_close() {
  return be()->can_close();
}

//------------------------------------------------------------------------------
void DbSqlEditorView::on_exec_sql_done() {
  _output.refresh();

  if (_be->exec_sql_error_count() > 0)
    _be->show_output_area();
}

//------------------------------------------------------------------------------
void DbSqlEditorView::output_text(const std::string &text, bool bring_to_front) {
  _output.output_text(text, bring_to_front);
}
