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

#include "base/geometry.h"

using namespace base;

#include "grtdb/db_helpers.h"
#include "db_conn_be.h"
#include "grtsqlparser/sql_facade.h"
#include "grt/grt_manager.h"
#include "grtdb/charset_utils.h"
#include "base/string_utilities.h"
#include "base/log.h"

#include <algorithm>
#include <cctype>
#include <algorithm>
#include <memory>
#include <set>
#include <string>

#undef max

DEFAULT_LOG_DOMAIN("DbConnectPanel");

grt::StringRef DbDriverParam::get_control_name() const {
  return grt::StringRef(_inner->name());
}

grt::StringRef DbDriverParam::get_accessibility_name() const {
  return grt::StringRef(_inner->accessibilityName());
}

DbDriverParam::ParamType DbDriverParam::decode_param_type(std::string type_name, std::string real_type) {
  ParamType result = ptUnknown;

  std::transform(type_name.begin(), type_name.end(), type_name.begin(), g_unichar_tolower);

  if (0 == type_name.compare("string"))
    result = ptString;
  else if (0 == type_name.compare("int"))
    result = ptInt;
  else if (0 == type_name.compare("boolean"))
    result = ptBoolean;
  else if (0 == type_name.compare("tristate"))
    result = ptTristate;
  else if (0 == type_name.compare("dir"))
    result = ptDir;
  else if (0 == type_name.compare("file"))
    result = ptFile;
  else if (0 == type_name.compare("password"))
    result = ptPassword;
  else if (0 == type_name.compare("keychain"))
    result = ptKeychainPassword;
  else if (0 == type_name.compare("enum")) {
    if (real_type == "int")
      result = ptIntEnum;
    else if (real_type == "unsigned int")
      result = ptIntOption;
    else
      result = ptEnum;
  } else if (0 == type_name.compare("text"))
    result = ptText;
  else if (0 == type_name.compare("button"))
    result = ptButton;
  else
    logWarning("Unknown DB driver parameter type '%s'\n", type_name.c_str());

  return result;
}

DbDriverParam::DbDriverParam(const db_mgmt_DriverParameterRef &driver_param, const db_mgmt_ConnectionRef &stored_conn)
  : _inner(driver_param), _type(ptUnknown) {
  _type = decode_param_type(_inner->paramType(), _inner->paramTypeDetails().get_string("type"));

  if (stored_conn.is_valid() && !(*stored_conn->name()).empty())
    set_value(stored_conn->parameterValues().get(driver_param->name(), driver_param->defaultValue()));
  else
    set_value(driver_param->defaultValue());
}

DbDriverParam::DbDriverParam(const db_mgmt_DriverParameterRef &driver_param, const grt::ValueRef &value)
  : _inner(driver_param), _type(ptUnknown) {
  _type = decode_param_type(_inner->paramType(), _inner->paramTypeDetails().get_string("type"));
  set_value(value);
}

ControlType DbDriverParam::get_control_type() const {
  switch (get_type()) {
    case DbDriverParam::ptBoolean:
    case DbDriverParam::ptTristate:
      return ctCheckBox;
    case DbDriverParam::ptDir:
      return ctDirSelector;
    case DbDriverParam::ptFile:
      return ctFileSelector;
    case DbDriverParam::ptKeychainPassword:
      return ctKeychainPassword;
    case DbDriverParam::ptEnum:
    case DbDriverParam::ptIntEnum:
      return ctEnumSelector;
    case DbDriverParam::ptIntOption:
      return ctEnumOption;
    case DbDriverParam::ptText:
      return ctText;
    case DbDriverParam::ptButton:
      return ctButton;
    case DbDriverParam::ptInt:
    case DbDriverParam::ptString:
    case DbDriverParam::ptPassword:
    default:
      return ctTextBox;
  }
}

void DbDriverParam::set_value(const grt::ValueRef &value) {
  switch (_type) {
    case ptString:
    case ptPassword:
    case ptDir:
    case ptFile:
    case ptText:
    case ptKeychainPassword: // this only keeps the storage key format
    {
      _value = grt::StringRef::cast_from(value);
      break;
    }

    case ptInt:
    case ptBoolean:
    case ptTristate:
    case ptIntOption:
    case ptIntEnum: {
      if (value.type() == grt::IntegerType)
        _value = value;
      else {
        grt::StringRef s = grt::StringRef::cast_from(value);
        if (s.is_valid() && !(*s).empty()) {
          try {
            int n = std::stoi(*s);
            _value = grt::IntegerRef(n);
          } catch (...) {
            _value = grt::ValueRef();
          }
        } else
          _value = grt::ValueRef();
      }

      break;
    }

    case ptEnum: {
      _value = grt::StringRef::cast_from(value);
      break;
    }
    case ptUnknown:
    default: { break; }
  }
}

