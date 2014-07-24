/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WBCONTEXT_SQLIDE_H_
#define _WBCONTEXT_SQLIDE_H_

#include "workbench/wb_backend_public_interface.h"
#include "grt/grt_manager.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.app.h"
//#include "dialogs/search_replace.h"
#include <boost/weak_ptr.hpp>
#include "base/trackable.h"
#include "base/notifications.h"
class SqlEditorPanel;
class SqlEditorForm;

// Central point of management for SQLIDE (sql editor)
namespace wb 
{
  class WBContextUI;
  class CommandUI;
  
  class MYSQLWBBACKEND_PUBLIC_FUNC WBContextSQLIDE : public base::trackable, base::Observer
  {
    WBContextUI *_wbui;
    std::list<boost::weak_ptr<SqlEditorForm> > _open_editors;
    app_ToolbarRef _toolbar;

    ssize_t _auto_save_interval;
    bool _auto_save_active;
    bool _option_change_signal_connected;
    
  public:
    WBContextSQLIDE(WBContextUI *wbui);
    virtual ~WBContextSQLIDE();
    void init();
    void finalize();
    
    static void detect_auto_save_files(const std::string &autosave_dir);
    static std::map<std::string, std::string> auto_save_sessions();
    
    boost::shared_ptr<SqlEditorForm> create_connected_editor(const db_mgmt_ConnectionRef &conn);

    WBContextUI *get_wbui() { return _wbui; }
    CommandUI *get_cmdui();
    bec::GRTManager *get_grt_manager();
    
    SqlEditorForm* get_active_sql_editor();

    bool activate_live_object(GrtObjectRef object);

    void open_document(const std::string &path);
    void run_file(const std::string &path);
    
    bool request_quit();

    void reconnect_editor(SqlEditorForm *editor);
    
    void editor_will_close(SqlEditorForm*); // to be called by SqlEditorForm

  public:
    void update_plugin_arguments_pool(bec::ArgumentPool &args);

    db_query_EditorRef get_grt_editor_object(SqlEditorForm *editor);
    std::list<boost::weak_ptr<SqlEditorForm> >* get_open_editors() { return &_open_editors; }
  private:
    void call_in_editor(void (SqlEditorForm::*method)());    
    void call_in_editor_str(void (SqlEditorForm::*method)(const std::string &arg), const std::string &arg);
    void call_in_editor_bool(void (SqlEditorForm::*method)(bool arg), bool arg);

    void call_in_editor_panel(void (SqlEditorPanel::*method)());
    
    bool auto_save_workspaces();
    void option_changed(grt::internal::OwnedDict*dict, bool, const std::string&key);
    
    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  };
};
                      
#endif
