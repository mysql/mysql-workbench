/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _GRTDBCONNECTPANEL_H_
#define _GRTDBCONNECTPANEL_H_

#include <mforms/box.h>
#include <mforms/table.h>
#include <mforms/label.h>
#include <mforms/textentry.h>
#include <mforms/selector.h>
#include <mforms/panel.h>
#include <mforms/tabview.h>

#include "db_conn_be.h"

namespace grtui {

  enum {
    DbConnectPanelShowConnectionCombo = (1 << 0),
    DbConnectPanelShowRDBMSCombo = (1 << 1),
    DbConnectPanelShowManageConnections = (1 << 2),
    DbConnectPanelHideConnectionName = (1 << 3),
    DbConnectPanelDontSetDefaultConnection = (1 << 4),
    DbConnectPanelDefaults = (DbConnectPanelShowConnectionCombo | DbConnectPanelShowManageConnections)
  };
  typedef int DbConnectPanelFlags;

  class WBPUBLICBACKEND_PUBLIC_FUNC DbConnectPanel : public mforms::Box {
  public:
    DbConnectPanel(DbConnectPanelFlags = DbConnectPanelDefaults);
    virtual ~DbConnectPanel();

    void init(const db_mgmt_ManagementRef &mgmt, const grt::ListRef<db_mgmt_Rdbms> &allowed_rdbms,
              const db_mgmt_ConnectionRef &default_conn = db_mgmt_ConnectionRef());
    void init(const db_mgmt_ManagementRef &mgmt, const db_mgmt_ConnectionRef &default_conn = db_mgmt_ConnectionRef());

    void init(DbConnection *conn, const db_mgmt_ConnectionRef &default_conn = db_mgmt_ConnectionRef());

    static bool is_connectable_driver_type(db_mgmt_DriverRef driver);

    void set_default_host_name(const std::string &host, bool update = false);
    std::string default_host_name() {
      return _default_host_name;
    }

    void set_skip_schema_name(bool flag);

    void set_enabled(bool flag);

    mforms::TextEntry *get_name_entry() {
      return &_name_entry;
    }

    DbConnection *get_be() const {
      return _connection;
    }

    void set_active_stored_conn(const std::string &name);
    void set_active_stored_conn(db_mgmt_ConnectionRef connection);

    db_mgmt_ConnectionRef get_default_connection() {
      return _anonymous_connection;
    }

    db_mgmt_ConnectionRef get_connection(bool initInvalid = false);
    void set_connection(const db_mgmt_ConnectionRef &conn);

    boost::signals2::signal<void(std::string, bool)> *signal_validation_state_changed() {
      return &_signal_validation_state_changed;
    }

    void save_connection_as(const std::string &name);

    void set_driver_changed_cb(const std::function<void(db_mgmt_DriverRef)> &cb) {
      _driver_changed_cb = cb;
    };

    bool test_connection();

    void connection_user_input(std::string &text_entry, bool &create_group, bool new_entry = true);

    db_mgmt_RdbmsRef selected_rdbms();
    db_mgmt_DriverRef selected_driver();

  protected:
    grt::ListRef<db_mgmt_Rdbms> _allowed_rdbms;
    DbConnection *_connection;
    db_mgmt_ConnectionRef _anonymous_connection;
    std::map<std::string, grt::DictRef> _parameters_per_driver;
    std::string _default_host_name;

    mforms::Table _table;
    mforms::Label _label1;
    mforms::Label _label2;
    mforms::Label _label3;

    mforms::TextEntry _name_entry;
    mforms::Selector _stored_connection_sel;
    mforms::Selector _rdbms_sel;
    mforms::Selector _driver_sel;
    mforms::Label _desc1;
    mforms::Label _desc2;
    mforms::Label _desc3;

    mforms::TabView _tab;

    mforms::Box _content;

    mforms::Panel _params_panel;
    mforms::Table *_params_table;
    std::vector<mforms::Box *> _param_rows;

    mforms::Panel _ssl_panel;
    mforms::Table *_ssl_table;
    std::vector<mforms::Box *> _ssl_rows;

    mforms::Panel _advanced_panel;
    mforms::Table *_advanced_table;
    std::vector<mforms::Box *> _advanced_rows;

    mforms::Panel _options_panel;
    mforms::Table *_options_table;
    std::vector<mforms::Box *> _options_rows;

    std::list<mforms::View *> _views;
    mforms::Label _warning;

  private:
    void save_param(const std::string &name, const grt::StringRef &param);
    std::string get_saved_param(const std::string &name);

    boost::signals2::signal<void(std::string, bool)> _signal_validation_state_changed;

    bool _initialized;
    bool _create_group;
    bool _delete_connection_be;
    bool _show_connection_combo;
    bool _show_manage_connections;
    bool _allow_edit_connections;
    bool _updating;
    bool _skip_schema_name;
    bool _dont_set_default_connection;
    std::string _last_validation;

    int _last_active_tab;

    void suspend_view_layout(bool flag);
    void begin_layout();
    void end_layout();
    void create_control(DbDriverParam *driver_param, ControlType ctrl_type, const base::ControlBounds &bounds,
                        const std::string &caption);

    void change_active_rdbms();
    void change_active_driver();

    void set_keychain_password(DbDriverParam *param, bool clear);

    void param_value_changed(mforms::View *sender, bool trim_whitespace);
    void enum_param_value_changed(mforms::Selector *sender, std::vector<std::string> options);

    void refresh_stored_connections();

    void change_active_stored_conn();
    void reset_stored_conn_list();
    void change_connection_name();

    void launch_ssl_wizard();
    void open_ssl_wizard_directory();

    grt::ListRef<db_mgmt_Connection> connection_list();

    db_mgmt_ConnectionRef open_editor();

    grt::StringListRef get_enum_values(db_mgmt_DriverParameterRef param);

    std::function<void(const db_mgmt_DriverRef &)> _driver_changed_cb;
  };
};

#endif /* _GRTDBCONNECTFORM_H_ */