std::vector<std::pair<std::string, std::string> > DbDriverParam::get_enum_options() {
  std::vector<std::pair<std::string, std::string> > options;

  if ((*_inner->lookupValueModule()).empty()) {
    std::string type = _inner->paramTypeDetails().get_string("type");
    std::vector<std::string> optionsv = base::split(_inner->paramTypeDetails().get_string("options"), ",");
    for (std::vector<std::string>::const_iterator opt = optionsv.begin(); opt != optionsv.end(); ++opt) {
      std::string s = *opt;
      std::string::size_type pos;
      if ((pos = s.find('|')) != std::string::npos)
        options.push_back(std::make_pair(s.substr(0, pos), s.substr(pos + 1)));
      else
        options.push_back(std::make_pair(s, s));
    }
  } else {
    grt::Module *module = grt::GRT::get()->get_module(*_inner->lookupValueModule());
    if (module) {
      grt::BaseListRef args(true);
      grt::ValueRef result = module->call_function(*_inner->lookupValueMethod(), args);
      if (result.is_valid() && grt::StringListRef::can_wrap(result)) {
        grt::StringListRef list = grt::StringListRef::cast_from(result);
        for (int i = 0; i < (int)list.count(); i++) {
          std::string s = list[i];
          std::string::size_type pos;
          if ((pos = s.find('|')) != std::string::npos)
            options.push_back(std::make_pair(s.substr(0, pos), s.substr(pos + 1)));
          else
            options.push_back(std::make_pair(s, s));
        }
      } else
        logWarning("Error calling enum value lookup method %s.%s for DriverParameter %s\n",
                   _inner->lookupValueModule().c_str(), _inner->lookupValueMethod().c_str(), _inner->name().c_str());
    } else
      logWarning("Error searching module for enum value lookup method %s.%s for DriverParameter %s\n",
                 _inner->lookupValueModule().c_str(), _inner->lookupValueMethod().c_str(), _inner->name().c_str());
  }
  return options;
}

grt::StringRef DbDriverParam::getValue() {
  grt::StringRef value = "";
  if (!(*_inner->lookupValueModule()).empty() && (*_inner->lookupValueModule()) == "Options") {
    grt::DictRef wb_options = grt::DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
    switch (this->_type) {
      case ParamType::ptInt:
        value = std::to_string(wb_options.get_int(*_inner->lookupValueMethod(), 0));
        break;
      case ParamType::ptString:
        value = wb_options.get_string(*_inner->lookupValueMethod(), this->get_value_repr());
        break;
      default:
        value = this->get_value_repr();
        break;
    }

  } else
    value = this->get_value_repr();

  return value;
}
//----------------------------------------------------------------------

struct LayoutControl {
  LayoutControl(int offset) : param_handle(NULL), type(ctUnknown) {
    bounds.left = offset;
  }
  DbDriverParam *param_handle;
  ControlType type;
  ControlBounds bounds;
  std::string caption;
};

class LayoutRow {
public:
  typedef std::list<LayoutControl> LayoutControls;
  LayoutRow(int seq_no, int hmargin) : _seq_no(seq_no), _hmargin(hmargin), _offset(hmargin), _max_height(0) {
  }

private:
  LayoutControls _controls;
  int _seq_no;
  int _hmargin;
  int _offset;
  int _max_height;
  std::string _row_desc;

public:
  int seq_no() const {
    return _seq_no;
  }
  int offset() const {
    return _offset;
  }
  int max_height() const {
    return _max_height;
  }
  bool empty() const {
    return _controls.empty();
  }
  void insert(LayoutControl &control) {
    _controls.push_back(control);
    _offset += control.bounds.width + _hmargin;
    _max_height = std::max(_max_height, control.bounds.height);
  }
  void add_desc(const std::string &desc) {
    if (!desc.empty()) {
      if (!_row_desc.empty())
        _row_desc.append(" - ");
      _row_desc.append(desc);
    }
  }
  LayoutControls *controls() {
    return &_controls;
  }
  LayoutControl *control(int index) {
    LayoutControls::iterator i = _controls.begin();
    if (!_controls.empty())
      while (index--)
        if (++i == _controls.end())
          break;
    return (i == _controls.end() ? NULL : &(*i));
  }
  LayoutControl desc_control() const {
    LayoutControl ctrl(_offset);
    ctrl.param_handle = _controls.begin()->param_handle;
    ctrl.type = ctDescriptionLabel;
    ctrl.caption = _row_desc;
    return ctrl;
  }
};

