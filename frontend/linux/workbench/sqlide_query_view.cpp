/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "sqlide_query_view.h"

#include "gtk/lf_mforms.h"
#include "linux_utilities/gtk_helpers.h"
#include "linux_utilities/widget_saver.h"
#include "base/util_functions.h"
#include "base/wb_iterators.h"
#include "base/log.h"
#include "mforms/../gtk/lf_native.h"
#include "mforms/../gtk/lf_view.h"
#include "mforms/../gtk/lf_toolbar.h"
#include "objimpl/wrapper/mforms_ObjectReference_impl.h"
#include <glib.h>
#include "grt/common.h"
#include "sqlide/wb_sql_editor_result_panel.h"
#include "sqlide_form.h"

DEFAULT_LOG_DOMAIN("UI")
using base::strfmt;


//==============================================================================
//
//==============================================================================
QueryView::QueryView(const int current_index, DbSqlEditorView* owner)
          : _owner(owner)
          , _editor_box(false)
          , _label(0)
          , _query_toolbar(owner->be()->sql_editor_toolbar(current_index))
          , _apply_btn(_("Apply"))
          , _cancel_btn(_("Revert"))
          , _dock_delegate(&_rs_tabs, RESULT_DOCKING_POINT)
          , _query_collapsed(false)
          , _updating_results(false)
{
  _dpoint = mforms::manage(new mforms::DockingPoint(&_dock_delegate, false));
  SqlEditorForm::Ref             be = owner->be();
  MySQLEditor::Ref            editor = be->sql_editor(current_index);
  Gtk::Widget&        editor_widget = *mforms::widget_for_view(editor->get_editor_control());

  _editor = editor;

  {
    db_query_QueryEditorRef qeditor(db_query_QueryEditorRef::cast_from(_editor->grtobj()));
    qeditor->resultDockingPoint(mforms_to_grt(qeditor->get_grt(), _dpoint, "DockingPoint"));
  }

  _dock_delegate.notebook_changed_signal.connect(sigc::mem_fun(this, &QueryView::update_lower_notebook_visibility));

  editor_widget.set_size_request(-1, 1);

  editor->get_editor_control()->set_show_find_panel_callback(boost::bind(&QueryView::show_find_panel, this, _1, _2));

  _rs_tabs.set_tab_pos(Gtk::POS_BOTTOM);

  boost::shared_ptr<mforms::ToolBar> toolbar(be->sql_editor_toolbar(be->sql_editor_index(editor)));
  Gtk::Widget* w = mforms::widget_for_toolbar(toolbar.get());
  if (w)
  {
    w->show();
    _editor_box.pack_start(*w, false, true);
    //_rs_box.pack_start(*w, false, true);
  }
  else
    g_warning("No toolbar for editor");

  _rs_box.pack_start(_rs_tabs, true, true);
  _rs_box.show_all();

  _editor_box.pack_end(editor_widget, true, true);
  _top_pane.pack1(_editor_box, true, true);
  _top_pane.pack2(_rs_box, false, true);

  _top_pane.set_name("sqled.query_view.top_pane");
  _top_pane.show_all();
  _top_pane.property_position().signal_changed().connect(sigc::mem_fun(this, &QueryView::top_pane_changed));

  _rs_tabs.signal_switch_page().connect(sigc::mem_fun(this, &QueryView::rs_page_switched));
  _rs_tabs.signal_page_reordered().connect(sigc::mem_fun(this, &QueryView::tab_reordered));

  // map is not reliable, it can be mapped but still not visible on screen. so monitor idle 
 // _polish_conn = _top_pane.signal_map().connect(sigc::mem_fun(this, &QueryView::polish));

  _btn_box.pack_start(_apply_btn, true, true);
  _btn_box.pack_start(_cancel_btn, true, true);
  _btn_box.pack_start(_editability_label, true, true);
  _btn_box.pack_start(_editability_icon, false, true);

  _editability_icon.set(Gdk::Pixbuf::create_from_file(_owner->grt_manager()->get_data_file_path("images/mini_notice.png")));

  _rs_tabs.set_action_widget(&_btn_box, Gtk::PACK_END);
  _btn_box.show_all();
  _editability_label.hide();
  _editability_icon.hide();
  _apply_btn.signal_clicked().connect(sigc::mem_fun(this, &QueryView::apply_recordset_changes));
  _cancel_btn.signal_clicked().connect(sigc::mem_fun(this, &QueryView::discard_recordset_changes));
//  utils::gtk::load_settings(_owner->grt_manager(), &_top_pane);
 _polish_conn = Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(this, &QueryView::polish), false));
}

//------------------------------------------------------------------------------
QueryView::~QueryView()
{
  //We need to remove all pending gtk::load.
  if (!_gtk_load.empty())
    _gtk_load.disconnect();

  if (_dpoint)
    _dpoint->release();
}


