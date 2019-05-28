/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "SSHSessionWrapper.h"
#include "SSHFileWrapper.h"
#include "base/log.h"
#include <fcntl.h>
#include <vector>
#include "mforms/utilities.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_context.h"
#include "grt/grt_manager.h"
#include "base/threaded_timer.h"

DEFAULT_LOG_DOMAIN("SSHSessionWrapper")

namespace ssh {

  SSHSessionWrapper::SSHSessionWrapper(const SSHConnectionConfig &config, const SSHConnectionCredentials &credentials)
  : _session(SSHSession::createSession()), _config(config), _credentials(credentials), _sessionPoolHandle(0), _isClosing(false), _canClose(0) {
  }

  SSHSessionWrapper::SSHSessionWrapper(const db_mgmt_ConnectionRef connectionProperties)
      : _session(SSHSession::createSession()), _sessionPoolHandle(0), _isClosing(false), _canClose(0) {
    grt::DictRef parameter_values = connectionProperties->parameterValues();
    if (connectionProperties->driver()->name() == "MysqlNativeSSH") {
      auto connection = getConnectionInfo(connectionProperties);
      _config = std::get<0>(connection);
      _credentials = std::get<1>(connection);
    } else
      throw std::runtime_error("Invalid connection data, expected SSH Connection, got standard\n");
  }

  SSHSessionWrapper::SSHSessionWrapper(const db_mgmt_ServerInstanceRef serverInstanceProperties)
      : _session(SSHSession::createSession()), _sftp(nullptr), _sessionPoolHandle(0), _isClosing(false), _canClose(0) {

    if (serverInstanceProperties->serverInfo().get_int("remoteAdmin", 0) == 1 && !serverInstanceProperties->loginInfo().get_string("ssh.hostName").empty())
    {
      auto connection = getConnectionInfo(serverInstanceProperties);
      _config = std::get<0>(connection);
      _credentials = std::get<1>(connection);
    }
    else
      throw std::runtime_error("Invalid connection data, expected SSH Connection, got standard\n");
  }

  SSHSessionWrapper::~SSHSessionWrapper() {
    logDebug2("destroyed\n");
    _isClosing = true;
    disconnect();
  }

  void SSHSessionWrapper::disconnect() {
    if (_sessionPoolHandle != 0) {
      if (!ThreadedTimer::remove_task(_sessionPoolHandle)) {
        _canClose.wait();
      }
      _sessionPoolHandle = 0;
    }

    auto timeoutLock = lockTimeout();
    // before we continue, we should close all opened files
    _sftp.reset();
    _session->disconnect();
  }

  grt::IntegerRef SSHSessionWrapper::isConnected() {
    return _session->isConnected() ? 1 : 0;
  }

  grt::IntegerRef SSHSessionWrapper::connect() {
    bool resetPassword = false;
    while (true) {
      std::string service = fillupAuthInfo(_config, _credentials, resetPassword);

      logInfo("Opening new SSH connection to %s\n", _config.getServer().c_str());

      auto retVal = _session->connect(_config, _credentials);
      switch (std::get<0>(retVal)) {
        case ssh::SSHReturnType::CONNECTION_FAILURE: {
          std::string errorMsg = std::get<1>(retVal);
          logError("Unable to connect to SSH: %s\n", errorMsg.c_str());
          return -1;
        }
        case ssh::SSHReturnType::CONNECTED: {
          logInfo("Succesfully made SSH connection\n");
          makeSessionPoll();
          _sftp = std::shared_ptr<ssh::SSHSftp>(new ssh::SSHSftp(_session, wb::WBContextUI::get()->get_wb()->get_wb_options().get_int("SSH:maxFileSize", 65535)));
          return 0;
        }
        case ssh::SSHReturnType::INVALID_AUTH_DATA: {
          std::string errorMsg = std::get<1>(retVal);
          logError("Authentication error opening SSH connection: %s\n", errorMsg.c_str());
          if (mforms::Utilities::show_error("Unable to establish SSH connection", errorMsg, _("Retry"), _("Cancel"))
              == mforms::ResultOk) {
            resetPassword = true;
            try {
              mforms::Utilities::forget_password(service, _credentials.username);
            } catch (std::exception &exc) {
              logWarning("Could not clear password: %s\n", exc.what());
            }
            _credentials.password = "";
            _session->disconnect();
          } else
            return -2;
          break;
        }
        case ssh::SSHReturnType::FINGERPRINT_CHANGED:
        case ssh::SSHReturnType::FINGERPRINT_MISMATCH: {
          std::string fingerprint = std::get<1>(retVal);
          std::string errorMsg =
              "WARNING: Server public key has changed. It means either you're under attack or the administrator has changed the key. New public fingerprint is: "
                  + fingerprint;
          mforms::Utilities::show_error("Could not connect the SSH server", errorMsg, _("Ok"));
          logDebug("Tunnel auth error, key fingerprint mismatch\n");
          throw grt::user_cancelled("");
        }
        case ssh::SSHReturnType::FINGERPRINT_UNKNOWN:  //The server is unknown. The public key fingerprint is:"
        case ssh::SSHReturnType::FINGERPRINT_UNKNOWN_AUTH_FILE_MISSING: {
          std::string fingerprint = std::get<1>(retVal);
          std::string msg = "The authenticity of host '" + _config.remoteSSHhost
              + "' can't be established.\n Server key fingerprint is " + fingerprint
              + "\nAre you sure you want to continue connecting?";

          if (mforms::Utilities::show_error("Unable to establish SSH connection", msg, _("Ok"), _("Cancel"))
              != mforms::ResultOk) {
            return -3;
          }
          _config.fingerprint = fingerprint;
          _session->disconnect();
          break;
        }
      }
    }
  }