bool DbDriverParams::parameter_not_valid(const db_mgmt_DriverRef &driver, const std::string &param) {
  const std::string &name = driver->name();
  if (name == "MysqlNativeSocket") {
    static const std::set<std::string> restricted_params = {
      "port",          "connections_created", "haGroupFilter", "managedConnectionsUpdateTime",
      "mysqlUserName", "sshPassword",         "sshKeyFile",    "sshHost",
      "sshUserName"};
    if (restricted_params.count(param) > 0)
      return true;
  } else if (name == "MysqlNative") {
    static const std::set<std::string> restricted_params = {
      "connections_created", "socked",  "haGroupFilter", "managedConnectionsUpdateTime", "mysqlUserName", "sshPassword",
      "sshKeyFile",          "sshHost", "sshUserName"};

    if (restricted_params.count(param) > 0)
      return true;
  } else if (name == "MysqlNativeSSH") {
    static const std::set<std::string> restricted_params = {"socket", "haGroupFilter", "managedConnectionsUpdateTime",
                                                            "mysqlUserName"};
    if (restricted_params.count(param) > 0)
      return true;
  }
  return false;
}

typedef std::list<LayoutRow> LayoutRows;

void DbDriverParams::init(
  const db_mgmt_DriverRef &driver, const db_mgmt_ConnectionRef &stored_conn,
  const std::function<void(bool)> &suspend_layout, const std::function<void()> &begin_layout,
  const std::function<void(DbDriverParam *, ControlType, const ControlBounds &, const std::string &)> &create_control,
  const std::function<void()> &end_layout, bool skip_schema, int first_row_label_width, int hmargin, int vmargin) {
  typedef std::vector<std::string>::iterator StringVectorIterator;
  if (begin_layout)
    begin_layout();

  // check what options are in the connections file that are not known, so we can lump them and show in the other
  // options textbox
  std::vector<std::string> unknown_options;
  if (stored_conn.is_valid()) {
    grt::DictRef vals(stored_conn->parameterValues());
    for (grt::DictRef::const_iterator i = vals.begin(); i != vals.end(); ++i)
      unknown_options.push_back(i->first);
  }
  free_dyn_mem();

  if (suspend_layout)
    suspend_layout(true);

  _driver = driver;
  grt::ListRef<db_mgmt_DriverParameter> params = driver->parameters();
  size_t param_count = params.count();

  _collection.resize(param_count);
  _control_name_index.clear();
  DbDriverParam *param_handle;
  bool trim_schema = false;
  db_mgmt_DriverParameterRef others_option;
  // create param handles
  for (size_t n = 0; n < param_count; ++n) {
    db_mgmt_DriverParameterRef param = params.get(n);

    // remove known options

    StringVectorIterator end = unknown_options.end();
    StringVectorIterator it = std::find(unknown_options.begin(), end, *param->name());
    if (it != end)
      unknown_options.erase(it);

    if (skip_schema && param->name() == "schema") {
      trim_schema = true;
      continue;
    }
    if (param->name() == "$others") {
      others_option = param;
      continue;
    }
    param_handle = new DbDriverParam(param, stored_conn);
    _collection[_control_name_index.size()] = param_handle;
    _control_name_index[param->name()] = param_handle;
  }

  if (others_option.is_valid()) {
    std::string unknown_options_text;
    StringVectorIterator end = unknown_options.end();
    for (std::vector<std::string>::const_iterator k = unknown_options.begin(); k != end; ++k) {
      if (parameter_not_valid(driver, *k))
        continue;
      if (!k->empty()) {
        unknown_options_text.append(*k);
        unknown_options_text.append("=");
        unknown_options_text.append(stored_conn->parameterValues().get(*k).toString());
        unknown_options_text.append("\n");
      }
    }
    param_handle = new DbDriverParam(others_option, grt::StringRef(unknown_options_text));
    _collection[_control_name_index.size()] = param_handle;
    _control_name_index[others_option->name()] = param_handle;
  }

  if (trim_schema)
    _collection.resize(param_count - 1);
  for (int layout_type = 0; layout_type < 4; ++layout_type) // separate cycle for controls tagged as advanced layout
  {
    LayoutRows rows;
    LayoutRow row(0, hmargin);
    int y_offset = vmargin;
    for (Collection::iterator i = _collection.begin(); i != _collection.end(); ++i) {
      param_handle = *i;
      db_mgmt_DriverParameterRef param = param_handle->object();

      // process change of layout row
      if (row.seq_no() != param->layoutRow() || layout_type) {
        if (!row.empty()) {
          rows.push_back(row);
          y_offset += row.max_height() + vmargin;
        }
        row = LayoutRow((int)param->layoutRow(), hmargin);
      }

      if (layout_type == param->layoutAdvanced()) {
        if (-1 == row.seq_no() && !layout_type)
          continue;

        // create related label ctrl in UI
        if (param_handle->get_control_type() != ctCheckBox && param_handle->get_control_type() != ctButton) {
          LayoutControl ctrl(row.offset());
          ctrl.param_handle = param_handle;
          ctrl.type = ctLabel;
          ctrl.caption = param->caption();
          if (row.empty())
            ctrl.bounds.width = first_row_label_width;
          ctrl.bounds.top = y_offset;
          // create_control(ctrl.param_handle, ctrl.type, ctrl.pos, ctrl.size, ctrl.caption);
          row.insert(ctrl);
        }

        // create ctrl for param ed
        {
          LayoutControl ctrl(row.offset());
          ctrl.param_handle = param_handle;
          ctrl.type = param_handle->get_control_type();
          ctrl.bounds.width = (int)param->layoutWidth();
          ctrl.bounds.top = y_offset;
          if (param_handle->get_control_type() == ctCheckBox || param_handle->get_control_type() == ctButton)
            ctrl.caption = param->caption();
          // create_control(ctrl.param_handle, ctrl.type, ctrl.pos, ctrl.size, ctrl.caption);
          row.insert(ctrl);
        }

        // add param description to the row desc
        row.add_desc(param->description());
      }
    }
    if (!row.empty()) {
      rows.push_back(row);
      y_offset += row.max_height() + vmargin;
    }

    // visualize controls
    int row_index = 0;
    for (LayoutRows::iterator r = rows.begin(); r != rows.end(); ++r) {
      LayoutRow::LayoutControls *controls = r->controls();
      for (LayoutRow::LayoutControls::iterator c = controls->begin(); c != controls->end(); ++c) {
        c->bounds.top = row_index;
        if (create_control)
          create_control(c->param_handle, c->type, c->bounds, c->caption);
      }

      // row description control
      {
        LayoutControl ctrl = r->desc_control();
        ctrl.bounds.top = row_index;

        if (create_control)
          create_control(ctrl.param_handle, ctrl.type, ctrl.bounds, ctrl.caption);
      }
      row_index++;
    }
  }

  if (suspend_layout)
    suspend_layout(false);

  if (end_layout)
    end_layout();
}

