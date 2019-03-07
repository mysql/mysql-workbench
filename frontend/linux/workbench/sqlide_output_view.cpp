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

#include "gtk/lf_mforms.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "base/wb_iterators.h"
#include "base/log.h"
#include "gtk/lf_view.h"
#include "objimpl/wrapper/mforms_ObjectReference_impl.h"
#include "sqlide_form.h"
#include "sqlide/wb_sql_editor_panel.h"

using base::strfmt;

//==============================================================================
//
//==============================================================================

void drop_eol(const int column, Glib::ValueBase* vbase) {
  if (column == 7) {
    GValue* vb = vbase->gobj();
    char* str = g_value_dup_string(vb);
    char* tstr = str;
    while (tstr && *tstr++) {
      if (*tstr == '\n')
        *tstr = ' ';
    }
    g_value_take_string(vb, str);
  }
}

static const std::vector<bec::NodeId> selected_nodeids(GridView& g) {
  std::vector<int> rows = g.get_selected_rows();

  std::vector<bec::NodeId> entries;
  entries.reserve(rows.size());

  for (base::const_range<std::vector<int> > it(rows); it; ++it)
    entries.push_back((bec::NodeId)*it);

  return entries;
}

//==============================================================================
//
//==============================================================================
struct SigcBlocker {
  SigcBlocker(sigc::connection& c) : _c(c) {
    _c.block();
  }
  ~SigcBlocker() {
    _c.unblock();
  }

  sigc::connection& _c;
};

static void copy_accessibility_name(Gtk::Widget& w) {
  Glib::RefPtr<Atk::Object> acc = w.get_accessible();
  if (acc)
    acc->set_name(w.get_name());
}

