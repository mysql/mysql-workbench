/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

class tunnel_auth_error : public std::runtime_error
{
public:
  tunnel_auth_error(const std::string &err) : std::runtime_error(err) {}
};

class tunnel_auth_retry : public std::runtime_error
{
public:
  tunnel_auth_retry(const std::string &err) : std::runtime_error(err) {}
};

class tunnel_auth_cancelled : public std::runtime_error
{
public:
  tunnel_auth_cancelled(const std::string &err) : std::runtime_error(err) {}
};

class tunnel_auth_key_error : public std::runtime_error
{
public:
  tunnel_auth_key_error(const std::string &err) : std::runtime_error(err) {}
};


class SSHTunnel : public sql::TunnelConnection
{
  TunnelManager *_tm;
  int _port;

public:
  SSHTunnel(TunnelManager *tm, int port)
  : _tm(tm), _port(port)
  {
  }

  virtual ~SSHTunnel()
  {
    disconnect();
  }

  virtual int get_port()
  {
    return _port;
  }

  virtual void connect(db_mgmt_ConnectionRef connectionProperties)
  {
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

  virtual void disconnect()
  {
    _tm->close_tunnel(_port);
  }

  virtual bool get_message(std::string &type, std::string &message)
  {
    return _tm->get_message_for(_port, type, message);
  }
};



TunnelManager::TunnelManager(wb::WBContext *wb)
: _wb(wb)
{
}


void TunnelManager::start()
{
  std::string progpath = bec::make_path(_wb->get_grt_manager()->get_basedir(), "sshtunnel.py");

  WillEnterPython lock;
  grt::PythonContext *py = grt::PythonContext::get();
  if (py->run_file(progpath, false) < 0)
  {
    g_warning("Tunnel manager could not be executed");
    throw std::runtime_error("Cannot start SSH tunnel manager");
  }
  _tunnel = py->eval_string("TunnelManager()");
}


int TunnelManager::lookup_tunnel(const char *server, const char *username, const char *target)
{
  WillEnterPython lock;

  // Note: without the (char*) cast gcc will complain about passing a const char* to a char*.
  //       Ideally the function signature should be changed to take a const char*.
  PyObject *ret = PyObject_CallMethod(_tunnel, (char*) "lookup_tunnel", (char*) "sss", server, username, target);
  if (!ret)
  {
    PyErr_Print();
    return -1;
  }
  if (ret == Py_None)
  {
    Py_XDECREF(ret);
    return -1;
  }
  int port = PyInt_AsLong(ret);
  Py_XDECREF(ret);
  return port;
}



void TunnelManager::shutdown()
{
  WillEnterPython lock;
  if (_tunnel)
  {
    PyObject *ret = PyObject_CallMethod(_tunnel, (char*) "shutdown", NULL);
    if (!ret)
    {
      PyErr_Print();
      return;
    }
    Py_XDECREF(ret);
  }
}


int TunnelManager::open_tunnel(const char *server, const char *username, const char *password, 
                               const char *keyfile, const char *target)
{
  WillEnterPython lock;
  PyObject *ret = PyObject_CallMethod(_tunnel, (char*) "open_tunnel", (char*) "sssss",
                                      server, username, password, keyfile, target);
  if (!ret)
  {
    PyErr_Print();
    throw std::runtime_error("Error calling TunnelManager.open_tunnel");
  }
  if (PyTuple_Size(ret) != 2)
  {
    Py_XDECREF(ret);
    throw std::runtime_error("TunnelManager.open_tunnel returned invalid value");
  }

  PyObject *status = PyTuple_GetItem(ret, 0);
  PyObject *value = PyTuple_GetItem(ret, 1);

  if (status == Py_False)
  {
    char *error = PyString_AsString(value);
    Py_XDECREF(ret);

    if (g_str_has_prefix(error, "Authentication error"))
      throw tunnel_auth_error(error);

    throw std::runtime_error(error);
  }
  else
  {
    int port = PyInt_AsLong(value);
    Py_XDECREF(ret);
    return port;
  }
}


void TunnelManager::wait_tunnel(int port)
{
  WillEnterPython lock;

  log_debug("Waiting on tunnel to connect...\n");

  PyObject *ret = PyObject_CallMethod(_tunnel, (char*) "wait_connection", (char*) "i", port);
  if (!ret)
  {
    PyErr_Print();
    log_error("TunnelManager.wait_connection had an uncaught python exception\n");
    throw std::runtime_error("Error calling TunnelManager.wait_connection");
  }
  if (ret == Py_None)
  {
    log_info("TunnelManager.wait_connection returned OK\n");
    Py_XDECREF(ret);
    return;
  }
  std::string str = PyString_AsString(ret);
  Py_XDECREF(ret);

  log_debug("TunnelManager.wait_connection() returned %s\n", str.c_str());

  if ( g_str_has_prefix(str.c_str(), "Bad authentication type") ||
       g_str_has_prefix(str.c_str(), "Private key file is encrypted") ||
       g_str_has_prefix(str.c_str(), "Authentication failed")              )
    throw tunnel_auth_error("Authentication error. Please check that your username and password are correct and try again."
            "\nDetails (Original exception message):\n" + str);

  if (g_str_has_prefix(str.c_str(), "Server key has been stored"))
  {
    log_info("TunnelManager.wait_connection server key stored, retrying: %s\n", str.c_str());
    throw tunnel_auth_retry("Retry due to fingerprint missing, user accept new fingerprint");
  }

  if (g_str_has_prefix(str.c_str(), "Host key for server "))
  {
    log_info("TunnelManager.wait_connection host key does not match, abandoning connection: %s\n", str.c_str());
    throw tunnel_auth_key_error(str);
  }

  if (g_str_has_prefix(str.c_str(), "User cancelled"))
  {
    log_info("TunnelManager.wait_connection cancelled by the user: %s\n", str.c_str());
    throw tunnel_auth_cancelled("Tunnel connection cancelled by the user");
  }
  if (g_str_has_prefix(str.c_str(), "IO Error"))
  {
    log_error("TunnelManager.wait_connection got IOError: %s\n", str.c_str());
    throw tunnel_auth_key_error(str);
  }

  if (g_str_has_prefix(str.c_str(), "Authentication error"))
  {
    log_info("TunnelManager.wait_connection authentication error: %s\n", str.c_str());
    throw tunnel_auth_error(str);
  }

  throw std::runtime_error("Error connecting SSH tunnel: "+str);
}


bool TunnelManager::get_message_for(int port, std::string &type, std::string &message)
{
  std::list<std::pair<std::string, std::string> > messages;

  WillEnterPython lock;

  PyObject *ret = PyObject_CallMethod(_tunnel, (char*) "get_message", (char*) "i", port);
  if (!ret)
  {
    PyErr_Print();
    log_error("TunnelManager.get_message had an uncaught python exception\n");
    throw std::runtime_error("Error calling TunnelManager.get_message");
  }
  if (ret == Py_None)
  {
    Py_XDECREF(ret);
    return false;
  }

  if (!PyTuple_Check(ret) || PyTuple_GET_SIZE(ret) != 2)
  {
    Py_XDECREF(ret);
    log_error("TunnelManager.get_message returned unexpected value\n");
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


void TunnelManager::close_tunnel(int port)
{
  WillEnterPython lock;
  PyObject *ret = PyObject_CallMethod(_tunnel, (char*) "close", (char*) "i", port);
  if (!ret)
  {
    PyErr_Print();
    return;
  }
  Py_XDECREF(ret);
}


boost::shared_ptr<sql::TunnelConnection> TunnelManager::create_tunnel(db_mgmt_ConnectionRef connectionProperties)
{
  if (!_tunnel)
  {
    log_info("Starting tunnel\n");
    start();
  }

  boost::shared_ptr<sql::TunnelConnection> tunnel;
  grt::DictRef parameter_values= connectionProperties->parameterValues();

  if (connectionProperties->driver()->name() == "MysqlNativeSSH")
  {
    std::string server = parameter_values.get_string("sshHost");
    std::string username = parameter_values.get_string("sshUserName");
    std::string password = parameter_values.get_string("sshPassword");
    std::string keyfile = base::expand_tilde(parameter_values.get_string("sshKeyFile"));
    std::string target = parameter_values.get_string("hostName");
    size_t target_port = parameter_values.get_int("port", 3306);

    target += ":" + base::to_string(target_port);

    // before anything, check if a tunnel already exists for this server/user/target tuple
    _wb->get_grt_manager()->replace_status_text("Looking for existing SSH tunnel to "+server+"...");
    int tunnel_port;
    tunnel_port = lookup_tunnel(server.c_str(), username.c_str(), target.c_str());
    if (tunnel_port > 0)
    {
      _wb->get_grt_manager()->replace_status_text("Existing SSH tunnel found, connecting...");
      log_info("Existing SSH tunnel found, connecting\n");
      tunnel = boost::shared_ptr<sql::TunnelConnection>(new ::SSHTunnel(this, tunnel_port));
    }
    else
    {
      bool reset_password = false;
    retry:

      _wb->get_grt_manager()->replace_status_text("Existing SSH tunnel not found, opening new one...");
      log_info("Existing SSH tunnel not found, opening new one\n");
      std::string service;
      if (keyfile.empty() && password.empty())
      {
        // interactively ask user for password
        service = strfmt("ssh@%s", server.c_str());
        if (!mforms::Utilities::credentials_for_service(_("Open SSH Tunnel"),
                                                        service,
                                                        username,
                                                        reset_password,
                                                        password))
          // we need to throw an exception to signal that tunnel could not be opened (and not that it was not needed)
          throw grt::user_cancelled("SSH password input cancelled by user");
      }
      if (!keyfile.empty())
      {
        bool encrypted = true;
        char *contents = NULL;
        gsize length;
        // check if the keyfile is encrypted
        if (g_file_get_contents(keyfile.c_str(), &contents, &length, NULL) && contents)
        {
          if (!g_strstr_len(contents, length, "ENCRYPTED"))
            encrypted = false;
        }

        // interactively ask user for SSH key passphrase
        service = strfmt("ssh_keyfile@%s", keyfile.c_str());
        if (encrypted && !mforms::Utilities::find_or_ask_for_password(_("Open SSH Tunnel"),
                                                                     service,
                                                                     username,
                                                                     reset_password,
                                                                     password))
          // we need to throw an exception to signal that tunnel could not be opened (and not that it was not needed)
          throw std::runtime_error("SSH key passphrase input cancelled by user");
      }

      _wb->get_grt_manager()->replace_status_text("Opening SSH tunnel to "+server+"...");
      log_info("Opening SSH tunnel to %s\n", server.c_str());

      try
      {
        tunnel_port = open_tunnel(server.c_str(), username.c_str(), password.c_str(), keyfile.c_str(), target.c_str());

        _wb->get_grt_manager()->replace_status_text("SSH tunnel opened, connecting...");

        tunnel = boost::shared_ptr<sql::TunnelConnection>(new ::SSHTunnel(this, tunnel_port));

        if (tunnel)
        {
          tunnel->connect(connectionProperties);
          log_info("SSH tunnel connect executed OK\n");
        }
      }
      catch (tunnel_auth_error &exc)
      {
        log_error("Authentication error opening SSH tunnel: %s\n", exc.what());
        _wb->get_grt_manager()->replace_status_text("Authentication error opening SSH tunnel");
        if (mforms::Utilities::show_error("Could not connect the SSH Tunnel", exc.what(), _("Retry"), _("Cancel")) == mforms::ResultOk)
        {
          reset_password= true;
          mforms::Utilities::forget_password(service, username);
          password = "";
          goto retry;
        }
        else
          throw grt::user_cancelled("Tunnel connection cancelled");
      }
      catch (tunnel_auth_retry &exc)
      {
        log_warning("Opening SSH tunnel: %s\n", exc.what());
        goto retry;
      }
      catch (tunnel_auth_cancelled &exc)
      {
        log_debug("Tunnel auth cancelled: %s\n", exc.what());
        throw grt::user_cancelled(exc.what());
      }
      catch (tunnel_auth_key_error &exc)
      {
        mforms::Utilities::show_error("Tunnel Connection Error", exc.what(), _("OK"));
        log_debug("Tunnel auth key error: %s\n", exc.what());
        throw grt::user_cancelled(exc.what());
      }
      catch (std::exception &exc)
      {
        log_error("Exception while opening SSH tunnel: %s\n", exc.what());
        _wb->get_grt_manager()->replace_status_text("Could not open SSH tunnel");
        throw std::runtime_error(std::string("Cannot open SSH Tunnel: ").append(exc.what()));
      }
    }
    
    _wb->get_grt_manager()->replace_status_text("Using SSH tunnel to "+server);
  }

  return tunnel;
}


TunnelManager::~TunnelManager()
{
  shutdown();
}
