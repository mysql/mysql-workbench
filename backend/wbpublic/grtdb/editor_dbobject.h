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

#pragma once

#include "wbpublic_public_interface.h"
#include "grt/editor_base.h"

#include "grtsqlparser/sql_facade.h"
#include "grtsqlparser/invalid_sql_parser.h" // XXX: to go

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC DBObjectEditorBE : public BaseEditor
  {
  public:
    virtual bool should_close_on_delete_of(const std::string &oid);

    virtual GrtObjectRef get_object() { return get_dbobject(); }
    virtual db_DatabaseObjectRef get_dbobject()= 0;
    virtual db_mgmt_RdbmsRef get_rdbms() { return _rdbms; }
    GrtVersionRef get_rdbms_target_version();

    virtual std::string get_name();
    virtual void set_name(const std::string &name);

    virtual std::string get_comment();
    virtual void set_comment(const std::string &descr);

    virtual std::string get_sql();
    virtual bool is_sql_commented();
    virtual void set_sql_commented(bool flag);

    virtual Sql_editor::Ref get_sql_editor();
    virtual void reset_editor_undo_stack();

    db_SchemaRef get_schema();
    virtual std::string get_schema_name();

    db_CatalogRef get_catalog();
    
    db_SchemaRef get_schema_with_name(const std::string &schema_name);

    virtual std::vector<std::string> get_all_table_names();
    virtual std::vector<std::string> get_all_schema_names();
    virtual std::vector<std::string> get_schema_table_names();
    virtual std::vector<std::string> get_table_column_names(const std::string &table_name);
    virtual std::vector<std::string> get_table_column_names(const db_TableRef &table);

    // charsets and collations
    virtual std::vector<std::string> get_charset_collation_list();
    bool parse_charset_collation(const std::string &str, std::string &charset, std::string &collation);
    std::string format_charset_collation(const std::string &charset, const std::string &collation);

    void update_change_date();
    void sql_mode(const std::string &value);

  protected:
    db_mgmt_RdbmsRef _rdbms;
    Sql_editor::Ref _sql_editor;

    DBObjectEditorBE(GRTManager *grtm, const db_DatabaseObjectRef &object, const db_mgmt_RdbmsRef &rdbms);

    virtual std::string get_object_type();

  public:
    void check_sql();

  public:
    typedef std::vector<std::string> Log_messages;
    typedef boost::function<void (const Log_messages&)> Sql_parser_log_cb;
    typedef Sql_parser_base::Parse_error_cb Sql_parser_err_cb;

    void set_sql_parser_log_cb(const Sql_parser_log_cb &cb);
    void set_sql_parser_err_cb(const Sql_parser_err_cb &cb);
    void set_sql(const std::string &sql, bool sync, const db_DatabaseObjectRef &template_obj, const std::string &comment= "");

  protected:
    typedef boost::function<grt::ValueRef (grt::GRT*, grt::StringRef)> Sql_parser_task_cb;
    void set_sql_parser_task_cb(const Sql_parser_task_cb &cb);
    Invalid_sql_parser::Ref _sql_parser;
    SqlFacade::Ref _parsing_services;
    std::string _non_std_sql_delimiter;

  private:

    void sql_parser_task_finished_cb(grt::ValueRef value);
    void sql_parser_msg_cb(const grt::Message &msg);

    Sql_parser_task_cb _sql_parser_task_cb;
    Log_messages _sql_parser_log;
    Sql_parser_log_cb _sql_parser_log_cb;
    Sql_parser_err_cb _sql_parser_err_cb;

  public:
    virtual bool is_editing_live_object();
    virtual void apply_changes_to_live_object();
    virtual void refresh_live_object();
    bool is_server_version_at_least(int major, int minor);

    boost::function<bool (DBObjectEditorBE*, bool)> on_apply_changes_to_live_object;
    boost::function<void (DBObjectEditorBE*)> on_refresh_live_object;
    boost::function<void (DBObjectEditorBE*)> on_create_live_table_stubs;
    boost::function<bool (DBObjectEditorBE*, std::string&, std::string&)> on_expand_live_table_stub;

  public:
    virtual bool can_close();

  private:
    boost::signals2::scoped_connection _val_notify_conn;
    void notify_from_validation(const grt::Validator::Tag& tag, const grt::ObjectRef&, const std::string&, const int level);//level is grt::MessageType
    // Real-time validation part
    grt::MessageType    _last_validation_check_status;
    std::string         _last_validation_message;
    
    bool custom_string_compare(const std::string &first, const std::string &second);
  };
};
