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

#include "sqlide/recordset_view.h"
#include "linux_utilities/gtk_helpers.h"
#include "base/string_utilities.h"
#include "mforms/utilities.h"
#include "mforms/menubar.h"
#include "mforms/../gtk/lf_view.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("RecordsetView");

using base::strfmt;


RecordsetView * RecordsetView::create(Recordset::Ref model)
{
  RecordsetView *view= Gtk::manage(new RecordsetView(model));
  view->init();
  return view;
}

RecordsetView::RecordsetView(Recordset::Ref model)
:
_grid(NULL), _single_row_height(-1)
{
  this->model(model);
}

RecordsetView::~RecordsetView()
{
  _refresh_ui_sig.disconnect();
  _refresh_ui_stat_sig.disconnect();
}

void RecordsetView::init()
{
  _grid= GridView::create(_model, false); // XXX put back to true when column-autosizing is fixed
  _grid->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

  _grid->_copy_func_ptr = sigc::mem_fun(this,&RecordsetView::copy);

  add(*_grid);
  show_all();

  ActionList &action_list= _model->action_list();
  action_list.register_action("record_first", sigc::mem_fun(this, &RecordsetView::on_goto_first_row_btn_clicked));
  action_list.register_action("record_back", sigc::mem_fun(this, &RecordsetView::on_record_prev));
  action_list.register_action("record_next", sigc::mem_fun(this, &RecordsetView::on_record_next));
  action_list.register_action("record_last", sigc::mem_fun(this, &RecordsetView::on_goto_last_row_btn_clicked));
  //action_list.register_action("record_fetch_all", sigc::mem_fun(this, &RecordsetView::));
  action_list.register_action("record_wrap_vertical", sigc::mem_fun(this, &RecordsetView::on_toggle_vertical_sizing));
  action_list.register_action("record_sort_asc", sigc::mem_fun(this, &RecordsetView::on_record_sort_asc));
  action_list.register_action("record_sort_desc", sigc::mem_fun(this, &RecordsetView::on_record_sort_desc));
  //action_list.register_action("record_refresh", sigc::mem_fun(this, &RecordsetView::));

  mforms::ToolBar *tbar = _model->get_toolbar();
  if (tbar->find_item("record_edit"))
  {
    tbar->find_item("record_edit")->signal_activated()->connect(boost::bind(&RecordsetView::on_record_edit, this));
    tbar->find_item("record_add")->signal_activated()->connect(boost::bind(&RecordsetView::on_record_add, this));
    tbar->find_item("record_del")->signal_activated()->connect(boost::bind(&RecordsetView::on_record_del, this));
  }

  _grid->signal_event().connect(sigc::mem_fun(this, &RecordsetView::on_event));

  _model->update_edited_field = boost::bind(&RecordsetView::selected_record_changed, this);
}

void RecordsetView::model(Recordset::Ref value)
{
  _model= value;
  _refresh_ui_sig = _model->refresh_ui_signal.connect(sigc::mem_fun(this, &RecordsetView::refresh));
  _model->update_edited_field = boost::bind(&RecordsetView::selected_record_changed, this);
  //_model->task->msg_cb(sigc::mem_fun(this, &RecordsetView::process_task_msg));

  if (_grid)
    _grid->model(_model);
}

void RecordsetView::copy(const std::vector<int> &rows)
{
  if (_model)
    _model->copy_rows_to_clipboard(rows, ", ");
}


bool RecordsetView::activate_toolbar_item(const std::string &action)
{
  try
  {
    bool r = _model->action_list().trigger_action(action);
    return r;
  }
  catch (const std::exception &exc)
  {
    log_error("Unhandled exception in activate_toolbar_item(%s): %s", action.c_str(), exc.what());
    mforms::Utilities::show_error(_("Unhandled Error"), exc.what(), "OK", "", "");
  }
  return false;
}


void RecordsetView::reset()
{
  _model->reset();
}

void RecordsetView::refresh()
{
  _grid->refresh(true);

  // calculate the height of a row with single line of text
  if (_grid->view_model()->row_numbers_visible())
  {
    Gtk::TreeViewColumn *col = _grid->get_column(0);
    Gtk::CellRenderer *rend = col ? col->get_first_cell_renderer() : 0;
    if (rend)
    {
      int x, y, w, h;
      rend->get_size(*_grid, x, y, w, h);
      _single_row_height = h;
    }
  }
 
  if (_grid->get_fixed_height_mode())
    set_fixed_row_height(_single_row_height);
  else
    set_fixed_row_height(-1);
}