//------------------------------------------------------------------------------
void QueryView::show_find_panel(mforms::CodeEditor* editor, bool show)
{
  Gtk::Widget *panel = mforms::gtk::ViewImpl::get_widget_for_view(editor->get_find_panel());
  if (show)
  {
    if (!panel->get_parent())
      _editor_box.pack_start(*panel, false, true);
    panel->show();
  }
  else
  {
    panel->hide();
  }
}


//------------------------------------------------------------------------------
void QueryView::top_pane_changed()
{
  if (!_polish_conn.connected() && _rs_tabs.get_n_pages() > 0)
  {
    // user has dragged the splitter (or maybe it was changed by us, but shouldn't be the case)
    _query_collapsed = false;
    utils::gtk::save_settings(_owner->grt_manager(), &_top_pane, false);
  }
}

//------------------------------------------------------------------------------
void QueryView::polish()
{
  if (_owner->be()->sql_editor_start_collapsed(_owner->be()->sql_editor_index(_editor)))
  {
    _query_collapsed = true;
    _top_pane.set_position(100);
  }
  else
    gtk_paned_set_pos_ratio(&_top_pane, 1);
  _polish_conn.disconnect();
}

//------------------------------------------------------------------------------
void QueryView::update_label(const std::string& label)
{
  if (_label)
    _label->set_text(label);
}


//------------------------------------------------------------------------------

void QueryView::update_recordset_caption()
{
  RecordsetView* rs = active_recordset();
  if (rs)
  {
    ActiveLabel *label = dynamic_cast<ActiveLabel*>(_rs_tabs.get_tab_label(*rs));
    if (label)
      label->set_text(rs->model()->caption());

    const bool enable_buttons = rs->model()->has_pending_changes();
    _btn_box.set_sensitive(enable_buttons);
    if (_owner->be()->get_menubar())
      _owner->be()->get_menubar()->validate();
  }
}

//------------------------------------------------------------------------------

static void hide_rs_pane(Gtk::Paned *pane)
{
  void *p = pane->get_data("allow_save");
  pane->set_data("allow_save", 0);
  gtk_paned_set_pos_ratio(pane, 1);
  pane->set_data("allow_save", p);
}


void QueryView::update_resultsets()
{
  const int editor_index = index();

  SqlEditorForm::Ref             be                  = _owner->be();
  const int                      n_recordsets_in_be  = be->recordset_count(editor_index);
  std::vector<SqlEditorResult::Ref>  recordsets_from_be;
  std::map<SqlEditorResult::Ref, int>  recordset_indexes;

  _updating_results = true;

  recordsets_from_be.reserve(n_recordsets_in_be);

  // prepare list of recordsets in BE, which later be matched against exisiting ones in UI(tabs)
  if (n_recordsets_in_be > 0)
  {
    for (int i = n_recordsets_in_be - 1; i >= 0; --i)
    {
      recordsets_from_be.push_back(be->result_panel(be->recordset(editor_index, i)));
      recordset_indexes[recordsets_from_be.back()] = i;
    }
  }

  const int                   ntabs = _rs_tabs.get_n_pages();
  std::vector<SqlEditorResult*>   orphan_views; // list of views which Recordset::ref is not in BE

  // Walk notebook pages removing existing Recordset objects from the @recordsets_from_be list
  // Also add view to @orphan_views if Recordset is not in BE, for later remove_page
  for (int i = 0; i < ntabs; ++i)
  {
    SqlEditorResult* view = dynamic_cast<SqlEditorResult*>(mforms::view_for_widget(_rs_tabs.get_nth_page(i)));
    if (view)
    {
      const size_t before_remove = recordsets_from_be.size();
      for (std::vector<SqlEditorResult::Ref>::iterator it = recordsets_from_be.begin();
          it != recordsets_from_be.end(); ++it)
        if (it->get() == view)
        {
          recordsets_from_be.erase(it);
          break;
        }

      if (recordsets_from_be.size() == before_remove)
        orphan_views.push_back(view);
    }
  }

  // Remove orphan pages
  for (size_t i = 0; i < orphan_views.size(); ++i)
    _rs_tabs.remove_page(*mforms::widget_for_view(orphan_views[i]));

  // Create new pages
  const int size = recordsets_from_be.size();
  for (int i = 0; i < size; ++i)
  {
    SqlEditorResult::Ref        result_panel = recordsets_from_be[i];

    std::auto_ptr<ActiveLabel>   lbl(new ActiveLabel(result_panel->caption(), 
		sigc::bind(sigc::mem_fun(this, &QueryView::close_recordset), result_panel->recordset()->key(), true)));
    std::auto_ptr<RecordsetView> view(RecordsetView::create(result_panel->recordset()));

    if (lbl.get() && view.get())
    {
      lbl->show();
      result_panel->dock_result_grid(mforms::manage(mforms::native_from_widget(view.get())));

      mforms::Menu* menu = init_tab_menu(view.get());
      lbl.get()->set_menu(menu, true);
      menu->set_handler(sigc::bind(sigc::mem_fun(this, &QueryView::tab_menu_handler), lbl.get(), view.get()));
      widget_for_view(result_panel.get())->set_data("active_label", lbl.get());

      const int pos_added = _rs_tabs.append_page(*Gtk::manage(widget_for_view(result_panel.get())), *(Gtk::manage(lbl.release())));
      _rs_tabs.set_tab_reorderable(*widget_for_view(result_panel.get()), true);

      view->refresh();
      view->show();

      view.release();

      _rs_tabs.set_current_page(pos_added);
    }
  }

  if (ntabs == 0 && _rs_tabs.get_n_pages() > 0)
    update_lower_notebook_visibility(true);
  else if (ntabs > 0 && _rs_tabs.get_n_pages() == 0)
    update_lower_notebook_visibility(false);
  _updating_results = false;

  rs_page_switched(0, _rs_tabs.get_current_page());

  reenable_items_in_tab_menus();
  if (_label)
    _label->stop_busy();
}