//==============================================================================
//
//==============================================================================
QueryOutputView::QueryOutputView(const SqlEditorForm::Ref& be, DbSqlEditorView* db_sql_editor_view)
  : _be(be),
    _top_box(Gtk::ORIENTATION_VERTICAL),
    _action_output(be->log(), true, false),
    _history_box(Gtk::ORIENTATION_HORIZONTAL),
    _entries_grid(be->history()->entries_model(), true, false),
    _details_grid(be->history()->details_model(), true, false),
    _db_sql_editor_view(db_sql_editor_view) {
  const char* const sections[] = {"Action Output", "Text Output", "History"};
  _action_output.show();
  _action_output.row_numbers_visible(false);
  _action_output.set_fixed_height_mode(true);
  _action_output.refresh(true);
  _action_output.view_model()->before_render = sigc::ptr_fun(drop_eol);
  _action_output.set_has_tooltip(true);
  _action_output.signal_query_tooltip().connect(sigc::mem_fun(this, &QueryOutputView::on_query_tooltip));
  _action_output.set_name("Action Output");
  copy_accessibility_name(_action_output);

  auto acc = _top_box.get_accessible();
  if (acc)
    acc->set_name("Action Output Area");
  _action_swnd.add(_action_output);
  _action_swnd.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

  {
    Gtk::TreeViewColumn* col;
    col = _action_output.get_column(0); // icon
    if (col)
      col->set_resizable(false);
    col = _action_output.get_column(1); // index
    if (col)
      col->set_fixed_width(40);
    col = _action_output.get_column(2); // time
    if (col)
      col->set_fixed_width(80);
    col = _action_output.get_column(3); // action
    if (col) {
      col->set_fixed_width(400);
    }
    col = _action_output.get_column(4); // message
    if (col) {
      col->set_fixed_width(350);
    }
    col = _action_output.get_column(5); // duration
    if (col) {
      col->set_fixed_width(150);
      col->set_resizable(false);
    }
  }

  mforms::Menu* context_menu = be->log()->get_context_menu();
  _action_output.set_context_menu(context_menu);
  context_menu->signal_will_show()->connect(std::bind(&QueryOutputView::output_menu_will_show, this));

  _entries_grid.set_context_menu_responder(sigc::mem_fun(this, &QueryOutputView::history_context_menu_responder));

  context_menu = be->history()->details_model()->get_context_menu();
  context_menu->set_handler(std::bind(&QueryOutputView::handle_history_context_menu, this, std::placeholders::_1));
  _details_grid.set_context_menu(context_menu);

  _refresh_ui_sig_entries = _be->history()->entries_model()->refresh_ui_signal.connect(
    sigc::mem_fun(this, &QueryOutputView::on_history_entries_refresh));
  _refresh_ui_sig_details = _be->history()->details_model()->refresh_ui_signal.connect(
    sigc::mem_fun(this, &QueryOutputView::on_history_details_refresh));

  _on_history_entries_selection_changed_conn = _entries_grid.get_selection()->signal_changed().connect(
    sigc::mem_fun(this, &QueryOutputView::on_history_entries_selection_changed));

  _entries_grid.refresh(true);
  _details_grid.refresh(true);

  for (size_t i = 0; i < (sizeof(sections) / sizeof(const char* const)); ++i)
    _mode.append(sections[i]);

  _text_output.set_name("Text Output");
  copy_accessibility_name(_text_output);

  _text_swnd.add(_text_output);
  _text_swnd.show_all();

  _entries_grid.set_name("History Entries");
  copy_accessibility_name(_entries_grid);
  _details_grid.set_name("History Details");
  copy_accessibility_name(_details_grid);

  _entries_swnd.add(_entries_grid);
  _details_swnd.add(_details_grid);

  _entries_swnd.show_all();
  _details_swnd.show_all();

  _history_box.pack1(_entries_swnd, Gtk::FILL);
  _entries_swnd.set_size_request(100, -1);
  _history_box.pack2(_details_swnd, Gtk::EXPAND);
  _history_box.show_all();

  _note.append_page(_action_swnd, sections[0]);
  _note.append_page(_text_swnd, sections[1]);
  _note.append_page(_history_box, sections[2]);
  _note.show_all();

  _note.set_show_tabs(false);

  Gtk::Box* mode_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
  Gtk::Label* spacer = Gtk::manage(new Gtk::Label());
  mode_box->pack_start(_mode, false, true);
  mode_box->pack_start(*spacer, true, true);
  _mode.property_has_frame() = false;

  _top_box.pack_start(*mode_box, false, true);
  _top_box.pack_start(_note, true, true);
  _top_box.show_all();

  _refresh_ui_sig_log =
    _be->log()->refresh_ui_signal.connect(sigc::bind<bool>(sigc::mem_fun(_action_output, &GridView::refresh), false));

  _mode.signal_changed().connect(sigc::mem_fun(this, &QueryOutputView::mode_change_requested));
  _mode.set_active(0);
}

