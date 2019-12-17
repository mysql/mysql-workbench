/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DRIVER_MANAGER_H_
#define _DRIVER_MANAGER_H_

#include "cppdbc_public_interface.h"

#include <cppconn/driver.h>
#include <memory>
#include <set>

#include "grts/structs.db.mgmt.h"
#include <cppconn/connection.h>

namespace wb {
  class SSHTunnel;
}

namespace sql {
  typedef std::shared_ptr<Connection> ConnectionPtr;

  class ConnectionWrapper {
    ConnectionPtr _conn;
    std::shared_ptr<wb::SSHTunnel> _tunnel_conn;

  public:
    ConnectionWrapper() {
    }
    ConnectionWrapper(std::shared_ptr<Connection> conn, std::shared_ptr<wb::SSHTunnel> tunn_conn)
      : _conn(conn), _tunnel_conn(tunn_conn) {
    }
    void reset() {
      _conn.reset();
    }
    Connection *operator->() {
      return _conn.get();
    }
    Connection *get() {
      return _conn.get();
    }
    ConnectionPtr get_ptr() {
      return _conn;
    }
  };

  class CPPDBC_PUBLIC_FUNC Authentication {
  protected:
    db_mgmt_ConnectionRef _props;
    std::string _service;
    char *_password;

    Authentication();
    Authentication(const db_mgmt_ConnectionRef &props, const std::string &service = "");

  public:
    typedef std::shared_ptr<Authentication> Ref;

    static Ref create(const db_mgmt_ConnectionRef &props, const std::string &service = "");
    ~Authentication();

    db_mgmt_ConnectionRef connectionProperties() const {
      return _props;
    }
    std::string service() const {
      return _service;
    }

    void set_password(const char *password);
    const char *password() const {
      return _password;
    }
    void invalidate();
    bool is_valid() const {
      return _password != NULL;
    }
    std::string uri(bool withPassword = false);
  };

  typedef std::set<Authentication::Ref> AuthenticationSet;

  class CPPDBC_PUBLIC_FUNC AuthenticationError : public grt::db_login_error {
    Authentication::Ref _authobj;

  public:
    AuthenticationError(const AuthenticationError &other)
      : grt::db_login_error(other.what()), _authobj(other._authobj) {
    }
    AuthenticationError(const std::string &what, Authentication::Ref auth) : grt::db_login_error(what), _authobj(auth) {
    }
    virtual ~AuthenticationError() {
    }

    Authentication::Ref authentication() {
      return _authobj;
    }
  };

  class CPPDBC_PUBLIC_FUNC DriverManager {
    std::string _driver_path;

    // Functions added to this map can be called from multiple unrelated threads, multiple times
    // be sure that the function is protected before that before appending any new call
    std::map<std::string, std::function<void()> > _drivers;

  public:
    // Returns the DriverManager singleton
    static DriverManager *getDriverManager();

    DriverManager();

    // Sets the path to the directory where drivers are located
    void set_driver_dir(const std::string &path);

    // Callback to initialize connection with DBMS specific startup script
    typedef std::function<void(Connection *, const db_mgmt_ConnectionRef &)> ConnectionInitSlot;

    // Returns a Connection object for the give connection params
    ConnectionWrapper getConnection(const db_mgmt_ConnectionRef &connectionProperties,
                                    ConnectionInitSlot connection_init_slot = ConnectionInitSlot());

    ConnectionWrapper getConnection(const db_mgmt_ConnectionRef &connectionProperties,
                                    std::shared_ptr<wb::SSHTunnel> tunnel, Authentication::Ref password,
                                    ConnectionInitSlot connection_init_slot = ConnectionInitSlot());

    void thread_cleanup();

    std::shared_ptr<wb::SSHTunnel> getTunnel(const db_mgmt_ConnectionRef &connectionProperties);

    // Returns the list of available drivers
    std::list<Driver *> getDrivers();

  public:
    typedef std::function<std::shared_ptr<wb::SSHTunnel>(const db_mgmt_ConnectionRef &)> TunnelFactoryFunction;
    typedef std::function<bool(const db_mgmt_ConnectionRef &, std::string &)> PasswordFindFunction;
    typedef std::function<std::string(const db_mgmt_ConnectionRef &, bool)> PasswordRequestFunction;

    void setTunnelFactoryFunction(TunnelFactoryFunction function);
    void setPasswordFindFunction(PasswordFindFunction function);
    void setPasswordRequestFunction(PasswordRequestFunction function);

    bool findStoredPassword(const db_mgmt_ConnectionRef &conn, std::string &password) {
      return _findPassword(conn, password);
    }
    std::string requestPassword(const db_mgmt_ConnectionRef &conn, bool forceAsk) {
      return _requestPassword(conn, forceAsk);
    }

    const std::string &getClientLibVersion() const;

  private:
    void getClientLibVersion(Driver *driver);
    unsigned int getClientLibVersionNumeric(Driver *driver);

    TunnelFactoryFunction _createTunnel;
    PasswordFindFunction _findPassword;
    PasswordRequestFunction _requestPassword;

    std::string _cachedPassword;
    std::string _cacheKey;
    time_t _cacheTime;
    std::string _versionInfo;
  };

  class Dbc_connection_handler {
  public:
    Dbc_connection_handler() : id(-1), autocommit_mode(true), is_stop_query_requested(false) {
    }
    typedef std::shared_ptr<Dbc_connection_handler> Ref;
    typedef ConnectionWrapper ConnectionRef;
    ConnectionRef ref;
    std::string name;
    std::int64_t id;
    std::string active_schema;
    std::string ssl_cipher;
    bool autocommit_mode;
    bool is_stop_query_requested;
  };
} // namespace sql

#endif // _DRIVER_MANAGER_H_
