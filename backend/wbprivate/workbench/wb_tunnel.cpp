/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_tunnel.h"
#include "wb_context.h"

#include "base/string_utilities.h"
#include "base/file_utilities.h"

#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <sstream>
#include "boost/scoped_array.hpp"

#include "base/log.h"

DEFAULT_LOG_DOMAIN("SSH tunnel")

using namespace wb;
using namespace base;

class tunnel_auth_error : public std::runtime_error {
public:
  tunnel_auth_error(const std::string &err) : std::runtime_error(err) {
  }
};

class tunnel_auth_retry : public std::runtime_error {
public:
  tunnel_auth_retry(const std::string &err) : std::runtime_error(err) {
  }
};

class tunnel_auth_cancelled : public std::runtime_error {
public:
  tunnel_auth_cancelled(const std::string &err) : std::runtime_error(err) {
  }
};

class tunnel_auth_key_error : public std::runtime_error {
public:
  tunnel_auth_key_error(const std::string &err) : std::runtime_error(err) {
  }
};

class SSHTunnel : public sql::TunnelConnection {
  TunnelManager *_tm;
  int _port;

public:
  SSHTunnel(TunnelManager *tm, int port) : _tm(tm), _port(port) {
  }

  virtual ~SSHTunnel() {
    disconnect();
  }

  virtual int get_port() {
    return _port;
  }

  virtual void connect(db_mgmt_ConnectionRef connectionProperties) {
    if (_port == 0)
      throw std::runtime_error("Could not connect SSH tunnel");

    _tm->wait_tunnel(_port);

    /*
    if (!g_str_has_prefix(result.c_str(), "OK"))
    {
      if (g_str_has_prefix(result.c_str(), "Private key file is encrypted"))
      {
        std::string password;
        _tm->wb()->request_input("Enter passphrase for key", 1, password);
        if (!password.empty())
        {
          grt::DictRef parameter_values= connectionProperties->parameterValues();
          parameter_values["sshPassword"]= grt::StringRef(password);
          connect(connectionProperties);
          return;
        }
      }

      throw std::runtime_error("Could not connect SSH tunnel: "+result);
    }*/
  }

  virtual void disconnect() {
  }

  virtual bool get_message(std::string &type, std::string &message) {
    return _tm->get_message_for(_port, type, message);
  }
};

TunnelManager::TunnelManager(wb::WBContext *wb) : _wb(wb) {
}

void TunnelManager::start() {
  std::string progpath = base::makePath(bec::GRTManager::get()->get_basedir(), "sshtunnel.py");

  WillEnterPython lock;
  grt::PythonContext *py = grt::PythonContext::get();
  if (py->run_file(progpath, false) < 0) {
    g_warning("Tunnel manager could not be executed");
    throw std::runtime_error("Cannot start SSH tunnel manager");
  }
  _tunnel = py->eval_string("TunnelManager()");
}

int TunnelManager::lookup_tunnel(const char *server, const char *username, const char *target) {
  WillEnterPython lock;

  // Note: without the (char*) cast gcc will complain about passing a const char* to a char*.
  //       Ideally the function signature should be changed to take a const char*.
  PyObject *ret = PyObject_CallMethod(_tunnel, (char *)"lookup_tunnel", (char *)"sss", server, username, target);
  if (!ret) {
    PyErr_Print();
    return -1;
  }
  if (ret == Py_None) {
    Py_XDECREF(ret);
    return -1;
  }
  int port = (int)PyInt_AsLong(ret);
  Py_XDECREF(ret);
  return port;
}

void TunnelManager::shutdown() {
  WillEnterPython lock;
  if (_tunnel) {
    PyObject *ret = PyObject_CallMethod(_tunnel, (char *)"shutdown", NULL);
    if (!ret) {
      PyErr_Print();
      return;
    }
    Py_XDECREF(ret);
  }
}

int TunnelManager::open_tunnel(const char *server, const char *username, const char *password, const char *keyfile,
                               const char *target) {
  WillEnterPython lock;
  PyObject *ret =
    PyObject_CallMethod(_tunnel, (char *)"open_tunnel", (char *)"sssss", server, username, password, keyfile, target);
  if (!ret) {
    PyErr_Print();
    throw std::runtime_error("Error calling TunnelManager.open_tunnel");
  }
  if (PyTuple_Size(ret) != 2) {
    Py_XDECREF(ret);
    throw std::runtime_error("TunnelManager.open_tunnel returned invalid value");
  }

  PyObject *status = PyTuple_GetItem(ret, 0);
  PyObject *value = PyTuple_GetItem(ret, 1);

  if (status == Py_False) {
    char *error = PyString_AsString(value);
    Py_XDECREF(ret);

    if (g_str_has_prefix(error, "Authentication error"))
      throw tunnel_auth_error(error);

    throw std::runtime_error(error);
  } else {
    int port = (int)PyInt_AsLong(value);
    Py_XDECREF(ret);
    return port;
  }
}