  grt::DictRef SSHSessionWrapper::executeCommand(const std::string &command) {
    if (!_session->isConnected())
      return "";

    auto ret = _session->execCmd(command,
                             wb::WBContextUI::get()->get_wb()->get_wb_options().get_int("SSH:logSize", LOG_SIZE_100MB));
    grt::DictRef dict(true);
    dict.gset("stdout", std::get<0>(ret));
    dict.gset("stderr", std::get<1>(ret));
    dict.gset("status", std::get<2>(ret));
    return dict;
  }

  grt::DictRef SSHSessionWrapper::executeSudoCommand(const std::string &command, const std::string &user) {
    if (!_session->isConnected())
      return "";

    std::string sudoUser = user;
    if (sudoUser.empty()) {
      logWarning("Sudo user not specified, using connection user.\n");
      sudoUser = _credentials.username;
    }

    bool resetPw = false;
    while (true) {
      std::string password;
      if (mforms::Utilities::find_or_ask_for_password(_("Execute privileged command"), "sudo@localhost",
                                                      sudoUser, resetPw, password)) {
        try {
          auto ret = _session->execCmdSudo(
              command, password, "EnterPasswordHere",
              wb::WBContextUI::get()->get_wb()->get_wb_options().get_int("SSH:logSize", LOG_SIZE_100MB));
          grt::DictRef dict(true);
          dict.gset("stdout", std::get<0>(ret));
          dict.gset("stderr", std::get<1>(ret));
          dict.gset("status", std::get<2>(ret));
          return dict;
        } catch (ssh::SSHAuthException &) {
          logError("Invalid user password, retry\n");
          resetPw = true;
          continue;
        }
      } else {
        logDebug2("User cancel password dialog");
        grt::DictRef dict(true);
        dict.gset("stdout", "");
        dict.gset("stderr", "");
        dict.gset("status", -1);
        return dict;
      }
    }
  }