void QueryView::update_lower_notebook_visibility(bool added)
{
  if (!_query_collapsed)
  {
    const int max_position = _top_pane.get_height() - 200;
    _polish_conn.disconnect();

    if (added && _rs_tabs.get_n_pages() == 1) // first tab was added
      _gtk_load = utils::gtk::load_settings(_owner->grt_manager(), &_top_pane, sigc::bind(sigc::ptr_fun(gtk_paned_set_pos_ratio), &_top_pane, 0.4), false, -max_position);
    else if (_rs_tabs.get_n_pages() == 0) // tabs are gone
      Glib::signal_idle().connect_once(sigc::bind(sigc::ptr_fun(hide_rs_pane), &_top_pane));
    else if (_rs_tabs.get_n_pages() > 0) // more tabs was added or replaced
    { // if the current size is too small, make it a bit bigger
      if (_top_pane.get_position() > max_position)
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(&_top_pane, &Gtk::Paned::set_position), max_position));
    }
  }
}

//------------------------------------------------------------------------------
void QueryView::recalc_rstab_indices()
{
  const int size = _rs_tabs.get_n_pages();
  for (int i = 0; i < size; ++i)
  {
    SqlEditorResult* view = dynamic_cast<SqlEditorResult*>(mforms::view_for_widget(_rs_tabs.get_nth_page(i)));
    if (view)
      _owner->be()->recordset_reorder(index(), view->recordset(), i);
  }
}

//------------------------------------------------------------------------------
void QueryView::close()
{
  _owner->close_editor_tab(this);
}

//------------------------------------------------------------------------------
RecordsetView* QueryView::recordset_view_for_widget(Gtk::Widget *w)
{
  SqlEditorResult *result = dynamic_cast<SqlEditorResult*>(mforms::view_for_widget(w));
  if (result)
    return dynamic_cast<RecordsetView*>(mforms::widget_for_view(result->result_grid()));
  return NULL;
}

//------------------------------------------------------------------------------
RecordsetView* QueryView::active_recordset()
{
  RecordsetView* rs(0);
  const int page = _rs_tabs.get_current_page();
  if (page >= 0)
    rs = recordset_view_for_widget(_rs_tabs.get_nth_page(page));

  return rs;
}