bool RecordsetView::on_event(GdkEvent *event)
{
  bool processed= false;

  if (GDK_BUTTON_PRESS == event->type && 3 == event->button.button)
  {
    std::vector<int> rows = _grid->get_selected_rows();
    Gtk::TreePath path;
    Gtk::TreeViewColumn *column;

    _grid->grab_focus();
    int cell_x, cell_y;
    if (_grid->get_path_at_pos(event->button.x, event->button.y, path, column, cell_x, cell_y))
    {
      // if we clicked on an unselected item, then select it otherwise keep original selection state
      if (std::find(rows.begin(), rows.end(), path[0]) == rows.end())
      {
        if (_grid->allow_cell_selection() && column != _grid->get_column(0))
        {
          _grid->select_cell(path[0], *column);
          _grid->get_selection()->unselect_all();
          rows.clear();
          rows.push_back(path[0]);
        }
        else
          _grid->select_cell(path[0], -1);
      }
      else
      {
        // the right-clicked row is selected, so keep the selection as is
      }
    }

    int row, col;
    _grid->current_cell(row, col);

    _model->update_selection_for_menu(rows, col);
    _model->get_context_menu()->popup_at(event->button.x, event->button.y);

    processed= true;
  }

  if (!processed)
    processed= Gtk::ScrolledWindow::on_event(event);

  return processed;
}

void RecordsetView::selected_record_changed()
{
  _grid->get_selection()->unselect_all();
  _grid->select_cell(_model->edited_field_row(), _model->edited_field_column());
}

void RecordsetView::on_commit_btn_clicked()
{
  _model->apply_changes();
}

void RecordsetView::on_rollback_btn_clicked()
{
  _model->rollback();
}

bool RecordsetView::has_changes()
{
  return _model->has_pending_changes();
}

void RecordsetView::on_goto_first_row_btn_clicked()
{
  if (_model->row_count() == 0)
    return;
    
  Gtk::TreePath tree_path(1);
  tree_path[0]= 0;
  _grid->set_cursor(tree_path);
}

void RecordsetView::on_goto_last_row_btn_clicked()
{
  Gtk::TreePath tree_path(1);
  size_t row_count= _model->row_count();
  if (row_count == 0)
    return;
  tree_path[0] = row_count - 1;
  _grid->set_cursor(tree_path);
}

void RecordsetView::on_record_prev()
{
  Gtk::TreeModel::Path path;
  Gtk::TreeViewColumn *column= NULL;
  _grid->get_cursor(path, column);
  if (!column)
    return;
  path.prev();
  _grid->set_cursor(path, *column);
}

void RecordsetView::on_record_next()
{
  Gtk::TreeModel::Path path;
  Gtk::TreeViewColumn *column= NULL;
  _grid->get_cursor(path, column);
  if (!column)
    return;
  path.next();
  _grid->set_cursor(path, *column);
}

void RecordsetView::on_record_edit()
{
  if (_model->is_readonly())
    return;
  Gtk::TreeModel::Path path;
  Gtk::TreeViewColumn *column= NULL;
  _grid->get_cursor(path, column);
  if (!column)
    return;
  _grid->set_cursor(path, *column, true);
}

void RecordsetView::on_record_add()
{
  if (_model->is_readonly())
    return;
  Gtk::TreePath tree_path(1);
  size_t row_count= _model->row_count();
  if (row_count == 0)
    return;
  tree_path[0] = row_count;
  _grid->set_cursor(tree_path);
  on_record_edit();
}

void RecordsetView::on_record_del()
{
  if (_model->is_readonly())
    return;
  std::vector<int> rows = _grid->get_selected_rows();
  std::vector<bec::NodeId> nodes;
  for (size_t i = 0; i < rows.size(); i++)
    nodes.push_back(rows[i]);
  if (nodes.empty())
  {
    Gtk::TreeModel::Path path;
    Gtk::TreeViewColumn *column= NULL;
    _grid->get_cursor(path, column);
    nodes.push_back(path.front());
  }
//  _grid->delete_selected_rows();
  _model->delete_nodes(nodes);
  _grid->queue_draw();
}

void RecordsetView::on_record_sort_asc()
{
  int row, col;
  _grid->current_cell(row, col);
  if (col < 0)
    return;
  _grid->sort_by_column(col, -1, true);
}

void RecordsetView::on_record_sort_desc()
{
  int row, col;
  _grid->current_cell(row, col);
  if (col < 0)
    return;
  _grid->sort_by_column(col, 1, true);
}

void RecordsetView::on_toggle_vertical_sizing()
{
  if (!_grid->get_fixed_height_mode())
  {
    std::vector<Gtk::TreeViewColumn*> columns = _grid->get_columns();
    for (std::vector<Gtk::TreeViewColumn*>::iterator iter = columns.begin(); iter != columns.end(); ++iter)
      (*iter)->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
  }
  _grid->set_fixed_height_mode(!_grid->get_fixed_height_mode());
  refresh();
}

void RecordsetView::set_fixed_row_height(int height)
{
  if (_grid && _grid->view_model())
  {
    std::vector<Gtk::TreeViewColumn*> columns = _grid->get_columns();
    if (_grid->view_model()->row_numbers_visible())
      columns.erase(columns.begin());

    for (std::vector<Gtk::TreeViewColumn*>::iterator iter = columns.begin(); iter != columns.end(); ++iter)
    {
      std::vector<Gtk::CellRenderer*> cells((*iter)->get_cell_renderers());
      for (std::vector<Gtk::CellRenderer*>::iterator cell = cells.begin(); cell != cells.end(); ++cell)
        (*cell)->set_fixed_size(-1, height);
    }
  }
}