void TunnelManager::wait_tunnel(int port) {
  WillEnterPython lock;

  logDebug("Waiting on tunnel to connect...\n");

  PyObject *ret = PyObject_CallMethod(_tunnel, (char *)"wait_connection", (char *)"i", port);
  if (!ret) {
    PyErr_Print();
    logError("TunnelManager.wait_connection had an uncaught python exception\n");
    throw std::runtime_error("Error calling TunnelManager.wait_connection");
  }
  if (ret == Py_None) {
    logInfo("TunnelManager.wait_connection returned OK\n");
    Py_XDECREF(ret);
    return;
  }
  std::string str = PyString_AsString(ret);
  Py_XDECREF(ret);

  logDebug("TunnelManager.wait_connection() returned %s\n", str.c_str());

  if (g_str_has_prefix(str.c_str(), "Bad authentication type") ||
      g_str_has_prefix(str.c_str(), "Private key file is encrypted") ||
      g_str_has_prefix(str.c_str(), "Authentication failed"))
    throw tunnel_auth_error(
      "Authentication error. Please check that your username and password are correct and try again."
      "\nDetails (Original exception message):\n" +
      str);

  if (g_str_has_prefix(str.c_str(), "Server key has been stored")) {
    logInfo("TunnelManager.wait_connection server key stored, retrying: %s\n", str.c_str());
    throw tunnel_auth_retry("Retry due to fingerprint missing, user accept new fingerprint");
  }

  if (g_str_has_prefix(str.c_str(), "Host key for server ")) {
    logInfo("TunnelManager.wait_connection host key does not match, abandoning connection: %s\n", str.c_str());
    throw tunnel_auth_key_error(str);
  }

  if (g_str_has_prefix(str.c_str(), "User cancelled")) {
    logInfo("TunnelManager.wait_connection cancelled by the user: %s\n", str.c_str());
    throw tunnel_auth_cancelled("Tunnel connection cancelled by the user");
  }
  if (g_str_has_prefix(str.c_str(), "IO Error")) {
    logError("TunnelManager.wait_connection got IOError: %s\n", str.c_str());
    throw tunnel_auth_key_error(str);
  }

  if (g_str_has_prefix(str.c_str(), "Authentication error")) {
    logInfo("TunnelManager.wait_connection authentication error: %s\n", str.c_str());
    throw tunnel_auth_error(str);
  }

  throw std::runtime_error("Error connecting SSH tunnel: " + str);
}

bool TunnelManager::get_message_for(int port, std::string &type, std::string &message) {
  std::list<std::pair<std::string, std::string> > messages;

  WillEnterPython lock;

  PyObject *ret = PyObject_CallMethod(_tunnel, (char *)"get_message", (char *)"i", port);
  if (!ret) {
    PyErr_Print();
    logError("TunnelManager.get_message had an uncaught python exception\n");
    throw std::runtime_error("Error calling TunnelManager.get_message");
  }
  if (ret == Py_None) {
    Py_XDECREF(ret);
    return false;
  }

  if (!PyTuple_Check(ret) || PyTuple_GET_SIZE(ret) != 2) {
    Py_XDECREF(ret);
    logError("TunnelManager.get_message returned unexpected value\n");
    return false;
  }

  PyObject *obj = PyTuple_GetItem(ret, 0);
  if (obj && PyString_Check(obj))
    type = PyString_AsString(obj);

  obj = PyTuple_GetItem(ret, 1);
  if (obj && PyString_Check(obj))
    message = PyString_AsString(obj);

  Py_XDECREF(ret);

  return true;
}

void TunnelManager::set_keepalive(int port, int keepalive) {
  WillEnterPython lock;
  PyObject *ret = PyObject_CallMethod(_tunnel, (char *)"set_keepalive", (char *)"ii", port, keepalive);
  if (!ret) {
    PyErr_Print();
    return;
  }
  Py_XDECREF(ret);
}