//------------------------------------------------------------------------------
void QueryView::close_recordset(long long key, bool confirm)
{
  const int ntabs = _rs_tabs.get_n_pages();

  for (int i = 0; i < ntabs; ++i)
  {
    RecordsetView* view  = recordset_view_for_widget(_rs_tabs.get_nth_page(i));
    if (view)
    {
      const Recordset::Ref model = view->model();
      if (model && model->key() == key)
      {
        close_recordset_by_ptr(view, confirm);
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void QueryView::close_recordset_by_ptr(RecordsetView* view, const bool confirm)
{
  if (view)
  {
    if (confirm && view->model()->can_close(true))
    {
      view->model()->close();
     // _rs_tabs.remove_page(*view);
    }
  }

  if (_rs_tabs.get_n_pages() == 0) // tabs are gone
    gtk_paned_set_pos_ratio(&_top_pane, 1);

  reenable_items_in_tab_menus();
}

//------------------------------------------------------------------------------
void QueryView::rs_page_switched(GtkNotebookPage *page, guint page_index)
{
  bool show_action_area = false;
  bool action_area_enabled = false;

  if (_updating_results)
    return;

  RecordsetView* rs = active_recordset();
  if (rs)
  {
    Recordset::Ref rsm = rs->model();
    _owner->be()->active_recordset(index(), rsm);
    show_action_area    = !rsm->is_readonly();
    action_area_enabled = rsm->has_pending_changes();
    _editability_label.set_tooltip_text(rsm->readonly_reason());
    _editability_icon.set_tooltip_text(rsm->readonly_reason());
  }
  else
  {
    _owner->be()->active_recordset(index(), Recordset::Ref());
  }

  if (show_action_area)
  {
    _apply_btn.show();
    _cancel_btn.show();
    _editability_label.hide();
    _editability_icon.hide();
    _btn_box.set_sensitive(action_area_enabled);
  }
  else
  {
    _apply_btn.hide();
    _cancel_btn.hide();
    _editability_label.show();
    _editability_label.set_markup("<small>ReadOnly</small>");
    _editability_icon.show();
  }
}

// Label associated with the view in gtk::notebook
//------------------------------------------------------------------------------
void QueryView::set_linked_label(ActiveLabel* lbl)
{
  _label = lbl;
}

//------------------------------------------------------------------------------
int QueryView::index()
{
  return (_owner && _owner->be()) ? _owner->be()->sql_editor_index(_editor) : -1;
}

//------------------------------------------------------------------------------
void QueryView::save()
{
  const int view_index = index();

  if (view_index >= 0)
    _owner->be()->save_sql_script_file(_owner->be()->sql_editor_path(view_index), view_index);
}

//------------------------------------------------------------------------------
std::string QueryView::editor_path()
{
  std::string  path;
  const int    view_index = index();

  if (view_index >= 0)
    path = _owner->be()->sql_editor_path(view_index);

  return path;
}

//------------------------------------------------------------------------------
void QueryView::apply_recordset_changes()
{
  RecordsetView* rs = active_recordset();
  if (rs)
  {
    Recordset::Ref rsm = rs->model();
    if (rsm->has_pending_changes())
      rsm->apply_changes();
  }
}

//------------------------------------------------------------------------------
void QueryView::discard_recordset_changes()
{
  RecordsetView* rs = active_recordset();
  if (rs)
  {
    Recordset::Ref rsm = rs->model();
    if (rsm->has_pending_changes())
      rsm->rollback();
  }
}

//------------------------------------------------------------------------------
int QueryView::process_task_msg(int msgType, const std::string &message, const std::string &detail, RecordsetView *rsv)
{
  _owner->output_text(message, true);
  return 0;
}

//------------------------------------------------------------------------------
mforms::Menu* QueryView::init_tab_menu(RecordsetView* rs)
{
  {
    mforms::Menu *m = new mforms::Menu();
    m->add_item("Close Tab", "close tab");
    m->add_item("Close Other Tabs", "close other tabs");
    return m;
  }
}

//------------------------------------------------------------------------------
void QueryView::tab_menu_handler(const std::string& action, ActiveLabel* sender, RecordsetView* rsview)
{
  if (action == "close tab")
  {
    Glib::signal_idle().connect(sigc::bind_return(sigc::bind(sigc::mem_fun(this, &QueryView::close_recordset_by_ptr), rsview, true), false));
  }
  else if (action == "close other tabs")
  {
    int size = _rs_tabs.get_n_pages();
    std::vector<RecordsetView*> to_delete;
    to_delete.reserve(size - 1);

    for (int i = 0; i < size; ++i)
    {
      RecordsetView* view  = recordset_view_for_widget(_rs_tabs.get_nth_page(i));
      if (view && rsview != view)
        to_delete.push_back(view);
    }

    size = to_delete.size();
    for (int i = 0; i < size; ++i)
      close_recordset_by_ptr(to_delete[i], true);
  }
}

//------------------------------------------------------------------------------
void QueryView::reenable_items_in_tab_menus()
{
  const int size = _rs_tabs.get_n_pages();

  for (int i = 0; i < size; ++i)
  {
    ActiveLabel* const al = (ActiveLabel*)_rs_tabs.get_nth_page(i)->get_data("active_label");
    if (al)
    {
      mforms::Menu* const menu = al->get_menu();
      if (menu)
      {
        const int index = menu->get_item_index("close other tabs");
        if (index >= 0)
          menu->set_item_enabled(index, size > 1);
      }
    }
  }
}

//------------------------------------------------------------------------------
void QueryView::start_busy()
{
  if (_label)
    _label->start_busy();
}

//------------------------------------------------------------------------------
void QueryView::stop_busy()
{
  if (_label)
    _label->stop_busy();
}

//------------------------------------------------------------------------------
void QueryView::focus()
{
  _editor->get_editor_control()->focus();
}

