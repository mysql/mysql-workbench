/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_tunnel.h"
#include "wb_context.h"

#include "base/string_utilities.h"
#include "base/file_utilities.h"

#include <errno.h>
#ifndef _MSC_VER
  #include <unistd.h>
  #include <signal.h>
  #include <sys/types.h>
  #include <sys/wait.h>
#endif

#include <sstream>

#include "base/log.h"
#include "SSHSession.h"
#include "SSHSessionWrapper.h"

DEFAULT_LOG_DOMAIN("SSH tunnel")

using namespace wb;
using namespace base;

//----------------------------------------------------------------------------------------------------------------------

TunnelManager::TunnelManager()
    : _manager(nullptr) {
}

//----------------------------------------------------------------------------------------------------------------------

void TunnelManager::start() {
  if (_manager == nullptr)
    _manager = new ssh::SSHTunnelManager();

  if (!_manager->isRunning()) {
    logInfo("Starting tunnel\n");
    _manager->start();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TunnelManager::shutdown() {
  if (_manager != nullptr) {
    _manager->setStop();
    _manager->pokeWakeupSocket();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TunnelManager::portUsageIncrement(const ssh::SSHConnectionConfig &config) {
  logDebug2("Increment port usage count: %d\n", config.localport);
  base::MutexLock lock(_usageMapMtx);
  auto it = _portUsage.find(config.localport);
  if (it != _portUsage.end()) {
    g_atomic_int_inc(&it->second.second);
  } else {
    _portUsage.insert( { config.localport, { config, base::refcount_t(1) } });
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TunnelManager::portUsageDecrement(const ssh::SSHConnectionConfig &config) {
  logDebug2("Decrement port usage count: %d\n", config.localport);
  base::MutexLock lock(_usageMapMtx);
  auto it = _portUsage.find(config.localport);
  if (it != _portUsage.end()) {
    if (g_atomic_int_dec_and_test(&it->second.second)) {
      if (_manager != nullptr)
        _manager->disconnect(config);
      _portUsage.erase(it);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::shared_ptr<SSHTunnel> TunnelManager::createTunnel(db_mgmt_ConnectionRef connectionProperties) {
  grt::DictRef parameter_values = connectionProperties->parameterValues();

  if (connectionProperties->driver()->name() == "MysqlNativeSSH") {
    start();

    auto connection = ssh::SSHSessionWrapper::getConnectionInfo(connectionProperties);
    ssh::SSHConnectionConfig config = std::get<0>(connection);
    ssh::SSHConnectionCredentials credentials = std::get<1>(connection);

    // before anything, check if a tunnel already exists for this server/user/target tuple
    bec::GRTManager::get()->replace_status_text("Looking for existing SSH tunnel to " + config.getServer() + "...");

    int tunnel_port = _manager->lookupTunnel(config);
    if (tunnel_port > 0) {
      bec::GRTManager::get()->replace_status_text("Existing SSH tunnel found, connecting...");
      logInfo("Existing SSH tunnel found, connecting\n");
      config.localport = tunnel_port;
      return std::shared_ptr<SSHTunnel>(new ::SSHTunnel(this, config));
    } else {
      bool resetPassword = false;

      bec::GRTManager::get()->replace_status_text("Existing SSH tunnel not found, opening new one...");
      logInfo("Existing SSH tunnel not found, opening new one\n");

      auto session = ssh::SSHSession::createSession();
      while (true) {
        std::string service = ssh::SSHSessionWrapper::fillupAuthInfo(config, credentials, resetPassword);

        bec::GRTManager::get()->replace_status_text("Opening SSH tunnel to " + config.getServer() + "...");
        logInfo("Opening SSH tunnel to %s\n", config.getServer().c_str());

        auto retVal = session->connect(config, credentials);
        switch (std::get<0>(retVal)) {
          case ssh::SSHReturnType::CONNECTION_FAILURE: {
            std::string errorMsg = std::get<1>(retVal);
            logError("Unable to open SSH tunnel: %s\n", errorMsg.c_str());
            bec::GRTManager::get()->replace_status_text("Could not open SSH tunnel");
            throw std::runtime_error(std::string("Cannot open SSH Tunnel: ").append(errorMsg.c_str()));
          }
          case ssh::SSHReturnType::CONNECTED: {
            retVal = _manager->createTunnel(session);
            uint16_t port = std::get<1>(retVal);
            bec::GRTManager::get()->replace_status_text("SSH tunnel opened");
            logInfo("SSH tunnel opened on port: %d\n", (int )port);
            config.localport = port;
            return std::shared_ptr<SSHTunnel>(new ::SSHTunnel(this, config));
          }
          case ssh::SSHReturnType::INVALID_AUTH_DATA: {
            std::string errorMsg = std::get<1>(retVal);
            logError("Authentication error opening SSH tunnel: %s\n", errorMsg.c_str());
            bec::GRTManager::get()->replace_status_text("Authentication error opening SSH tunnel");
            if (mforms::Utilities::show_error("Could not connect the SSH Tunnel", errorMsg, _("Retry"), _("Cancel"))
                == mforms::ResultOk) {
              resetPassword = true;
              try {
                mforms::Utilities::forget_password(service, credentials.username);
              } catch (std::exception &exc) {
                logWarning("Could not clear password: %s\n", exc.what());
              }
              credentials.password = "";
              session->disconnect();
            } else
              throw grt::user_cancelled("Tunnel connection cancelled");
            break;
          }
          case ssh::SSHReturnType::FINGERPRINT_CHANGED:
          case ssh::SSHReturnType::FINGERPRINT_MISMATCH: {
            std::string fingerprint = std::get<1>(retVal);
            std::string errorMsg =
              "WARNING: Server public key has changed. It means either you're under attack or the administrator has "
              "changed the key. New public fingerprint is: " + fingerprint;
            mforms::Utilities::show_error("Could not connect the SSH Tunnel", errorMsg, _("Ok"));
            logDebug("Tunnel auth error, key fingerprint mismatch\n");
            throw grt::user_cancelled("");
          }
          case ssh::SSHReturnType::FINGERPRINT_UNKNOWN:  //The server is unknown. The public key fingerprint is:"
          case ssh::SSHReturnType::FINGERPRINT_UNKNOWN_AUTH_FILE_MISSING: {
            std::string fingerprint = std::get<1>(retVal);
            std::string msg = "The authenticity of host '" + config.remoteSSHhost
                + "' can't be established.\n Server key fingerprint is " + fingerprint
                + "\nAre you sure you want to continue connecting?";

            if (mforms::Utilities::show_error("Could not connect the SSH Tunnel", msg, _("Ok"), _("Cancel"))
                != mforms::ResultOk) {
              throw grt::user_cancelled("Tunnel connection cancelled");
            }
            config.fingerprint = fingerprint;
            session->disconnect();
            break;
          }
        }
      }
    }
  }
  return std::shared_ptr<SSHTunnel>();
}

//----------------------------------------------------------------------------------------------------------------------

TunnelManager::~TunnelManager() {
  shutdown();
  if (_manager != nullptr) {
    if (_manager->isRunning())
      _manager->join();

    delete _manager;
  }
}

//----------------------------------------------------------------------------------------------------------------------
