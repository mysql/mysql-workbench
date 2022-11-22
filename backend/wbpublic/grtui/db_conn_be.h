/*
 * Copyright (c) 2007, 2022, Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.db.mgmt.h"
#include "cppdbc.h"
#include <vector>
#include <map>

#include "wbpublic_public_interface.h"
#include "base/geometry.h"

class DbDriverParam;
class DbDriverParams;
class DbConnection;

enum ControlType {
  ctUnknown,
  ctLabel,
  ctDescriptionLabel,
  ctTextBox,
  ctKeychainPassword,
  ctCheckBox,
  ctNumericUpDown,
  ctButton,
  ctDirSelector,
  ctFileSelector,
  ctEnumSelector,
  ctEnumOption,
  ctText
};

class WBPUBLICBACKEND_PUBLIC_FUNC DbDriverParam {
public:
  enum ParamType {
    ptUnknown,
    ptInt,
    ptString,
    ptPassword,
    ptKeychainPassword,
    ptBoolean,
    ptTristate,
    ptDir,
    ptFile,
    ptEnum,
    ptIntEnum,
    ptIntOption,
    ptText,
    ptButton
  };

private:
  static ParamType decode_param_type(std::string type_name, std::string real_type);

  db_mgmt_DriverParameterRef _inner;
  ParamType _type;
  grt::ValueRef _value;

  DbDriverParam(const DbDriverParam &) {
  }
  DbDriverParam(const db_mgmt_DriverParameterRef &driver_param, const db_mgmt_ConnectionRef &stored_conn);
  DbDriverParam(const db_mgmt_DriverParameterRef &driver_param, const grt::ValueRef &value);

  ControlType get_control_type() const;

  friend class DbDriverParams;

public:
  const db_mgmt_DriverParameterRef &object() const {
    return _inner;
  }

  ParamType get_type() const {
    return _type;
  }
  grt::StringRef get_control_name() const;
  grt::StringRef get_accessibility_name() const;
  const grt::ValueRef &get_value() const {
    return _value;
  }
  const grt::StringRef get_value_repr() const {
    return _value.toString();
  }
  void set_value(const grt::ValueRef &value);
  std::vector<std::pair<std::string, std::string> > get_enum_options();
  grt::StringRef getValue();
};

class WBPUBLICBACKEND_PUBLIC_FUNC DbDriverParams {
private:
  typedef std::vector<DbDriverParam *> Collection;
  typedef std::map<std::string, DbDriverParam *> String_index;

  Collection _collection;
  String_index _control_name_index;
  db_mgmt_DriverRef _driver;

  DbDriverParams(const DbDriverParams &) {
  }
  void free_dyn_mem();

  bool parameter_not_valid(const db_mgmt_DriverRef &driver, const std::string &param);

public:
  DbDriverParams() {
  }
  ~DbDriverParams() {
    free_dyn_mem();
  }

  void init(const db_mgmt_DriverRef &driver, const db_mgmt_ConnectionRef &stored_conn,
            const std::function<void(bool)> &suspend_layout, const std::function<void()> &begin_layout,
            const std::function<void(DbDriverParam *, ControlType, const base::ControlBounds &, const std::string &)>
              &create_control,
            const std::function<void()> &end_layout, bool skip_schema = false, int first_row_label_width = 100,
            int hmargin = 10, int vmargin = 10);
  grt::DictRef get_params() const;
  std::string validate() const;

  size_t count() const {
    return _collection.size();
  }
  DbDriverParam *get(std::string control_name);
};

class WBPUBLICBACKEND_PUBLIC_FUNC DbConnection {
private:
  db_mgmt_ManagementRef _mgmt;
  DbDriverParams _db_driver_param_handles;
  db_mgmt_DriverRef _active_driver;
  db_mgmt_ConnectionRef _connection;
  bool _skip_schema;

  std::function<void()> _begin_layout;
  std::function<void()> _end_layout;
  std::function<void(bool)> _suspend_layout;
  std::function<void(DbDriverParam *, ControlType, const base::ControlBounds &, const std::string &)> _create_control;

  void init_dbc_connection(sql::Connection *dbc_conn, const db_mgmt_ConnectionRef &connectionProperties);

public:
  DbConnection(const db_mgmt_ManagementRef &mgmt, const db_mgmt_DriverRef &driver, bool skip_schema);

  ~DbConnection();

  void set_control_callbacks(const std::function<void(bool)> &suspend_layout, const std::function<void()> &begin_layout,
                             const std::function<void(DbDriverParam *, ControlType, const base::ControlBounds &,
                                                      const std::string &)> &create_control,
                             const std::function<void()> &end_layout);

  DbDriverParams *get_db_driver_param_handles() {
    return &_db_driver_param_handles;
  }

  void update();
  void set_connection_and_update(const db_mgmt_ConnectionRef &connection);
  void set_connection_keeping_parameters(const db_mgmt_ConnectionRef &connection);
  db_mgmt_ConnectionRef get_connection();

  void save_changes();

  sql::ConnectionWrapper get_dbc_connection();
  db_mgmt_ManagementRef get_db_mgmt() {
    return _mgmt;
  }
  db_mgmt_DriverRef driver() {
    return _active_driver;
  }

  void set_driver_and_update(db_mgmt_DriverRef);

  bool test_connection();
  std::string validate_driver_params() const;
};