QueryOutputView::~QueryOutputView() {
  _refresh_ui_sig_log.disconnect();
  _refresh_ui_sig_entries.disconnect();
  _refresh_ui_sig_details.disconnect();
}
//------------------------------------------------------------------------------
bool QueryOutputView::on_query_tooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
  Gtk::TreePath path;
  int bx, by;
  _action_output.convert_widget_to_bin_window_coords(x, y, bx, by);
  if (_action_output.get_path_at_pos(bx, by, path)) {
    std::string action, response, duration;
    bec::NodeId node(path[0]);
    _be->log()->get_field(node, 3, action);
    _be->log()->get_field(node, 4, response);
    _be->log()->get_field(node, 5, duration);

    if (duration.empty())
      tooltip->set_markup(base::strfmt("<b>Action:</b> %s\n<b>Response:</b> %s", action.c_str(), response.c_str()));
    else
      tooltip->set_markup(base::strfmt("<b>Action:</b> %s\n<b>Response:</b> %s\n<b>Duration:</b> %s", action.c_str(),
                                       response.c_str(), duration.c_str()));
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void QueryOutputView::mode_change_requested() {
  const int mode = _mode.get_active_row_number();
  if (mode >= 0)
    _note.set_current_page(mode);
}

//------------------------------------------------------------------------------
void QueryOutputView::refresh() {
  const int mode = _mode.get_active_row_number();
  switch (mode) {
    case 2: // History output
    {
      _entries_grid.scroll_to(0); // newest entry is always first
      _details_grid.scroll_to(1);
    }
    /* fall-thru */
    case 0: // Action Output - always need refresh even if it's not visible
    /* fall-thru */
    case 1: // Text output
    {
      _action_output.refresh(false);

      const int log_row_count = _action_output.row_count();
      if (log_row_count > 0) {
        Gtk::TreePath path;
        path.push_back(log_row_count - 1);
        _action_output.scroll_to_row(path);
        _action_output.set_cursor(path);
      }
      break;
    }
    /* fall-thru */
    default:
      break;
  }
}

//------------------------------------------------------------------------------
int QueryOutputView::on_history_entries_refresh() {
  SigcBlocker signal_block(_on_history_entries_selection_changed_conn);

  _entries_grid.refresh(false);
  _entries_grid.scroll_to(0);

  return 0;
}

//------------------------------------------------------------------------------
int QueryOutputView::on_history_details_refresh() {
  SigcBlocker signal_block(_on_history_entries_selection_changed_conn);

  _details_grid.refresh(false);
  _details_grid.scroll_to(1);

  return 0;
}

//------------------------------------------------------------------------------
void QueryOutputView::on_history_entries_selection_changed() {
  const int row = _entries_grid.current_row();
  if (-1 < row) {
    _be->history()->current_entry(row);
    _details_grid.refresh(false);
  }
}

//------------------------------------------------------------------------------
void QueryOutputView::output_menu_will_show() {
  std::vector<int> sel_indices = _action_output.get_selected_rows();
  _be->log()->set_selection(sel_indices);
}

//------------------------------------------------------------------------------
void QueryOutputView::handle_history_context_menu(const std::string& action) {
  DbSqlEditorHistory::EntriesModel::Ref entries_model = _be->history()->entries_model();

  const std::vector<bec::NodeId> entries = selected_nodeids(_entries_grid);

  if (action == "delete_selection") {
    entries_model->activate_popup_item_for_nodes(action, entries);
    _entries_grid.refresh(false);
  } else if (action == "delete_all") {
    entries_model->activate_popup_item_for_nodes(action, entries);
    _entries_grid.refresh(false);
  } else {
    const int selected_entry = (entries.size() > 0) ? (*entries.begin())[0] : -1;

    if (selected_entry >= 0) {
      if (action == "clear") {
        {
          std::vector<size_t> e(1, selected_entry);
          entries_model->delete_entries(e);
          _entries_grid.refresh(false);
        }
      } else {
        std::vector<int> rows = _details_grid.get_selected_rows();
        std::list<int> details;

        for (base::const_range<std::vector<int> > it(rows); it; ++it)
          details.push_back(*it);

        const std::string sql = _db_sql_editor_view->be()->restore_sql_from_history(selected_entry, details);

        if (action == "append_selected_items") {
          SqlEditorPanel* qv = _be->active_sql_editor_panel();
          if (qv)
            qv->editor_be()->append_text(sql);
        } else if (action == "replace_sql_script") {
          SqlEditorPanel* qv = _be->active_sql_editor_panel();
          if (qv)
            qv->editor_be()->sql(sql.c_str());
        } else if (action == "copy_row") {
          Glib::RefPtr<Gtk::Clipboard> clip = Gtk::Clipboard::get();
          if (clip)
            clip->set_text(sql);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void QueryOutputView::history_context_menu_responder() {
  const std::vector<bec::NodeId> entries = selected_nodeids(_entries_grid);
  const bec::MenuItemList menuitems = _be->history()->entries_model()->get_popup_items_for_nodes(entries);

  run_popup_menu(menuitems, gtk_get_current_event_time(),
                 sigc::mem_fun(this, &QueryOutputView::handle_history_context_menu), &_context_menu);
}

//------------------------------------------------------------------------------
void QueryOutputView::output_text(const std::string& text, const bool bring_to_front) {
  Glib::RefPtr<Gtk::TextBuffer> buf = _text_output.get_buffer();
  buf->insert(buf->end(), text);

  if (bring_to_front)
    _mode.set_active(1); // 1 - Text output tab
}