std::shared_ptr<sql::TunnelConnection> TunnelManager::create_tunnel(db_mgmt_ConnectionRef connectionProperties) {
  std::shared_ptr<sql::TunnelConnection> tunnel;
  grt::DictRef parameter_values = connectionProperties->parameterValues();

  if (connectionProperties->driver()->name() == "MysqlNativeSSH") {
    if (!_tunnel) {
      logInfo("Starting tunnel\n");
      start();
    }

    std::string server = parameter_values.get_string("sshHost");
    std::string username = parameter_values.get_string("sshUserName");
    std::string password = parameter_values.get_string("sshPassword");
    std::string keyfile = base::expand_tilde(parameter_values.get_string("sshKeyFile"));
    std::string target = parameter_values.get_string("hostName");
    size_t target_port = parameter_values.get_int("port", 3306);

    target += ":" + std::to_string(target_port);

    // before anything, check if a tunnel already exists for this server/user/target tuple
    bec::GRTManager::get()->replace_status_text("Looking for existing SSH tunnel to " + server + "...");
    int tunnel_port;
    tunnel_port = lookup_tunnel(server.c_str(), username.c_str(), target.c_str());
    if (tunnel_port > 0) {
      bec::GRTManager::get()->replace_status_text("Existing SSH tunnel found, connecting...");
      logInfo("Existing SSH tunnel found, connecting\n");
      tunnel = std::shared_ptr<sql::TunnelConnection>(new ::SSHTunnel(this, tunnel_port));
    } else {
      bool reset_password = false;
    retry:

      bec::GRTManager::get()->replace_status_text("Existing SSH tunnel not found, opening new one...");
      logInfo("Existing SSH tunnel not found, opening new one\n");
      std::string service;
      if (keyfile.empty() && password.empty()) {
        // interactively ask user for password
        service = strfmt("ssh@%s", server.c_str());

        bool result = false;
        try {
          result = mforms::Utilities::credentials_for_service(_("Open SSH Tunnel"), service, username, reset_password,
                                                              password);
        } catch (std::exception &exc) {
          logWarning("Exception caught on credentials_for_service: %s", exc.what());
          mforms::Utilities::show_error("Clear Password", base::strfmt("Could not clear password: %s", exc.what()),
                                        "OK");
        }

        if (!result)
          // we need to throw an exception to signal that tunnel could not be opened (and not that it was not needed)
          throw grt::user_cancelled("SSH password input cancelled by user");
      }
      if (!keyfile.empty()) {
        bool encrypted = true;
        char *contents = NULL;
        gsize length;
        // check if the keyfile is encrypted
        if (g_file_get_contents(keyfile.c_str(), &contents, &length, NULL) && contents) {
          if (!g_strstr_len(contents, length, "ENCRYPTED"))
            encrypted = false;
        }

        // interactively ask user for SSH key passphrase
        service = strfmt("ssh_keyfile@%s", keyfile.c_str());
        if (encrypted &&
            !mforms::Utilities::find_or_ask_for_password(_("Open SSH Tunnel"), service, username, reset_password,
                                                         password))
          // we need to throw an exception to signal that tunnel could not be opened (and not that it was not needed)
          throw std::runtime_error("SSH key passphrase input cancelled by user");
      }

      bec::GRTManager::get()->replace_status_text("Opening SSH tunnel to " + server + "...");
      logInfo("Opening SSH tunnel to %s\n", server.c_str());

      try {
        tunnel_port = open_tunnel(server.c_str(), username.c_str(), password.c_str(), keyfile.c_str(), target.c_str());

        bec::GRTManager::get()->replace_status_text("SSH tunnel opened, connecting...");

        tunnel = std::shared_ptr<sql::TunnelConnection>(new ::SSHTunnel(this, tunnel_port));

        if (tunnel) {
          tunnel->connect(connectionProperties);
          set_keepalive(tunnel_port, (int)bec::GRTManager::get()->get_app_option_int("sshkeepalive", 0));
          logInfo("SSH tunnel connect executed OK\n");
        }
      } catch (tunnel_auth_error &exc) {
        logError("Authentication error opening SSH tunnel: %s\n", exc.what());
        bec::GRTManager::get()->replace_status_text("Authentication error opening SSH tunnel");
        if (mforms::Utilities::show_error("Could not connect the SSH Tunnel", exc.what(), _("Retry"), _("Cancel")) ==
            mforms::ResultOk) {
          reset_password = true;
          try {
            mforms::Utilities::forget_password(service, username);
          } catch (std::exception &exc) {
            logWarning("Could not clear password: %s\n", exc.what());
          }

          password = "";
          goto retry;
        } else
          throw grt::user_cancelled("Tunnel connection cancelled");
      } catch (tunnel_auth_retry &exc) {
        logWarning("Opening SSH tunnel: %s\n", exc.what());
        goto retry;
      } catch (tunnel_auth_cancelled &exc) {
        logDebug("Tunnel auth cancelled: %s\n", exc.what());
        throw grt::user_cancelled(exc.what());
      } catch (tunnel_auth_key_error &exc) {
        mforms::Utilities::show_error("Tunnel Connection Error", exc.what(), _("OK"));
        logDebug("Tunnel auth key error: %s\n", exc.what());
        throw grt::user_cancelled(exc.what());
      } catch (std::exception &exc) {
        logError("Exception while opening SSH tunnel: %s\n", exc.what());
        bec::GRTManager::get()->replace_status_text("Could not open SSH tunnel");
        throw std::runtime_error(std::string("Cannot open SSH Tunnel: ").append(exc.what()));
      }
    }

    bec::GRTManager::get()->replace_status_text("Using SSH tunnel to " + server);
  }

  return tunnel;
}

TunnelManager::~TunnelManager() {
  shutdown();
}