void DbDriverParams::free_dyn_mem() {
  for (Collection::const_iterator i = _collection.begin(); i != _collection.end(); ++i)
    delete *i;
}

grt::DictRef DbDriverParams::get_params() const {
  if (_driver.is_valid()) {
    grt::DictRef params(true);
    for (Collection::const_iterator i = _collection.begin(); i != _collection.end(); ++i) {
      DbDriverParam *param_handle = *i;
      if (param_handle->get_value().is_valid()) {
        if (param_handle->object()->name() == "$others") {
          std::vector<std::string> options(base::split(param_handle->get_value().toString(), "\n"));
          for (std::vector<std::string>::const_iterator op = options.begin(); op != options.end(); ++op) {
            std::string name, value;
            base::partition(*op, "=", name, value);
            if (value.empty())
              params.set(name, grt::StringRef(value));
            else if (value[0] == '\'' && value[value.size() - 1] == '\'')
              params.set(name, grt::StringRef(base::unescape_sql_string(value, '\\')));
            else if (value[0] == '"' && value[value.size() - 1] == '"')
              params.set(name, grt::StringRef(base::unescape_sql_string(value, '\\')));
            else {
              bool isnum = true;
              for (int i = value[0] == '-' ? 1 : 0; isnum && i < (int)value.size(); i++)
                if (!isdigit(value[i]))
                  isnum = false;
              if (isnum)
                params.set(name, grt::IntegerRef(base::atoi<int>(value, 0)));
              else
                params.set(name, grt::StringRef(value));
            }
          }
        } else
          params.set(param_handle->object()->name(), param_handle->get_value());
      }
    }
    return params;
  }
  return grt::DictRef();
}

