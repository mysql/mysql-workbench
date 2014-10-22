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

#include "driver_manager.h"

#include "mysql_driver.h"
#include "cppconn/driver.h"
#include "cppconn/statement.h"
#include "cppconn/exception.h"

#include <gmodule.h>
#include <boost/foreach.hpp>

namespace sql {

typedef std::map<std::string, std::string> Param_types;


static bool conv_to_dbc_value(const std::string &key, const grt::ValueRef value, ConnectOptionsMap &properties, Param_types &param_types)
{
  ConnectPropertyVal tmp;

  switch (value.type())
  {
  case grt::IntegerType:
    {
    grt::IntegerRef val= grt::IntegerRef::cast_from(value);
    std::string param_type;
    if (param_types.find(key) != param_types.end())
      param_type= param_types[key];
    if (param_type == "tristate")
      tmp = (int)(*val != 0);
    else if (param_type == "boolean")
      tmp = (bool)(*val != 0);
    else
      tmp = (int)(*val);
    properties[key] = tmp;
    }
    break;

  case grt::DoubleType:
    {
    grt::DoubleRef val= grt::DoubleRef::cast_from(value);
    tmp = *val;
    properties[key] = tmp;
    }
    break;

  case grt::StringType:
    {
    grt::StringRef val= grt::StringRef::cast_from(value);
    tmp = SQLString(val.c_str(), (*val).length());
    properties[key] = tmp;
    }
    break;

  case grt::UnknownType:
  //case grt::AnyType: // equal to grt::UnknownType
  case grt::ListType:
  case grt::DictType:
  case grt::ObjectType:
    break;
  }

  return true;
}

  
////

Authentication::Ref Authentication::create(const db_mgmt_ConnectionRef &props, const std::string &service)
{
  return Authentication::Ref(new Authentication(props, service));
}


Authentication::Authentication(const db_mgmt_ConnectionRef &props, const std::string &service)
  : _props(props), _service(service), _password(NULL)
{
}


Authentication::Authentication()
: _password(NULL)
{
}


Authentication::~Authentication()
{
  invalidate();
}


void Authentication::set_password(const char *password)
{
  invalidate();
  _password = g_strdup(password);
}


void Authentication::invalidate()
{
  if (_password != NULL)
  {
    memset(_password, 0, strlen(_password));
    g_free(_password);
    _password = NULL;
  }
}  

//----------------- DriverManager ------------------------------------------------------------------

DriverManager *DriverManager::getDriverManager()
{
  static DriverManager *dm= new DriverManager;
  return dm;
}


DriverManager::DriverManager()
:
_driver_path("."),
_cacheTime(0)
{
}

  

void DriverManager::setTunnelFactoryFunction(TunnelFactoryFunction function)
{
  _createTunnel = function;
}

  
void DriverManager::setPasswordFindFunction(PasswordFindFunction function)
{
  _findPassword = function;
}


void DriverManager::setPasswordRequestFunction(PasswordRequestFunction function)
{
  _requestPassword = function;
}
  

  
void DriverManager::set_driver_dir(const std::string &path)
{
  _driver_path= path;
}

boost::shared_ptr<TunnelConnection> DriverManager::getTunnel(const db_mgmt_ConnectionRef &connectionProperties)
{
  db_mgmt_DriverRef drv = connectionProperties->driver();
  if (!drv.is_valid())
    throw SQLException("Invalid connection settings: undefined connection driver");

  if (_createTunnel)
    return _createTunnel(connectionProperties);
  return boost::shared_ptr<TunnelConnection>();
}

//--------------------------------------------------------------------------------------------------

#define MYSQL_PASSWORD_CACHE_TIMEOUT 60

ConnectionWrapper DriverManager::getConnection(const db_mgmt_ConnectionRef &connectionProperties, ConnectionInitSlot connection_init_slot)
{
  db_mgmt_DriverRef drv = connectionProperties->driver();
  if (!drv.is_valid())
    throw SQLException("Invalid connection settings: undefined connection driver");

  boost::shared_ptr<TunnelConnection> tunnel;
  if (_createTunnel)
  {
    tunnel = _createTunnel(connectionProperties);
    
    if (tunnel)
    {
      // this can throw an exception if the tunnel can't be created
      //!tunnel->connect();
    }
  }
  return getConnection(connectionProperties, tunnel, Authentication::Ref(), connection_init_slot);
}
//--------------------------------------------------------------------------------------------------
// This method is called when each dispatcher is ending is about to be gone
// it needs to be called right after that to cleanup the thread storage allocated by driver.
// If we will not free the mem then after wb close we will get error about "threads didn't exit"
void DriverManager::thread_cleanup()
{
  std::map<std::string, boost::function<void ()> >::iterator it;
  for(it = _drivers.begin(); it != _drivers.end(); ++it)
    it->second();
}

//--------------------------------------------------------------------------------------------------

ConnectionWrapper DriverManager::getConnection(const db_mgmt_ConnectionRef &connectionProperties,
  boost::shared_ptr<TunnelConnection> tunnel, Authentication::Ref password,
  ConnectionInitSlot connection_init_slot)
{
  grt::DictRef parameter_values = connectionProperties->parameterValues();
  if (parameter_values.get_string("userName").empty())
    throw SQLException("No user name set for this connection");

  // Load the driver for the connection dynamically. However for the C++ connector we have a static
  // link anyway, so use this instead.

  // 0. determine correct driver filename
  db_mgmt_DriverRef drv = connectionProperties->driver();
  
  std::string library= "";
  if (drv.is_valid())
    library = drv->driverLibraryName();
  else
    throw SQLException("Invalid connection settings: undefined connection driver");

  Driver *driver;
  if (library == "mysqlcppconn")
    driver = get_driver_instance();
  else
  {
  #ifdef _WIN32
    library.append(".dll");
  #elif defined(__APPLE__)
    library.append(".dylib");
  #else
    library.append(".so");
  #endif

    // 1. find driver
    GModule *gmodule= g_module_open((_driver_path + "/" + library).c_str(), G_MODULE_BIND_LOCAL);
    if (NULL == gmodule)
    {
      fprintf(stderr, "Error: %s", g_module_error());
      throw SQLException(std::string("Database driver: Failed to open library '").append(_driver_path + "/" + library).append("'. Check settings.").c_str());
    }

    Driver *(* get_driver_instance)()= NULL;
    g_module_symbol(gmodule, "sql_mysql_get_driver_instance", (gpointer*)&get_driver_instance);
    if (NULL == get_driver_instance)
    {
      g_module_close(gmodule);
      throw SQLException("Database driver: Failed to get library instance. Check settings.");
    }

    driver = get_driver_instance();
  }
  if (driver == NULL)
    throw SQLException("Database driver: Failed to get driver instance. Check  settings.");
  else
    _drivers[library] = boost::bind(&Driver::threadEnd, driver);

  // 2. call driver->connect()
  Param_types param_types;
  {
    grt::ListRef<db_mgmt_DriverParameter> params= connectionProperties->driver()->parameters();
    for (size_t n = 0, count = params.count(); n < count; ++n)
    {
      db_mgmt_DriverParameterRef param= params.get(n);
      param_types[param->name()]= param->paramType();
    }
    // set the values that take boolean options, so that they can be converted correctly in case they're specified manually by the user
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

    param_types["useLegacyAuth"] = "boolean";
  }
  ConnectOptionsMap properties;
  parameter_values.foreach(boost::bind(&conv_to_dbc_value, _1, _2, boost::ref(properties), param_types));

  {
    ConnectPropertyVal tmp;
    const int conn_timeout = 60;
    const int read_timeout = 600;
    if (properties.find("OPT_CONNECT_TIMEOUT") == properties.end())
      properties["OPT_CONNECT_TIMEOUT"] = conn_timeout;
    if (properties.find("OPT_READ_TIMEOUT") == properties.end())
      properties["OPT_READ_TIMEOUT"] = read_timeout;
  }
  properties["OPT_CAN_HANDLE_EXPIRED_PASSWORDS"] = true;
  properties["CLIENT_MULTI_STATEMENTS"] = true;
  properties["metadataUseInfoSchema"] = false; // I_S is way too slow for many things as of MySQL 5.6.10, so disable it for now
#if defined(__APPLE__) || defined(_WIN32) || defined(MYSQLCPPCONN_VERSION_1_1_4)
  // set application name
  {
    std::map< sql::SQLString, sql::SQLString > attribs;
    attribs["program_name"] = "MySQLWorkbench";
    properties["OPT_CONNECT_ATTR_ADD"] = attribs;
  }
#endif
  // If SSL is enabled but there's no certificate or anything, create the sslKey option to force enabling SSL without a key
  // (equivalent to starting cmdline client with mysql --ssl-key=)
  if (parameter_values.get_string("sslKey", "") == "" && parameter_values.get_int("useSSL", 0) != 0)
    properties["sslKey"] = std::string();

  // If SSL is not enabled, clear all ssl related props to not confuse the connector
  if (parameter_values.get_int("useSSL", 0) == 0)
  {
    properties.erase("sslKey");
    properties.erase("sslCert");
    properties.erase("sslCA");
    properties.erase("sslCAPath");
    properties.erase("sslCipher");
  }

  // If we are on a pipe connection then set the host name explicitly.
  // However, pipe connections can only be established on the local box (Win only).
  if (drv->name() == "MysqlNativeSocket")
  {
#ifdef _WIN32
    properties["hostName"] = std::string(".");
#else
    properties["hostName"] = std::string();
#endif
  }

  if (tunnel)
  {
    // Make the driver connect to the local tunnel port.
    properties["port"] = tunnel->get_port();
    properties["hostName"] = sql::SQLString("127.0.0.1");
  }
  else
    properties["hostName"] = "["+parameter_values.get_string("hostName")+"]"; // [quote] hostname so that ipv6 addresses aren't parsed as URIs


  Authentication::Ref authref;

  // Check if there is a stored or cached password, if there isn't try without one (blank)
  // If we get an auth error, then we ask for the password
  bool force_ask_password = false;
retry:
  if (password)
  {
    authref = password;
    if (password->is_valid())
      properties["password"] = std::string(authref->password());
  }
  else
  {
    // password not in profile (and no keyfile provided)
    if (_requestPassword && (force_ask_password || parameter_values.get_string("password") == ""))
    {
      // check if we have cached the password for this connection
      if (time(NULL) - _cacheTime > MYSQL_PASSWORD_CACHE_TIMEOUT || force_ask_password)
      {
        _cacheKey.clear();
        _cachedPassword.clear();
      }
      
      std::string key = connectionProperties->hostIdentifier();
      if (parameter_values.get_string("userName") + std::string("@") + key == _cacheKey && !key.empty()) // hostIdentifier will be "" if it's a temporary connection
        properties["password"] = _cachedPassword;
      else
      {
        if (force_ask_password)
          _cachedPassword = _requestPassword(connectionProperties, force_ask_password);
        else
        {
          bool is_cached_password_found= _findPassword(connectionProperties, _cachedPassword);
          if (!is_cached_password_found)
            _cachedPassword= ""; // try no password
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
    BOOST_FOREACH (const std::string &prop_name, prop_names)
    {
      ConnectOptionsMap::iterator prop_iter= properties.find(prop_name);
      if (properties.end() != prop_iter)
      {
#ifdef MYSQLCPPCONN_VERSION_1_1_5
        sql::SQLString *val= prop_iter->second.get<sql::SQLString>();
        if (val->compare("") == 0)
          properties.erase(prop_iter);
#else
        sql::SQLString &val= boost::get<sql::SQLString>(prop_iter->second);
        if (val->empty())
          properties.erase(prop_iter);        
#endif        
      }
    }
  }

  try
  {
    std::auto_ptr<Connection> conn(driver->connect(properties));
    std::string ssl_cipher;

    // make sure that SSL got enabled if it was requested to be required
    // TODO: there's a client lib option to do this, but C/C++ does not support as of 1.1.4
    if (parameter_values.get_int("useSSL", 0) > 1)
    {
      boost::scoped_ptr<sql::Statement> statement(conn.get()->createStatement());
      boost::scoped_ptr<sql::ResultSet> rs(statement->executeQuery("SHOW SESSION STATUS LIKE 'Ssl_cipher'"));
      if (rs->next())
      {
        ssl_cipher = rs->getString(2);
        if (ssl_cipher.empty())
          throw std::runtime_error("Unable to establish SSL connection");
      }
    }
    
    // make sure the user we got logged in is the user we wanted
    {
      boost::scoped_ptr<sql::Statement> statement(conn.get()->createStatement());
      boost::scoped_ptr<sql::ResultSet> rs(statement->executeQuery("SELECT current_user()"));
      if (rs->next())
      {
        std::string current_user = rs->getString(1);
        if (current_user == "" && parameter_values.get_string("userName") != "")
        {
          // got logged in as anon-user when we wanted something else
          // if we got there by accident, try to login again after asking for pwd
          if (!force_ask_password)
          {
            if (authref)
            {
              authref->invalidate();
              throw AuthenticationError("Invalid username or password", authref);
            }
            else
            {
              force_ask_password = true;
              goto retry;
            }
          }
        }
      }
    }
    
    std::string sql_mode = parameter_values.get_string("SQL_MODE", "");
    if (!sql_mode.empty())
    {
      boost::scoped_ptr<sql::Statement> statement(conn.get()->createStatement());
      statement->execute("SET SESSION SQL_MODE='"+sql_mode+"'");
    }

    if (connection_init_slot)
      connection_init_slot(conn.get(), connectionProperties);
    
    std::string def_schema= parameter_values.get_string("schema", "");
    if (!def_schema.empty())
      conn->setSchema(def_schema);

    return ConnectionWrapper(conn, tunnel);
  }
  catch (sql::SQLException &exc)
  {
    // authentication error
    if (exc.getErrorCode() == 1045 || exc.getErrorCode() == 1044
        || exc.getErrorCode() == 1968 // ER_ACCESS_DENIED_NO_PASSWORD_ERROR
        )
    {
      if (!force_ask_password)
      {
        if (authref)
        {
          authref->invalidate();
          throw AuthenticationError(exc.what(), authref);
        }
        else
        {          
          // ask for password again, this time disablig the password caching
          force_ask_password = true;
          goto retry;
        }
      }
    }

    throw;
  }
  catch (...)
  {

    _cacheKey.clear();
    _cachedPassword.clear();
    
    throw;
  }
}


} // namespace sql