  std::tuple<ssh::SSHConnectionConfig, ssh::SSHConnectionCredentials> SSHSessionWrapper::getConnectionInfo(
      db_mgmt_ConnectionRef connectionProperties) {
    grt::DictRef parameter_values = connectionProperties->parameterValues();
    ssh::SSHConnectionConfig config;
    config.localhost = "127.0.0.1";
    config.bufferSize = bec::GRTManager::get()->get_app_option_int("SSH:BufferSize", 10240);
    config.connectTimeout = bec::GRTManager::get()->get_app_option_int("SSH:connectTimeout", 10);
    config.readWriteTimeout = bec::GRTManager::get()->get_app_option_int("SSH:readWriteTimeout", 5);
    config.commandTimeout = bec::GRTManager::get()->get_app_option_int("SSH:commandTimeout", 1);
    config.commandRetryCount = bec::GRTManager::get()->get_app_option_int("SSH:commandRetryCount", 3);
    config.configFile = bec::GRTManager::get()->get_app_option_string("SSH:pathtosshconfig");
    config.knownHostsFile = bec::GRTManager::get()->get_app_option_string("SSH:knownhostsfile");
    config.compressionLevel = static_cast<int>(connectionProperties->parameterValues().get_int("sshCompressionLevel", 0));

    auto parts = base::split(parameter_values.get_string("sshHost"), ":");
    config.remoteSSHhost = parts[0];
    if (parts.size() > 1)
      config.remoteSSHport = base::atoi<std::size_t>(parts[1], 22);
    else
      config.remoteSSHport = 22;

    config.remotehost = parameter_values.get_string("hostName");
    config.remoteport = static_cast<int>(parameter_values.get_int("port", 3306));

    ssh::SSHConnectionCredentials credentials;
    credentials.username = parameter_values.get_string("sshUserName");
    credentials.password = parameter_values.get_string("sshPassword");
    credentials.keyfile = base::expand_tilde(parameter_values.get_string("sshKeyFile"));
    if (credentials.keyfile.empty())
      credentials.auth = ssh::SSHAuthtype::PASSWORD;
    else
      credentials.auth = ssh::SSHAuthtype::KEYFILE;
    return std::make_tuple(config, credentials);
  }
//
  std::tuple<ssh::SSHConnectionConfig, ssh::SSHConnectionCredentials> SSHSessionWrapper::getConnectionInfo(
              db_mgmt_ServerInstanceRef serverInstanceProperties) {
    ssh::SSHConnectionConfig config;
    config.localhost = "127.0.0.1";
    config.bufferSize = bec::GRTManager::get()->get_app_option_int("SSH:BufferSize", 10240);
    config.connectTimeout = bec::GRTManager::get()->get_app_option_int("SSH:connectTimeout", 10);
    config.readWriteTimeout = bec::GRTManager::get()->get_app_option_int("SSH:readWriteTimeout", 5);
    config.commandTimeout = bec::GRTManager::get()->get_app_option_int("SSH:commandTimeout", 1);
    config.commandRetryCount = bec::GRTManager::get()->get_app_option_int("SSH:commandRetryCount", 3);
    config.configFile = bec::GRTManager::get()->get_app_option_string("SSH:pathtosshconfig");
    config.knownHostsFile = bec::GRTManager::get()->get_app_option_string("SSH:knownhostsfile");

    auto loginInfo = serverInstanceProperties->loginInfo();

    auto parts = base::split(loginInfo.get_string("ssh.hostName"), ":");
    config.remoteSSHhost = parts[0];
    if (parts.size() > 1) {
      config.remoteSSHport = base::atoi<std::size_t>(parts[1], 22);
    } else {
      if (loginInfo.has_key("ssh.port")) {
        config.remoteSSHport =  base::atoi<std::size_t>(loginInfo.get_string("ssh.port"), 22);
      } else {
        config.remoteSSHport = 22;
      }
    }

    auto parameter_values = serverInstanceProperties->connection()->parameterValues();
    config.remotehost = parameter_values.get_string("hostName");
    config.remoteport = static_cast<int>(parameter_values.get_int("port", 3306));

    ssh::SSHConnectionCredentials credentials;
    credentials.username = loginInfo.get_string("ssh.userName");
    if (loginInfo.get_int("ssh.useKey", 0) == 1)
      credentials.keyfile = base::expand_tilde(loginInfo.get_string("ssh.key"));

    if (credentials.keyfile.empty())
      credentials.auth = ssh::SSHAuthtype::PASSWORD;
    else
      credentials.auth = ssh::SSHAuthtype::KEYFILE;

    return std::make_tuple(config, credentials);
  }

  std::string SSHSessionWrapper::fillupAuthInfo(ssh::SSHConnectionConfig &config,
                                                ssh::SSHConnectionCredentials &credentials, bool resetPassword) {

    std::string service;
    if (credentials.keyfile.empty() && credentials.password.empty()) {
      // interactively ask user for password
      service = base::strfmt("ssh@%s", config.getServer().c_str());

      bool result = false;
      try {
        result = mforms::Utilities::credentials_for_service(_("Open SSH Connection"), service, credentials.username,
                                                            resetPassword, credentials.password);
      } catch (std::exception &exc) {
        logWarning("Exception caught on credentials_for_service: %s", exc.what());
        mforms::Utilities::show_error("Clear Password", base::strfmt("Could not clear password: %s", exc.what()), "OK");
      }

      if (!result)
        // we need to throw an exception to signal that tunnel could not be opened (and not that it was not needed)
        throw grt::user_cancelled("SSH password input cancelled by user");
    }

    if (!credentials.keyfile.empty()) {
      bool encrypted = base::contains_string(base::getTextFileContent(credentials.keyfile), "ENCRYPTED", true);

      // interactively ask user for SSH key passphrase
      service = base::strfmt("ssh_keyfile@%s", credentials.keyfile.c_str());
      if (encrypted
          && !mforms::Utilities::find_or_ask_for_password(_("Open SSH Connection"), service, credentials.username,
                                                          resetPassword, credentials.keypassword))
        // we need to throw an exception to signal that tunnel could not be opened (and not that it was not needed)
        throw std::runtime_error("SSH key passphrase input cancelled by user");
    }
    return service;
  }



