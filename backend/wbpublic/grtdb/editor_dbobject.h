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

#pragma once

#include "grtpp_notifications.h"

#include "wbpublic_public_interface.h"
#include "grt/editor_base.h"

#include "grtsqlparser/mysql_parser_services.h"

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC DBObjectEditorBE : public BaseEditor, public grt::GRTObserver {
  public:
    virtual ~DBObjectEditorBE();

    virtual bool should_close_on_delete_of(const std::string &oid);

    virtual db_DatabaseObjectRef get_dbobject() {
      return db_DatabaseObjectRef::cast_from(get_object());
    };

    virtual std::string get_name();
    virtual void set_name(const std::string &name);

    virtual std::string get_comment();
    virtual void set_comment(const std::string &descr);

    virtual std::string get_sql();
    virtual void set_sql(const std::string &sql);

    virtual bool is_sql_commented();
    virtual void set_sql_commented(bool flag);

    virtual bool has_editor();
    virtual MySQLEditor::Ref get_sql_editor();
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
    virtual std::vector<std::string> get_charset_list();
    virtual std::vector<std::string> get_charset_collation_list(const std::string &charset);
    virtual std::vector<std::string> get_charset_collation_list();
    bool parse_charset_collation(const std::string &str, std::string &charset, std::string &collation);
    std::string format_charset_collation(const std::string &charset, const std::string &collation);

    void update_change_date();
    void send_refresh();
    void set_sql_mode(const std::string &value);

    virtual bool is_editing_live_object();
    virtual void apply_changes_to_live_object();
    virtual void refresh_live_object();
    virtual bool can_close();

    std::function<bool(DBObjectEditorBE *, bool)> on_apply_changes_to_live_object;
    std::function<void(DBObjectEditorBE *)> on_refresh_live_object;
    std::function<void(DBObjectEditorBE *)> on_create_live_table_stubs;
    std::function<bool(DBObjectEditorBE *, std::string &, std::string &)> on_expand_live_table_stub;

  protected:
    parsers::MySQLParserContext::Ref _parserContext;
    parsers::MySQLParserContext::Ref _autocompletionContext;
    parsers::MySQLParserServices::Ref _parserServices;
    parsers::SymbolTable *_globalSymbols;

    DBObjectEditorBE(const db_DatabaseObjectRef &object);

  private:
    MySQLEditor::Ref _sql_editor;
    db_CatalogRef _catalog;

    boost::signals2::scoped_connection _val_notify_conn;
    void notify_from_validation(const grt::Validator::Tag &tag, const grt::ObjectRef &, const std::string &,
                                const int level); // level is grt::MessageType
    // Real-time validation part
    grt::MessageType _last_validation_check_status;
    std::string _last_validation_message;

    virtual void handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info);
  };
};