DbDriverParam *DbDriverParams::get(std::string control_name) {
  String_index::const_iterator i = _control_name_index.find(control_name);
  if (_control_name_index.end() != i)
    return i->second;
  return NULL;
}

std::string DbDriverParams::validate() const {
  std::string err_msg("");
  for (Collection::const_iterator i = _collection.begin(); i != _collection.end(); ++i) {
    DbDriverParam *param_handle = *i;
    const grt::StringRef &value = param_handle->get_value_repr();
    if ((!value.is_valid() || !(*value).length()) && param_handle->object()->required()) {
      std::string text;
      text.append("Required parameter '")
        .append(param_handle->object()->name())
        .append("' is not set. Please set it to continue.");
      err_msg = text;
    }
  }
  return err_msg;
}

//----------------------------------------------------------------------

DbConnection::DbConnection(const db_mgmt_ManagementRef &mgmt, const db_mgmt_DriverRef &driver, bool skip_schema)
  : _mgmt(mgmt), _active_driver(driver), _skip_schema(skip_schema) {
}

void DbConnection::set_control_callbacks(
  const std::function<void(bool)> &suspend_layout, const std::function<void()> &begin_layout,
  const std::function<void(DbDriverParam *, ControlType, const ControlBounds &, const std::string &)> &create_control,
  const std::function<void()> &end_layout) {
  _suspend_layout = suspend_layout;
  _begin_layout = begin_layout;
  _end_layout = end_layout;
  _create_control = create_control;
}

db_mgmt_ConnectionRef DbConnection::get_connection() {
  save_changes();
  return _connection;
}

void DbConnection::save_changes() {
  if (_connection.is_valid()) {
    _connection->driver(_active_driver);

    grt::replace_contents(_connection->parameterValues(), _db_driver_param_handles.get_params());
    _connection->hostIdentifier(bec::get_host_identifier_for_connection(_connection));
  }
}

DbConnection::~DbConnection() {
}

void DbConnection::set_connection_and_update(const db_mgmt_ConnectionRef &connection) {
  if (_connection != connection) {
    _connection = connection;
    _active_driver = connection->driver();

    _db_driver_param_handles.init(_active_driver, _connection, _suspend_layout, _begin_layout, _create_control,
                                  _end_layout, _skip_schema);
  }
}

void DbConnection::set_connection_keeping_parameters(const db_mgmt_ConnectionRef &connection) {
  if (_connection != connection) {
    _connection = connection;
    _connection->driver(_active_driver);
    grt::DictRef curparams(_db_driver_param_handles.get_params());
    if (curparams.is_valid())
      grt::merge_contents(_connection->parameterValues(), curparams, true);
  }
}

void DbConnection::update() {
  _db_driver_param_handles.init(_active_driver, _connection, _suspend_layout, _begin_layout, _create_control,
                                _end_layout, _skip_schema);
}

void DbConnection::set_driver_and_update(db_mgmt_DriverRef driver) {
  _active_driver = driver;
  if (_connection.is_valid())
    _connection->driver(driver);

  _db_driver_param_handles.init(_active_driver, _connection, _suspend_layout, _begin_layout, _create_control,
                                _end_layout, _skip_schema);

  if (_connection.is_valid())
    save_changes();
}

bool DbConnection::test_connection() {
  sql::ConnectionWrapper dbc_conn = get_dbc_connection();
  return (dbc_conn.get() != NULL);
}

void DbConnection::init_dbc_connection(sql::Connection *dbc_conn, const db_mgmt_ConnectionRef &connectionProperties) {
  // connection startup script
  {
    std::list<std::string> sql_script;
    {
      db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(get_connection()->driver()->owner());
      SqlFacade::Ref sql_facade = SqlFacade::instance_for_rdbms(rdbms);
      Sql_specifics::Ref sql_specifics = sql_facade->sqlSpecifics();
      sql_specifics->get_connection_startup_script(sql_script);
    }
    std::unique_ptr<sql::Statement> stmt(dbc_conn->createStatement());
    sql::SqlBatchExec sql_batch_exec;
    sql_batch_exec(stmt.get(), sql_script);
  }
}

sql::ConnectionWrapper DbConnection::get_dbc_connection() {
  save_changes();

  sql::ConnectionWrapper dbc_conn = sql::DriverManager::getDriverManager()->getConnection(
    get_connection(),
    std::bind(&DbConnection::init_dbc_connection, this, std::placeholders::_1, std::placeholders::_2));

  return dbc_conn;
}

std::string DbConnection::validate_driver_params() const {
  return _db_driver_param_handles.validate();
}