  grt::IntegerRef SSHSessionWrapper::cd(const std::string &directory) {
    if (_sftp)
      return _sftp->cd(directory);
    throw std::runtime_error("Not connected");

  }

  void SSHSessionWrapper::get(const std::string &src, const std::string &dest) {
    if (_sftp)
      _sftp->get(src, dest);
    else
      throw std::runtime_error("Not connected");
  }

  grt::StringRef SSHSessionWrapper::getContent(const std::string &src) {
    if (_sftp)
      return _sftp->getContent(src);
    throw std::runtime_error("Not connected");
  }

  static grt::DictRef sftpAttribToDict(const SftpStatAttrib &attrib) {
    grt::DictRef ret(true);
    ret.gset("size", static_cast<long int>(attrib.size));
    ret.gset("uid", static_cast<int>(attrib.uid));
    ret.gset("gid", static_cast<int>(attrib.gid));
    ret.gset("gid", static_cast<int>(attrib.gid));
    ret.gset("atime", static_cast<long int>(attrib.atime));
    ret.gset("mtime", static_cast<long int>(attrib.mtime));
    ret.gset("name",  attrib.name);
    ret.gset("isDir",  attrib.isDir);
    return ret;
  }

  grt::DictListRef SSHSessionWrapper::ls(const std::string &path) {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    auto ftpList = _sftp->ls(path);
    if (!ftpList.empty())
    {
      grt::DictListRef contents(grt::Initialized);
      for (const auto &it: ftpList)
        contents.insert(sftpAttribToDict(it));
      return contents;
    }
    else
      return grt::DictListRef();
  }

  void SSHSessionWrapper::mkdir(const std::string &directory) {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    _sftp->mkdir(directory);
  }

  db_mgmt_SSHFileRef SSHSessionWrapper::open(const std::string &path) {
    auto lock = _session->lockSession();
    logDebug2("About to open file: %s\n", path.c_str());
    db_mgmt_SSHFileRef object(grt::Initialized);
    object->owner(wb::WBContextUI::get()->get_wb()->get_root());
    object->name(path);
    object->set_data(new ssh::SSHFileWrapper(_session, _sftp, path, wb::WBContextUI::get()->get_wb()->get_wb_options().get_int("SSH:maxFileSize", 65535)));
    return object;
  }

  void SSHSessionWrapper::put(const std::string &src, const std::string &dest) {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    _sftp->put(src, dest);
  }

  grt::StringRef SSHSessionWrapper::pwd() {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    return _sftp->pwd();
  }

  void SSHSessionWrapper::rmdir(const std::string &directory) {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    _sftp->rmdir(directory);
  }

  void SSHSessionWrapper::setContent(const std::string &path, const std::string &content) {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    _sftp->setContent(path, content);
  }

  grt::DictRef SSHSessionWrapper::stat(const std::string &path) {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    return sftpAttribToDict(_sftp->stat(path));
  }

  void SSHSessionWrapper::unlink(const std::string &file) {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    _sftp->unlink(file);
  }

  grt::IntegerRef SSHSessionWrapper::fileExists(const std::string &path) {
    if (!_sftp)
      throw std::runtime_error("Not connected");

    return _sftp->fileExists(path) ? 1 : 0;
  }

  base::RecMutexLock SSHSessionWrapper::lockTimeout() {
    base::RecMutexLock mutexLock(_timeoutMutex);
    return mutexLock;
  }

  void SSHSessionWrapper::makeSessionPoll() {
    auto timeoutLock = lockTimeout();
    if (_sessionPoolHandle != 0) {
      ThreadedTimer::remove_task(_sessionPoolHandle);
      _sessionPoolHandle = 0;
    }

    _sessionPoolHandle = ThreadedTimer::add_task(TimerTimeSpan, 2.0,
                                                 false, std::bind(&SSHSessionWrapper::pollSession, this));
  }

  bool SSHSessionWrapper::pollSession() {
    auto timeoutLock = lockTimeout();
    if (_session != nullptr)
      _session->pollEvent();

    if (_isClosing) {
      _canClose.post();
      return true;
    }

    return false;
  }

} /* namespace ssh */
