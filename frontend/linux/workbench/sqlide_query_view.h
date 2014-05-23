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

#ifndef __DB_SQL_EDITOR_QUERY_VIEW_H__
#define __DB_SQL_EDITOR_QUERY_VIEW_H__

#include "mforms/toolbar.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/sql_editor_be.h"
#include "sqlide/recordset_view.h"
#include "linux_utilities/form_view_base.h"
#include "linux_utilities/notebooks.h"
#include "linux_utilities/notebook_dockingpoint.h"
//#include "gtk_helpers.h"
#include "active_label.h"
#include <glib.h>

class DbSqlEditorView;

//==============================================================================
//
//==============================================================================
class QueryView : public sigc::trackable
{
  public:
    QueryView(const int editor_index, DbSqlEditorView* owner);
    ~QueryView();
    Gtk::Widget* get_outer() {return &_top_pane;}
    void update_label(const std::string& label);
    void update_recordset_caption();
    void update_resultsets();
    void close();
    void focus();

    MySQLEditor::Ref be() { return _editor; }

    void set_linked_label(ActiveLabel* lbl); // Label associated with the view in gtk::notebook
    int index();

    void save();
    std::string editor_path();

    void reenable_items_in_tab_menus();
    void stop_busy();
    void start_busy();

  private:
    sigc::connection _gtk_load;
    void tab_reordered(Gtk::Widget*, guint)
    {
      recalc_rstab_indices();
    }
    void recalc_rstab_indices();
    void polish();
    void top_pane_changed();
    void show_find_panel(mforms::CodeEditor*, bool);
    void update_lower_notebook_visibility(bool added);

    RecordsetView* recordset_view_for_widget(Gtk::Widget *);
    RecordsetView* active_recordset();
    void close_recordset(long long key, bool confirm);
    void close_recordset_by_ptr(RecordsetView *view, const bool confirm);

    void rs_page_switched(GtkNotebookPage *page, guint index);
    void apply_recordset_changes();
    void discard_recordset_changes();
    int process_task_msg(int msgType, const std::string &message, const std::string &detail, RecordsetView *rsv);
    mforms::Menu *init_tab_menu(RecordsetView* rs);
    void tab_menu_handler(const std::string& action, ActiveLabel* sender, RecordsetView* qv);

    DbSqlEditorView         *_owner;
    MySQLEditor::Ref          _editor;
    Gtk::VBox                _editor_box;
    Gtk::VBox                _rs_box;
    Gtk::VPaned              _top_pane;
    ActiveLabel             *_label; // label from the tabswitcher/notebook
    ActionAreaNotebook       _rs_tabs;
    mforms::ToolBar::Ptr    _query_toolbar;
    Gtk::HBox               _btn_box;
    Gtk::Button             _apply_btn;
    Gtk::Button             _cancel_btn;
    Gtk::Label              _editability_label;
    Gtk::Image              _editability_icon;

    ActionAreaNotebookDockingPoint    _dock_delegate;
    mforms::DockingPoint    *_dpoint;

    sigc::connection        _polish_conn;
    bool                    _query_collapsed;
    bool                    _updating_results;
};


#endif // __DB_SQL_EDITOR_QUERY_VIEW_H__
