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


#include "SSHSession.h"
#include "base/log.h"
#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include <fcntl.h>
#include <vector>
#include <functional>
#include <sstream>
#ifdef _MSC_VER
#include "Shlobj.h"
#endif // _MSC_VER


DEFAULT_LOG_DOMAIN("SSHSession")

namespace ssh {
  std::shared_ptr<SSHSession> SSHSession::createSession() {
    return std::shared_ptr<SSHSession>(new SSHSession());
  }

  SSHSession::SSHSession()
      : _session(new ssh::Session()), _isConnected(false), _event(nullptr) {
    initLibSSH();
  }

  SSHSession::~SSHSession() {
    if (_event != nullptr)
      ssh_event_free(_event);
    delete _session;
  }

  std::tuple<SSHReturnType, base::any> SSHSession::connect(const SSHConnectionConfig &config,
                                                           const SSHConnectionCredentials &credentials) {
    if (isConnected()) {
      throw std::runtime_error("Unable to connect already connected SSHSession, please disconnect first.");
    }

    auto lock = lockSession();
    _config = config;
    _credentials = credentials;

    // The option setter does't indicate what option failed to set, so we have to wrap each call
    // individually to be able to report exactly what failed.
    try {
      _session->setOption(SSH_OPTIONS_USER, credentials.username.c_str());
    } catch (SshException &exc) {
      logError("Error setting user name in ssh session option: %s\n", exc.getError().c_str());
      return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
    }

    try {
      _session->setOption(SSH_OPTIONS_HOST, config.remoteSSHhost.c_str());
    } catch (SshException &exc) {
      logError("Error setting remote host in ssh session option: %s\n", exc.getError().c_str());
      return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
    }

    try {
      _session->setOption(SSH_OPTIONS_PORT, static_cast<long>(config.remoteSSHport));
    } catch (SshException &exc) {
      logError("Error setting remote port in ssh session option: %s\n", exc.getError().c_str());
      return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
    }

    try {
      _session->setOption(SSH_OPTIONS_TIMEOUT, static_cast<long>(config.connectTimeout));
    } catch (SshException &exc) {
      logError("Error setting connection timeout in ssh session option: %s\n", exc.getError().c_str());
      return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
    }

    try {
      _session->setOption(SSH_OPTIONS_STRICTHOSTKEYCHECK, config.strictHostKeyCheck ? 1 : 0);
    } catch (SshException &exc) {
      logError("Error setting strict host key check value in ssh session option: %s\n", exc.getError().c_str());
      return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
    }

    if (config.compressionLevel > 0) {
      try {
        _session->setOption(SSH_OPTIONS_COMPRESSION, "yes");
      } catch (SshException &exc) {
        logError("Error setting compression flag in ssh session option: %s\n", exc.getError().c_str());
        return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
      }

      try {
        _session->setOption(SSH_OPTIONS_COMPRESSION_LEVEL, config.compressionLevel);
      } catch (SshException &exc) {
        logError("Error setting compression level in ssh session option: %s\n", exc.getError().c_str());
        return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
      }
    }

    _config.dumpConfig();

    if (!config.knownHostsFile.empty()) {
      try {
        _session->setOption(SSH_OPTIONS_KNOWNHOSTS, config.knownHostsFile.c_str());
      } catch (SshException &exc) {
        logError("Error setting known hosts in ssh session option: %s\n", exc.getError().c_str());
        return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
      }
    }

    if (!config.optionsDir.empty()) {
      try {
        _session->setOption(SSH_OPTIONS_SSH_DIR, config.optionsDir.c_str());
      } catch (SshException &exc) {
        logError("Error setting options folder in ssh session option: %s\n", exc.getError().c_str());
        return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, exc.getError());
      }
    }

    try {
      if (!config.configFile.empty())
        _session->optionsParseConfig(config.configFile.c_str());
    } catch (ssh::SshException &exc) {
      logError("Unable to parse config file: %s\nError was: %s\n", config.configFile.c_str(), exc.getError().c_str());
    }

    try {
      _session->connect();
    } catch (ssh::SshException &exc) {
      logError("Unable to connect: %s:%lu\nError was: %s\n", config.remoteSSHhost.c_str(), config.remoteSSHport,
               exc.getError().c_str());
      return std::make_tuple(SSHReturnType::CONNECTION_FAILURE, exc.getError());
    }

