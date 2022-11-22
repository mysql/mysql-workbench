/*
 * Copyright (c) 2009, 2022, Oracle and/or its affiliates. All rights reserved.
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

#include "driver_manager.h"

#include "wb_tunnel.h"

#include "mysql_driver.h"
#include "cppconn/driver.h"
#include "cppconn/statement.h"
#include "cppconn/exception.h"
#include "cppconn/metadata.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "grt.h"
#include "mforms/app.h"

#include <gmodule.h>
#include <stdlib.h>

using namespace wb;

namespace sql {

  typedef std::map<std::string, std::string> Param_types;

  static bool conv_to_dbc_value(const std::string &key, const grt::ValueRef value, ConnectOptionsMap &properties,
                                Param_types &param_types) {
    ConnectPropertyVal tmp;

    switch (value.type()) {
      case grt::IntegerType: {
        grt::IntegerRef val = grt::IntegerRef::cast_from(value);
        std::string param_type;
        if (param_types.find(key) != param_types.end())
          param_type = param_types[key];
        if (param_type == "tristate")
          tmp = (int)(*val != 0);
        else if (param_type == "boolean")
          tmp = (bool)(*val != 0);
        else
          tmp = (int)(*val);
        properties[key] = tmp;
      } break;

      case grt::DoubleType: {
        grt::DoubleRef val = grt::DoubleRef::cast_from(value);
        tmp = *val;
        properties[key] = tmp;
      } break;

      case grt::StringType: {
        grt::StringRef val = grt::StringRef::cast_from(value);
        tmp = SQLString(val.c_str(), (*val).length());
        properties[key] = tmp;
      } break;

      case grt::UnknownType:
      // case grt::AnyType: // equal to grt::UnknownType
      case grt::ListType:
      case grt::DictType:
      case grt::ObjectType:
        break;
    }

    return true;
  }

  ////

  Authentication::Ref Authentication::create(const db_mgmt_ConnectionRef &props, const std::string &service) {
    return Authentication::Ref(new Authentication(props, service));
  }

  Authentication::Authentication(const db_mgmt_ConnectionRef &props, const std::string &service)
    : _props(props), _service(service), _password(NULL) {
  }

  Authentication::Authentication() : _password(NULL) {
  }

  Authentication::~Authentication() {
    invalidate();
  }

  void Authentication::set_password(const char *password) {
    invalidate();
    _password = g_strdup(password);
  }

  void Authentication::invalidate() {
    if (_password != NULL) {
      memset(_password, 0, strlen(_password));
      g_free(_password);
      _password = NULL;
    }
  }

  std::string Authentication::uri(bool withPassword) {
    std::vector<std::string> v;
    grt::DictRef parameter_values = connectionProperties()->parameterValues();

    v.push_back(parameter_values.get_string("hostName"));
    v.push_back(std::to_string(connectionProperties()->parameterValues().get_int("port")));
    v.push_back(parameter_values.get_string("userName"));
    if (parameter_values.get_string("password").empty()) {
      if (is_valid() && withPassword)
        v.push_back(_password);
    }

    std::string uri;
    if (v.size() == 4) // if there's no pw, we will ask for it later
      uri = v[2] + ":" + v[3] + "@" + v[0] + ":" + v[1];
    else
      uri = v[2] + "@" + v[0] + ":" + v[1];
    return uri;
  }

  //----------------- DriverManager ------------------------------------------------------------------

  DriverManager *DriverManager::getDriverManager() {
    static DriverManager *dm = new DriverManager;
    return dm;
  }

  DriverManager::DriverManager() : _driver_path("."), _cacheTime(0) {
  }

  void DriverManager::setTunnelFactoryFunction(TunnelFactoryFunction function) {
    _createTunnel = function;
  }

  void DriverManager::setPasswordFindFunction(PasswordFindFunction function) {
    _findPassword = function;
  }

  void DriverManager::setPasswordRequestFunction(PasswordRequestFunction function) {
    _requestPassword = function;
  }

  void DriverManager::set_driver_dir(const std::string &path) {
    _driver_path = path;
  }

  std::shared_ptr<SSHTunnel> DriverManager::getTunnel(const db_mgmt_ConnectionRef &connectionProperties) {
    db_mgmt_DriverRef drv = connectionProperties->driver();
    if (!drv.is_valid())
      throw SQLException("Invalid connection settings: undefined connection driver");

    if (_createTunnel)
      return _createTunnel(connectionProperties);
    return std::shared_ptr<SSHTunnel>();
  }

  //--------------------------------------------------------------------------------------------------

#define MYSQL_PASSWORD_CACHE_TIMEOUT 60

  ConnectionWrapper DriverManager::getConnection(const db_mgmt_ConnectionRef &connectionProperties,
                                                 ConnectionInitSlot connection_init_slot) {
    db_mgmt_DriverRef drv = connectionProperties->driver();
    if (!drv.is_valid())
      throw SQLException("Invalid connection settings: undefined connection driver");

    std::shared_ptr<SSHTunnel> tunnel;
    if (_createTunnel) {
      tunnel = _createTunnel(connectionProperties);

      if (tunnel) {
        // this can throw an exception if the tunnel can't be created
        //! tunnel->connect();
      }
    }
    return getConnection(connectionProperties, tunnel, Authentication::Ref(), connection_init_slot);
  }
  //--------------------------------------------------------------------------------------------------
  // This method is called when each dispatcher is ending is about to be gone
  // it needs to be called right after that to cleanup the thread storage allocated by driver.
  // If we will not free the mem then after wb close we will get error about "threads didn't exit"
  void DriverManager::thread_cleanup() {
    for (auto &it : _drivers)
      it.second();
  }

  //--------------------------------------------------------------------------------------------------

  unsigned int DriverManager::getClientLibVersionNumeric(Driver *driver) {
    assert(driver != NULL);
    return driver->getMajorVersion() * 10000 + driver->getMinorVersion() * 100 + driver->getPatchVersion();
  }

  //--------------------------------------------------------------------------------------------------

  void DriverManager::getClientLibVersion(Driver *driver) {
    assert(driver != NULL);
    _versionInfo = "C++ " + std::to_string(driver->getMajorVersion()) + ".";
    _versionInfo += std::to_string(driver->getMinorVersion()) + ".";
    _versionInfo += std::to_string(driver->getPatchVersion());
  }

  //--------------------------------------------------------------------------------------------------

  const std::string &DriverManager::getClientLibVersion() const {
    return _versionInfo;
  }

  //--------------------------------------------------------------------------------------------------

  ConnectionWrapper DriverManager::getConnection(const db_mgmt_ConnectionRef &connectionProperties,
                                                 std::shared_ptr<SSHTunnel> tunnel, Authentication::Ref password,
                                                 ConnectionInitSlot connection_init_slot) {
    grt::DictRef parameter_values = connectionProperties->parameterValues();

    // Load the driver for the connection dynamically. However for the C++ connector we have a static
    // link anyway, so use this instead.

    // 0. determine correct driver filename
    db_mgmt_DriverRef drv = connectionProperties->driver();

    std::string library = "";
    if (drv.is_valid())
      library = drv->driverLibraryName();
    else
      throw SQLException("Invalid connection settings: undefined connection driver");

    Driver *driver;
    if (library == "mysqlcppconn")
      driver = get_driver_instance();
    else {
#ifdef _MSC_VER
      library.append(".dll");
#elif defined(__APPLE__)
      library.append(".dylib");
#else
      library.append(".so");
#endif

      if (drv->name() != "MysqlNativeSaslKerberos" && drv->name() != "MysqlNativeKerberos" &&
          parameter_values.get_string("userName").empty())
        throw SQLException("No user name set for this connection");

      // 1. find driver
      GModule *gmodule = g_module_open((_driver_path + "/" + library).c_str(), G_MODULE_BIND_LOCAL);
      if (NULL == gmodule) {
        fprintf(stderr, "Error: %s", g_module_error());
        throw SQLException(std::string("Database driver: Failed to open library '")
                             .append(_driver_path + "/" + library)
                             .append("'. Check settings.")
                             .c_str());
      }

      Driver *(*get_driver_instance)() = NULL;
      g_module_symbol(gmodule, "sql_mysql_get_driver_instance", (gpointer *)&get_driver_instance);
      if (NULL == get_driver_instance) {
        g_module_close(gmodule);
        throw SQLException("Database driver: Failed to get library instance. Check settings.");
      }

      driver = get_driver_instance();
    }
    if (driver == NULL)
      throw SQLException("Database driver: Failed to get driver instance. Check  settings.");
    else
      _drivers[library] = std::bind(&Driver::threadEnd, driver);

    getClientLibVersion(driver);

    // 2. call driver->connect()
    Param_types param_types;
    {
      grt::ListRef<db_mgmt_DriverParameter> params = connectionProperties->driver()->parameters();
      for (size_t n = 0, count = params.count(); n < count; ++n) {
        db_mgmt_DriverParameterRef param = params.get(n);
        param_types[param->name()] = param->paramType();
      }
      // set the values that take boolean options, so that they can be converted correctly in case they're specified
      // manually by the user
      param_types["OPT_REPORT_DATA_TRUNCATION"] = "boolean";
      param_types["OPT_ENABLE_CLEARTEXT_PLUGIN"] = "boolean";
      param_types["CLIENT_INTERACTIVE"] = "boolean";
      param_types["CLIENT_COMPRESS"] = "boolean";
      param_types["CLIENT_FOUND_ROWS"] = "boolean";
      param_types["CLIENT_IGNORE_SIGPIPE"] = "boolean";
      param_types["CLIENT_IGNORE_SPACE"] = "boolean";
      param_types["CLIENT_LOCAL_FILES"] = "boolean";
      param_types["CLIENT_MULTI_STATEMENTS"] = "boolean";
      param_types["CLIENT_NO_SCHEMA"] = "boolean";
    }
    ConnectOptionsMap properties;
    parameter_values.foreach (
      std::bind(&conv_to_dbc_value, std::placeholders::_1, std::placeholders::_2, std::ref(properties), param_types));

    {
      ConnectPropertyVal tmp;
      const int conn_timeout = 60;
      const int read_timeout = 30;
      if (properties.find("OPT_CONNECT_TIMEOUT") == properties.end())
        properties["OPT_CONNECT_TIMEOUT"] = conn_timeout;
      if (properties.find("OPT_READ_TIMEOUT") == properties.end())
        properties["OPT_READ_TIMEOUT"] = read_timeout;
#ifdef _MSC_VER
      properties["pluginDir"] = base::dirname(mforms::App::get()->get_executable_path("base.dll"));
#endif
      properties["OPT_AUTHENTICATION_KERBEROS_CLIENT_MODE"] = "";
      std::string krb5 = parameter_values.get_string("krb5");
      std::string krb5cache = parameter_values.get_string("krb5cache");
      std::vector<char> env;
      if (!krb5.empty()) {
        auto tmp = std::string("KRB5_CONFIG=" + krb5);
        env = std::vector<char>(tmp.begin(), tmp.end());
      } else {
        auto tmp = std::string("KRB5_CONFIG=");
        env = std::vector<char>(tmp.begin(), tmp.end());
      }
      putenv(&env[0]);

      env.clear();
      if (!krb5cache.empty()) {
        auto tmp = std::string("KRB5CCNAME=" + krb5cache);
        env = std::vector<char>(tmp.begin(), tmp.end());
      } else {
        auto tmp = std::string("KRB5CCNAME=");
        env = std::vector<char>(tmp.begin(), tmp.end());
      }
      putenv(&env[0]);

      properties["defaultAuth"] = "";
    }
    properties["OPT_CAN_HANDLE_EXPIRED_PASSWORDS"] = true;
    properties["CLIENT_MULTI_STATEMENTS"] = true;
    properties["metadataUseInfoSchema"] =
      false; // I_S is way too slow for many things as of MySQL 5.6.10, so disable it for now

    // set application name
    {
      std::map<sql::SQLString, sql::SQLString> attribs;
      attribs["program_name"] = "MySQLWorkbench";
      properties["OPT_CONNECT_ATTR_ADD"] = attribs;
    }

    // If SSL is enabled but there's no certificate or anything, create the sslKey option to force enabling SSL without
    // a key
    // (equivalent to starting cmdline client with mysql --ssl-key=)
    if (parameter_values.get_string("sslKey", "") == "" && parameter_values.get_int("useSSL", 0) != 0)
      properties["sslKey"] = std::string();

    // If SSL is not enabled, clear all ssl related props to not confuse the connector
    if (parameter_values.get_int("useSSL", 0) == 0) {
      properties.erase("sslKey");
      properties.erase("sslCert");
      properties.erase("sslCA");
      properties.erase("sslCAPath");
      properties.erase("sslCipher");
    }

    ssize_t sslModeWb = parameter_values.get_int("useSSL", 0);
    sql::ssl_mode sslMode = sql::SSL_MODE_DISABLED;
    switch (sslModeWb) {
      case 0:
        sslMode = sql::SSL_MODE_DISABLED;
        properties["OPT_GET_SERVER_PUBLIC_KEY"] = true;
        break;
      case 1:
        sslMode = sql::SSL_MODE_PREFERRED;
        break;
      case 2:
        sslMode = sql::SSL_MODE_REQUIRED;
        break;
      case 3:
        sslMode = sql::SSL_MODE_VERIFY_CA;
        break;
      case 4:
        sslMode = sql::SSL_MODE_VERIFY_IDENTITY;
        break;
    }
    properties["OPT_SSL_MODE"] = sslMode;

    // If we are on a pipe connection then set the host name explicitly.
    // However, pipe connections can only be established on the local box (Win only).
    if (drv->name() == "MysqlNativeSocket") {
#ifdef _MSC_VER
      ConnectOptionsMap::iterator it = properties.find("socket");
      if (it != properties.end()) {
        properties["pipe"] = it->second;
        properties.erase(it);
      }
      properties["hostName"] = std::string(".");
#else
      properties["hostName"] = std::string();
#endif
    } else if (drv->name() == "MysqlNativeSaslKerberos") {
      properties["defaultAuth"] = "authentication_ldap_sasl_client";
      std::string plugin_dir_path = parameter_values.get_string("mysqlplugindir");
      if (!plugin_dir_path.empty()) {
        properties["pluginDir"] = plugin_dir_path;
      } else {
        std::string libName = "authentication_ldap_sasl_client";
#ifdef _MSC_VER
        libName.append(".dll");
#else
        libName.append(".so");
#endif
        properties["pluginDir"] = base::dirname(mforms::App::get()->get_executable_path(libName));
      }
    } else if (drv->name() == "MysqlNativeKerberos") {
      properties["defaultAuth"] = "authentication_kerberos_client";
      std::string plugin_dir_path = parameter_values.get_string("mysqlplugindir");
      if (!plugin_dir_path.empty()) {
        properties["pluginDir"] = plugin_dir_path;
      } else {
        std::string libName = "authentication_kerberos_client";
#ifdef _MSC_VER
        libName.append(".dll");
#else
        libName.append(".so");
#endif
        properties["pluginDir"] = base::dirname(mforms::App::get()->get_executable_path(libName));
      }

      properties["OPT_AUTHENTICATION_KERBEROS_CLIENT_MODE"] = parameter_values.get_int("kerberosMode", 0) == 1 ? "SSPI": "GSSAPI";
    } else if (drv->name() == "MysqlNativeLDAP") {
      properties["OPT_ENABLE_CLEARTEXT_PLUGIN"] = true;
    }

    if (tunnel) {
      // Make the driver connect to the local tunnel port.
      properties["port"] = tunnel->getConfig().localport;
      properties["hostName"] = sql::SQLString("127.0.0.1");
    } else
      properties["hostName"] = "[" + parameter_values.get_string("hostName") +
                               "]"; // [quote] hostname so that ipv6 addresses aren't parsed as URIs

    Authentication::Ref authref;

    // Check if there is a stored or cached password, if there isn't try without one (blank)
    // If we get an auth error, then we ask for the password
    bool force_ask_password = false;
  retry:
    if (password && drv->name() != "MysqlNativeSaslKerberos" && drv->name() != "MysqlNativeKerberos") {
      authref = password;
      if (password->is_valid())
        properties["password"] = std::string(authref->password());
    } else {
      // password not in profile (and no keyfile provided)
      if (_requestPassword && (force_ask_password || parameter_values.get_string("password") == "")) {
        // check if we have cached the password for this connection
        if (time(NULL) - _cacheTime > MYSQL_PASSWORD_CACHE_TIMEOUT || force_ask_password) {
          _cacheKey.clear();
          _cachedPassword.clear();
        }

        std::string key = connectionProperties->hostIdentifier();
        if (parameter_values.get_string("userName") + std::string("@") + key == _cacheKey &&
            !key.empty()) // hostIdentifier will be "" if it's a temporary connection
          properties["password"] = _cachedPassword;
        else {
          if (force_ask_password)
            _cachedPassword = _requestPassword(connectionProperties, force_ask_password);
          else {
            bool is_cached_password_found = _findPassword(connectionProperties, _cachedPassword);
            if (!is_cached_password_found)
              _cachedPassword = ""; // try no password
          }
          properties["password"] = _cachedPassword;
          _cacheKey = parameter_values.get_string("userName") + std::string("@") + key;
        }
        _cacheTime = time(NULL);
      }
    }

    // passing some empty values confuse the connector
    {
      std::list<std::string> prop_names;
      prop_names.push_back("socket");
      prop_names.push_back("schema");
      for (const std::string &prop_name : prop_names) {
        ConnectOptionsMap::iterator prop_iter = properties.find(prop_name);
        if (properties.end() != prop_iter) {
          sql::SQLString *val = prop_iter->second.get<sql::SQLString>();
          if (val->compare("") == 0)
            properties.erase(prop_iter);
        }
      }
    }

    try {
      std::unique_ptr<Connection> conn(driver->connect(properties));
      std::string ssl_cipher;
      // make sure the user we got logged in is the user we wanted
      {
        std::unique_ptr<sql::Statement> statement(conn.get()->createStatement());
        std::unique_ptr<sql::ResultSet> rs(statement->executeQuery("SELECT current_user()"));
        if (rs->next()) {
          std::string current_user = rs->getString(1);
          if (current_user == "" && parameter_values.get_string("userName") != "") {
            // got logged in as anon-user when we wanted something else
            // if we got there by accident, try to login again after asking for pwd
            if (!force_ask_password) {
              if (authref) {
                authref->invalidate();
                throw AuthenticationError("Invalid username or password", authref);
              } else {
                force_ask_password = true;
                goto retry;
              }
            }
          }
        }
      }

      std::string sql_mode = parameter_values.get_string("SQL_MODE", "");
      if (!sql_mode.empty()) {
        std::unique_ptr<sql::Statement> statement(conn.get()->createStatement());
        statement->execute("SET SESSION SQL_MODE='" + sql_mode + "'");
      }

      if (connection_init_slot)
        connection_init_slot(conn.get(), connectionProperties);

      //  We could set this on the parameters, but we wouldn't know the server version
      std::unique_ptr<sql::Statement> stmt(conn.get()->createStatement());
      std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("show character set where charset = 'utf8mb4'"));
      if (res->rowsCount() >= 1) {
        stmt->executeUpdate("SET NAMES 'utf8mb4'");
      } else {
        stmt->executeUpdate("SET NAMES 'utf8'");
      }

      std::string def_schema = parameter_values.get_string("schema", "");
      if (!def_schema.empty())
        conn->setSchema(def_schema);

      return ConnectionWrapper(std::move(conn), tunnel);
    } catch (sql::SQLException &exc) {
      // authentication error
      if (exc.getErrorCode() == 0 && getClientLibVersionNumeric(driver) >= 80019) {
        throw sql::SQLException(exc.what(), exc.getSQLStateCStr(),
                                2003); //  Convert to to error 2003 as the previous connector
      } else if (exc.getErrorCode() == 1045 
                || exc.getErrorCode() == 1044 
                || exc.getErrorCode() == 1968 // ER_ACCESS_DENIED_NO_PASSWORD_ERROR
                || (exc.getErrorCode() == 2000 && drv->name() == "MysqlNativeKerberos")) {
        if (!force_ask_password) {
          if (authref) {
            authref->invalidate();
            throw AuthenticationError(exc.what(), authref);
          } else {
            // ask for password again, this time disablig the password caching
            force_ask_password = true;
            goto retry;
          }
        }
      }

      throw;
    } catch (...) {
      _cacheKey.clear();
      _cachedPassword.clear();

      throw;
    }
  }

} // namespace sql
