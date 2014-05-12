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

#ifndef __DB_SQL_EDITOR_VIEW_H__
#define __DB_SQL_EDITOR_VIEW_H__

#include "mforms/toolbar.h"
#include "sqlide/wb_sql_editor_form.h"
#include "linux_utilities/gtk_helpers.h"
#include "linux_utilities/form_view_base.h"
#include "linux_utilities/notebooks.h"
#include "linux_utilities/notebook_dockingpoint.h"
#include "overview_panel.h"
//#include "gtk_helpers.h"
#include "active_label.h"
#include "sqlide_output_view.h"
#include <glib.h>

class SqlSnippetsView;
class ToolbarManager;
class QueryView;

//==============================================================================
//
//==============================================================================
class DbSqlEditorView : public Gtk::VBox, public FormViewBase
{
  public:
    DbSqlEditorView(SqlEditorForm::Ref editor_be);
    static DbSqlEditorView *create(SqlEditorForm::Ref editor_be);
    virtual ~DbSqlEditorView();

    virtual void init();
    virtual bool on_close();
    virtual void dispose();

    virtual bec::BaseEditor* get_be() { return NULL; }
    virtual bec::UIForm *get_form() const { return _be.get(); }
    virtual Gtk::Widget *get_panel() { return this; }
    //virtual void toggle_sidebar();

    virtual bool perform_command(const std::string &command);

    SqlEditorForm::Ref be()
    {
      return _be;
    }

    void close_editor_tab(QueryView* qv);
    void close_appview_tab(mforms::AppView *aview);
    void output_text(const std::string &text, bool bring_to_front);

    virtual bool close_focused_tab();

    QueryView *active_view();
    std::vector<QueryView*> query_views();

    bec::GRTManager* grt_manager() {return _grtm;}

  protected:
    virtual void plugin_tab_added(PluginEditorBase *plugin);


  private:
    void polish();

    bool validate_explain_sql();
    void explain_sql();

    void recordset_list_changed(int editor_index, Recordset::Ref rset, bool added);
    int on_exec_sql_done();

    void update_resultsets(int editor_index, Recordset::Ref rset, bool added);
    void update_resultsets_from_main();
    std::deque<sigc::slot<void> >  _update_resultset_slots; // Slot will have editor index, resultset attached
    base::Mutex                        _update_resultset_slots_lock;

    int  add_editor_tab(int active_sql_editor_index);
    void editor_page_switched(GtkNotebookPage *page, guint index);
    void editor_page_reordered(Gtk::Widget *page, guint index) {recalc_tab_indices();}
    void editor_page_added(Gtk::Widget *page, guint index);
    void editor_page_removed(Gtk::Widget *page, guint index);

    void partial_refresh_ui(const int what);

    QueryView *find_view_by_index(const int index);
    void recalc_tab_indices();

    mforms::Menu *init_tab_menu(Gtk::Widget *w, bool only_close_opts);
    void tab_menu_handler(const std::string& action, ActiveLabel *sender, Gtk::Widget *widget);
    void reenable_items_in_tab_menus();

    void set_maximized_editor_mode(bool flag);

    SqlEditorForm::Ref    _be;
    Gtk::HPaned           _top_pane;
    Gtk::HPaned           _top_right_pane;
    Gtk::VPaned           _main_pane;
    QueryOutputView       _output;
    Gtk::Widget          *_side_palette;

    sigc::connection      _polish_conn;
    sigc::connection      _sig_restore_sidebar;
    sigc::connection      _sig_close_editor;
    sigc::connection      _sig_close_plugin;
    sigc::connection      _sig_close_app;

    NotebookDockingPoint  _dock_delegate;
    mforms::DockingPoint  *_dpoint;

    Glib::Dispatcher      _dispatch_rset_update;
    sigc::connection      _dispatch_rset_update_conn;
    const bool            _right_aligned;
    bool                  _editor_maximized;
};


#endif // __DB_SQL_EDITOR_VIEW_H__