    std::string fingerprint;
    int retVal = verifyKnownHost(config, fingerprint);
    switch (retVal) {
      case SSH_SERVER_FILE_NOT_FOUND:
      case SSH_SERVER_NOT_KNOWN:
        return std::make_tuple(
            retVal == SSH_SERVER_FILE_NOT_FOUND ?
                SSHReturnType::FINGERPRINT_UNKNOWN_AUTH_FILE_MISSING : SSHReturnType::FINGERPRINT_UNKNOWN,
            fingerprint);
      case SSH_SERVER_KNOWN_CHANGED:
        return std::make_tuple(SSHReturnType::FINGERPRINT_CHANGED, fingerprint);
      case SSH_SERVER_FOUND_OTHER:
        return std::make_tuple(SSHReturnType::FINGERPRINT_MISMATCH, fingerprint);
      default:
        break;
    }

    try {
      authenticateUser(credentials);
    } catch (SSHTunnelException &sxc) {
      logError("User authentication failed.\n");
      return std::make_tuple(SSHReturnType::INVALID_AUTH_DATA, std::string(sxc.what()));
    }

    _isConnected = true;

    return std::make_tuple(SSHReturnType::CONNECTED, nullptr);
  }


  void SSHSession::pollEvent() {
    if (!_isConnected)
      return;

    if (!_sessionMutex.tryLock()) {
      logDebug2("Can't poll, session busy.\n");
      return;
    }

    if (_event == nullptr) {
      _event = ssh_event_new();
      ssh_event_add_session(_event, _session->getCSession());
    }

    logDebug2("Session pool event\n");
    ssh_event_dopoll(_event, 0);
    _sessionMutex.unlock();
  }

  void SSHSession::disconnect() {
    logDebug2("SSHSession disconnect\n");

    bool locked = _sessionMutex.tryLock();
    int i = 0;
    while (!locked && i < 5) { // Wait for 5 sec.
      ++i;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      locked = _sessionMutex.tryLock();
    }

    if (!locked) {
      logError("We're about to disconnect but can't obtain session lock, this may result in undefined behavior.");
    }

    if (_session != nullptr) {
      if (_event != nullptr) {
        logDebug2("Remove session event\n");
        ssh_event_free(_event);
        _event = nullptr;
      }

      if (_isConnected)
        _session->disconnect();
      delete _session;
      _session = new ssh::Session();
    }
    // Now we should release the lock.
    _isConnected = false;
    _sessionMutex.unlock();
  }

  bool SSHSession::isConnected() const {
    return _isConnected && ssh_is_connected(_session->getCSession());
  }

  SSHConnectionConfig SSHSession::getConfig() const {
    return _config;
  }

  ssh::Session* SSHSession::getSession() const {
    return _session;
  }


  bool SSHSession::openChannel(ssh::Channel *chann) {
    int rc = SSH_ERROR;
    std::size_t i = 0;
    while (i < _config.connectTimeout) {
      rc = ssh_channel_open_session(chann->getCChannel());
      // We can't rely on rc == 0 as there's possibility it will return 0 even that the channel is closed.
      // Because of this, we have to use isOpen() and try few times
      if (rc == SSH_AGAIN || !chann->isOpen()) {
        logDebug3("Unable to open channel, wait a moment and retry.\n");
        i++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      } else if (rc == SSH_ERROR) {
        logError("Unable to open channel: %s \n", ssh_get_error(chann->getCSession()));
        return false;
      } else {
        logDebug("Channel successfully opened\n");
        return true;
      }
    }
    return false;
  }

  /**
   * Execute command on the remote server.
   * It returns std::tuple<stdout, stderr, exitStatus>.
   */
  std::tuple<std::string, std::string, int> SSHSession::execCmd(std::string command, std::size_t logSize) {
    logDebug2("About to execute command: %s\n", command.c_str());

    logDebug3("Before session lock.\n");
    auto lock = lockSession();
    logDebug3("Session locked.\n");
    auto channel = std::unique_ptr<ssh::Channel, std::function<void(ssh::Channel *)>>(
        new ssh::Channel(*_session), [](ssh::Channel *chan) { chan->close(); delete chan; });
    if (!openChannel(channel.get())) {
      throw SSHTunnelException("Unable to open channel");
    }

    logDebug3("Before request exec.\n");
    channel->requestExec(command.c_str());
    logDebug3("Command executed.\n");
    ssize_t readLen = 0, readErrLen = 0;
    std::size_t retryCount = 0;
    std::vector<char> buff(_config.bufferSize, '\0');
    std::string retError;
    std::ostringstream so;
    std::size_t bytesRead = 0;
    do {
      try {
        readLen = channel->read(buff.data(), buff.size(), false, static_cast<int>(1000 * _config.readWriteTimeout));
        if (readLen == SSH_AGAIN && retryCount < _config.commandRetryCount) {
          retryCount++;
          logDebug2("Got SSH_AGAIN, retrying\n");
          continue;
        }

        if (!channel->isEof() && readLen == 0 && retryCount < _config.commandRetryCount) {
          logDebug2("Retry reading command output\n");
          if (channel->isClosed())
            logDebug2("Retry reading command output: closed channel :(\n");
          retryCount++;
          std::this_thread::sleep_for(std::chrono::seconds(_config.commandTimeout));
          continue;
        }
      } catch (SshException &exc) {
        if (channel->isClosed())
          return std::make_tuple(so.str(), retError, channel->getExitStatus());

        throw SSHTunnelException(exc.getError());
      }

      if (readLen > 0) {
        bytesRead += readLen;
        std::string data(buff.data(), readLen);
        logDebug3("Read SSH data: %s\n", data.c_str());
        so << data;
      }

      try {
        // Let's see if maybe there are some errors
        readErrLen = channel->read(buff.data(), buff.size(), true, static_cast<int>(1000 * _config.readWriteTimeout));
        if (readErrLen > 0) {
          std::string errorMsg(buff.data(), readErrLen);
          logError("Got error: %s\n", errorMsg.c_str());
          retError.append(errorMsg);
          if (channel->isEof()) {
            logDebug3("Got EOF.\n");
            channel->close();
            break;
          }
        }
      } catch (SshException &exc) {
        if (channel->isClosed())
          return std::make_tuple(so.str(), retError, channel->getExitStatus());

        throw SSHTunnelException(exc.getError());
      }

      if (readLen == SSH_EOF || readErrLen == SSH_EOF || channel->isEof()) {
        channel->close();
        logDebug3("Got EOF.\n");
        break;
      }

      if (readLen == SSH_ERROR || readErrLen == SSH_ERROR) {
        logDebug3("Client disconnected.\n");
        throw SSHTunnelException("client disconnected");
      }

      if (bytesRead > logSize) {
        throw SSHTunnelException("Too much data to read, limit is: " + std::to_string(logSize) + ". You can change the limit in the Workbench Preferences.");
      }
    } while (true);

    logDebug3("Bytes read: %lu\n", bytesRead);
    return std::make_tuple(so.str(), retError, channel->getExitStatus());
  }

  /**
   * Execute command on the remote server using sudo authentication.
   * It returns std::tuple <stdout, stderr, exitStatus>.
   */
  std::tuple<std::string, std::string, int> SSHSession::execCmdSudo(std::string command, std::string password,
                                                               std::string passwordQuery, std::size_t logSize) {
    logDebug2("About to execute elevated command: %s\n", command.c_str());
    auto lock = lockSession();
    auto channel = std::unique_ptr<ssh::Channel, std::function<void(ssh::Channel *)>>(
            new ssh::Channel(*_session), [](ssh::Channel *chan) { chan->close(); delete chan; });

    if (!openChannel(channel.get())) {
      throw SSHTunnelException("Unable to open channel");
    }

    channel->requestExec(command.c_str());
    logDebug3("Command executed.\n");
    ssize_t readlen = 0, readErrLen = 0;
    std::size_t writelen = 0;
    std::size_t retryCount = 0;
    std::vector<char> buff(_config.bufferSize, '\0');
    std::string retError;
    std::ostringstream so;
    std::size_t bytesRead = 0;
    bool pwSkip = false;

    do {
      if (!pwSkip) {
        try {
          readlen = channel->read(buff.data(), buff.size(), true, static_cast<int>(1000 * _config.readWriteTimeout));
          std::string test(buff.data(), readlen);
          logDebug2("Got password prompt: %s\n", test.c_str());
          if (strncmp(buff.data(), passwordQuery.c_str(), readlen) == 0) {
            writelen = channel->write(password.data(), password.size());
            if (writelen != password.size()) {
              throw SSHTunnelException("Problem while sending pw.\n");
            }
            if (password.back() != '\n') {  // Password need to be confimed by new line, check and append it
              writelen = channel->write("\n", 1);
              if (writelen != 1) {
                throw SSHTunnelException("Unable to confirm password.\n");
              }
            }
            pwSkip = true;
          } else {
            logDebug2("Expected pw prompt but it didn't came, instead we got: %s\n", test.c_str());
            retError.append(test);
          }
        } catch (SshException &) {
          logError("Sudo password didn't came, could be it was cached, we continue and see what will happen\n");
          pwSkip = true;
        }
      }

      try {
        // Let's see if maybe it was wrong sudo credentials
        readErrLen = channel->read(buff.data(), buff.size(), true, static_cast<int>(1000 * _config.readWriteTimeout));
        std::string errorMessage(buff.data(), readErrLen);
        if (readErrLen > 0) {
          logError("Got error: %s\n", errorMessage.c_str());
          if (errorMessage.find("Sorry, try again.") != std::string::npos) {
            logError("Incorrect sudo password.\n");
            throw SSHAuthException("Incorrect sudo password");
          } else if (errorMessage.find("This incident will be reported") != std::string::npos) {
            logError("User not in sudoers files.\n");
            throw SSHAuthException("User not in sudoers");
          } else {  // This means that there's another error, pass it as a return value.
            logDebug2("Got output on stderr: %s\n", errorMessage.c_str());
            retError.append(errorMessage);
          }
        }
      } catch (SshException &exc) {
        if (channel->isClosed())
          return std::make_tuple(so.str(), retError, channel->getExitStatus());

        throw SSHTunnelException(exc.getError());
      }

      try {
        readlen = channel->read(buff.data(), buff.size(), false, static_cast<int>(1000 * _config.readWriteTimeout));
        if (!channel->isEof() && readlen == 0 && retryCount < _config.commandRetryCount) {
          logDebug2("Retry reading command output\n");
          retryCount++;
          std::this_thread::sleep_for(std::chrono::seconds(_config.commandTimeout));
          continue;
        }
      } catch (SshException &exc) {
        if (channel->isClosed())
          return std::make_tuple(so.str(), retError, channel->getExitStatus());
        throw SSHTunnelException(exc.getError());
      }

      if (readlen > 0) {
        bytesRead += readlen;
        std::string data(buff.data(), readlen);
        logDebug3("Read SSH data: %s\n", data.c_str());
        so << data;
      }

      if (readlen == SSH_ERROR || readlen == SSH_EOF) {
        logDebug2("Client disconnected");
        throw SSHTunnelException("client disconnected");
      }

      if (readlen == 0 || channel->isEof()) {
        logDebug2("Nothing to read.\n");
        channel->close();
        break;
      }

      if (bytesRead > logSize) {
        throw SSHTunnelException("Too much data to read, limit is: " + std::to_string(logSize) + ". You can change the limit in the Workbench Preferences.");
      }
    } while (true);

    logDebug3("Bytes read: %lu\n", bytesRead);
    return std::make_tuple(so.str(), retError, channel->getExitStatus());
  }

  void SSHSession::reconnect() {
    if (!ssh_is_connected(_session->getCSession())) {
      disconnect();
      connect(_config, _credentials);
    }
  }

  base::MutexLock SSHSession::lockSession() {
    base::MutexLock mutexLock(_sessionMutex);
    return mutexLock;
  }

  int SSHSession::verifyKnownHost(const ssh::SSHConnectionConfig &config, std::string &fingerprint) {
    std::unique_ptr<unsigned char, void (*)(unsigned char*)> hash(
        nullptr, [](unsigned char* v) {if (v != nullptr) ssh_clean_pubkey_hash(&v);});
    ssh_key srvPubKey;
    int rc = 0;
    std::size_t hlen = 0;
    errno = 0;
    rc = ssh_get_server_publickey(_session->getCSession(), &srvPubKey);
    if (rc < 0)
      throw SSHTunnelException("Can't get server pubkey " + getError());

    unsigned char* hashPtr = nullptr;
    errno = 0;
    rc = ssh_get_publickey_hash(srvPubKey, SSH_PUBLICKEY_HASH_SHA1, &hashPtr, &hlen);
    ssh_key_free(srvPubKey);
    if (rc < 0)
      throw SSHTunnelException("Can't calculate pubkey hash " + getError());

    hash.reset(hashPtr);

    std::unique_ptr<char, void (*)(char*)> hexa(ssh_get_hexa(hash.get(), hlen), [](char*ptr) {free(ptr);});
    fingerprint = hexa.get();
    int retVal = _session->isServerKnown();
    switch (retVal) {
      case SSH_SERVER_FILE_NOT_FOUND:
      case SSH_SERVER_NOT_KNOWN: {
        if (config.fingerprint.empty())
          return retVal;
        else {
          if (config.fingerprint == hexa.get()) {
#if _MSC_VER
            std::string::size_type pos = 0;
            auto knownHostsFile = _config.knownHostsFile;
            if (knownHostsFile.empty()) {
              std::string userFolder = "";
              PWSTR outFolder = nullptr;
              if (SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &outFolder) == S_OK) {
                userFolder = base::wstring_to_string(outFolder);
                userFolder += "\\.ssh\\known_hosts";
                CoTaskMemFree(outFolder);
              }
              if (base_get_file_size(userFolder.c_str()) == 0) {
                std::ofstream outfile(userFolder);
                outfile.close();
              }
              knownHostsFile = userFolder;
              _config.knownHostsFile = userFolder;
            }
            while ((pos = knownHostsFile.find_first_of("\\", pos)) != std::string::npos)
              knownHostsFile.replace(pos, 1, "/");
            _session->setOption(SSH_OPTIONS_KNOWNHOSTS, knownHostsFile.c_str());
            // writeKnownhost from libssh supports only linux like path e.g c:/temp/hosts
            _session->writeKnownhost();
            _session->setOption(SSH_OPTIONS_KNOWNHOSTS, _config.knownHostsFile.c_str());
#else
            _session->writeKnownhost();
#endif
            return SSH_SERVER_KNOWN_OK;
          } else
            return retVal;
        }
      }
      case SSH_SERVER_KNOWN_OK:
      case SSH_SERVER_KNOWN_CHANGED:
      case SSH_SERVER_FOUND_OTHER:
        return retVal;
      case SSH_SERVER_ERROR:
        throw SSHTunnelException(_session->getError());
    }

    return SSH_SERVER_KNOWN_OK;
  }

  void SSHSession::authenticateUser(const SSHConnectionCredentials &credentials) {
    // We first try to use the none method. If that succeeds we're done. In other cases we will try other options.
    try {
      if (_session->userauthNone() == SSH_AUTH_SUCCESS)
        return;
    } catch (SshException &) {
      throw SSHTunnelException(ssh_get_error(_session->getCSession()));
    }

    logInfo("Banner: %s\n", _session->getIssueBanner().c_str());
    switch (credentials.auth) {
      case SSHAuthtype::PASSWORD:
        authPassword(credentials.password);
        break;
      case SSHAuthtype::AUTOPUBKEY:
        authAutoPubkey();
        break;

      case SSHAuthtype::KEYFILE: {
        ssh_key privKey;
        if (!base::file_exists(credentials.keyfile))
          throw std::runtime_error("The key file does not exist.");

        std::string fileKey = base::getTextFileContent(credentials.keyfile);

        if (ssh_pki_import_privkey_base64(fileKey.c_str(), credentials.keypassword.c_str(), nullptr, nullptr,
          &privKey) == SSH_OK) {
          int authInfo = _session->userauthPublickey(privKey);
          ssh_key_free(privKey);
          handleAuthReturn(authInfo);
        }
        else
          throw SSHTunnelException(ssh_get_error(_session->getCSession()));
        break;
      }
    }
  }

  void SSHSession::authPassword(const std::string &password) {
    int authList = _session->getAuthList();
    if (authList & SSH_AUTH_METHOD_INTERACTIVE) {
      int ret;
      while ((ret = _session->userauthKbdint(nullptr, nullptr)) == SSH_AUTH_INFO) {
        int nPrompts = _session->userauthKbdintGetNPrompts();
        for (int i = 0; i < nPrompts; i++)
          _session->userauthKbdintSetAnswer(i, password.c_str());
      }
    } else if (authList & SSH_AUTH_METHOD_PASSWORD) {
      try {
        handleAuthReturn(_session->userauthPassword(password.c_str()));
      } catch (ssh::SshException &sxc) {
        throw SSHTunnelException("Authentication using password failed: " + sxc.getError());
      }
    } else {
      throw SSHTunnelException("Unknown authentication type");
    }
  }

  void SSHSession::authAutoPubkey() {
    try {
      handleAuthReturn(_session->userauthPublickeyAuto());
    } catch (ssh::SshException &sxc) {
      throw SSHTunnelException("Automatic pubkey auth failed: " + sxc.getError());
    }
  }

  void SSHSession::handleAuthReturn(int auth) {
    if (auth == SSH_AUTH_DENIED)
      throw SSHTunnelException("Authentication failed, access denied.");
    else if (auth == SSH_AUTH_PARTIAL)
      throw SSHTunnelException(
          "Authentication failed, this server require double step authentication which is not supported yet.");
  }
} /* namespace ssh */
