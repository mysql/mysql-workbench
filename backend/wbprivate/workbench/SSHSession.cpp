/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include <fcntl.h>
#include <vector>
#include <functional>

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
    _session->setOption(SSH_OPTIONS_USER, credentials.username.c_str());
    _session->setOption(SSH_OPTIONS_HOST, config.remoteSSHhost.c_str());
    _session->setOption(SSH_OPTIONS_PORT, static_cast<long>(config.remoteSSHport));
    _session->setOption(SSH_OPTIONS_TIMEOUT, static_cast<long>(config.connectTimeout));
    _session->setOption(SSH_OPTIONS_STRICTHOSTKEYCHECK, config.strictHostKeyCheck ? 1 : 0);
    if (config.compressionLevel > 0) {
      _session->setOption(SSH_OPTIONS_COMPRESSION, "yes");
      _session->setOption(SSH_OPTIONS_COMPRESSION_LEVEL, config.compressionLevel);
    }

    _config.dumpConfig();

    if (!config.knownHostsFile.empty())
      _session->setOption(SSH_OPTIONS_KNOWNHOSTS, config.knownHostsFile.c_str());

    if (!config.optionsDir.empty())
      _session->setOption(SSH_OPTIONS_SSH_DIR, config.optionsDir.c_str());

    try {
      if (!config.configFile.empty())
        _session->optionsParseConfig(config.configFile.c_str());
    } catch (ssh::SshException &exc) {
      logError("Unable to parse config file: %s\nError was: %s", config.configFile.c_str(), exc.getError().c_str());
    }

    try {
      _session->connect();
    } catch (ssh::SshException &exc) {
      logError("Unable to connect: %s:%lu\nError was: %s", config.remoteSSHhost.c_str(), config.remoteSSHport,
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

    try {
      base::RecMutexLock lock(_sessionMutex, true);

      if (_event == nullptr) {
        _event = ssh_event_new();
        ssh_event_add_session(_event, _session->getCSession());
      }

      logDebug2("Session pool event\n");
      ssh_event_dopoll(_event, 0);
    } catch (...) {
      logDebug2("Can't poll, session busy.\n");
    }
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


  std::string SSHSession::execCmd(std::string command, std::size_t logSize) {
    logDebug2("About to execute command: %s\n", command.c_str());

    auto lock = lockSession();
    auto channel = std::unique_ptr<ssh::Channel, std::function<void(ssh::Channel*)>>(new ssh::Channel(*_session), [](ssh::Channel* chan){
      chan->close();
    });

    channel->openSession();
    channel->requestExec(command.c_str());
    ssize_t readLen = 0, readErrLen = 0;
    std::size_t retryCount = 0;
    std::vector<char> buff(_config.bufferSize, '\0');
    std::string ret;
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
        throw SSHTunnelException(exc.getError());
      }

      if (readLen > 0) {
        std::string data(buff.data(), readLen);
        logDebug3("Read SSH data: %s\n", data.c_str());
        ret.append(data);
      }

      try {
        // Let's see if maybe there are some errors
        readErrLen = channel->read(buff.data(), buff.size(), true, static_cast<int>(1000 * _config.readWriteTimeout));
        if (readErrLen > 0)
        {
          std::string errorMsg(buff.data(), readErrLen);
          logError("Got error: %s\n", errorMsg.c_str());
          throw SSHTunnelException(errorMsg.c_str());
        }
      } catch (SshException &exc) {
        throw SSHTunnelException(exc.getError());
      }

      if (readLen == SSH_ERROR || readLen == SSH_EOF || readErrLen == SSH_ERROR || readErrLen == SSH_EOF)
        throw SSHTunnelException("client disconnected");

      if (readLen == 0 && channel->isEof()) {
        logDebug2("Nothing to read.\n");
        break;
      }

      if (readLen > 0 && (std::size_t) readLen < buff.size()) {//we've got all we need
        logDebug2("All data has been read.\n");
        channel->sendEof();
        break;
      }

      if (ret.size() > logSize) {
        channel->sendEof();
        throw SSHTunnelException("Too much data to read, limit is: " + std::to_string(logSize));
      }
    } while (true);
    return ret;
  }

  std::string SSHSession::execCmdSudo(std::string command, std::string password, std::string passwordQuery,
                                      std::size_t logSize) {

    logDebug2("About to execute elevated command: %s\n", command.c_str());
    auto lock = lockSession();
    auto channel = std::unique_ptr<ssh::Channel, std::function<void(ssh::Channel*)>>(new ssh::Channel(*_session), [](ssh::Channel* chan){
          chan->close();
        });
    channel->openSession();
    channel->requestExec(command.c_str());
    ssize_t readlen = 0, readErrLen = 0;
    ;
    std::size_t writelen = 0;
    std::size_t retryCount = 0;
    std::vector<char> buff(_config.bufferSize, '\0');
    std::string ret;
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
          } else
            throw SSHTunnelException("Unknown password prompt.\n");

        } catch (SshException &) {
          logError("Sudo password didn't came, could be it was cached, we continue and see what will happen\n");
          pwSkip = true;
        }
      }

      try {
        // Let's see if maybe it was wrong sudo credentials
        readErrLen = channel->read(buff.data(), buff.size(), true, static_cast<int>(1000 * _config.readWriteTimeout));
        std::string testString(buff.data(), readErrLen);
        if (readErrLen > 0) {
          logError("Got error: %s\n", testString.c_str());
          if (testString.find("Sorry, try again.") != std::string::npos) {
            logError("Incorrect sudo password.\n");
            throw SSHAuthException("Incorrect sudo password");
          } else if (testString.find("This incident will be reported") != std::string::npos) {
            logError("User not in sudoers files.\n");
            throw SSHAuthException("User not in sudoers");
          } else { //ok this means that there's another error, pass it as a return value
            logDebug2("Got output on stderr: %s\n", testString.c_str());
            ret.append(testString);
          }
        }
      } catch (SshException &exc) {
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
        throw SSHTunnelException(exc.getError());
      }

      if (readlen > 0) {
        std::string data(buff.data(), readlen);
        logDebug3("Read SSH data: %s\n", data.c_str());
        ret.append(data);
      }

      if (readlen == SSH_ERROR || readlen == SSH_EOF) {
        logDebug2("Client disconnected");
        throw SSHTunnelException("client disconnected");
      }

      if (readlen == 0 && channel->isEof()) {
        logDebug2("Nothing to read.\n");
        break;
      }

      if (readlen > 0 && (std::size_t) readlen < buff.size()) { //we've got all we need
        logDebug2("All data has been read.\n");
        channel->sendEof();
        break;
      }

      if (ret.size() > logSize) {
        channel->sendEof();
        throw SSHTunnelException("Too much data to read, limit is: " + std::to_string(logSize));
      }
    } while (true);
    return ret;
  }

  void SSHSession::reconnect() {
    if (!ssh_is_connected(_session->getCSession()))
    {
      disconnect();
      connect(_config, _credentials);
    }
  }

  base::RecMutexLock SSHSession::lockSession() {
    base::RecMutexLock mutexLock(_sessionMutex);
    return mutexLock;
  }

  int SSHSession::verifyKnownHost(const ssh::SSHConnectionConfig &config, std::string &fingerprint) {
    std::unique_ptr<unsigned char, void (*)(unsigned char*)> hash(
        nullptr, [](unsigned char* v) {if (v != nullptr) ssh_clean_pubkey_hash(&v);});
    ssh_key srvPubKey;
    int rc = 0;
    std::size_t hlen = 0;
    errno = 0;
    rc = ssh_get_publickey(_session->getCSession(), &srvPubKey);
    if (rc < 0)
      throw SSHTunnelException("Can't get server pubkey " + getError());

    unsigned char* hashPtr = nullptr;
    errno = 0;
    rc = ssh_get_publickey_hash(srvPubKey, SSH_PUBLICKEY_HASH_SHA1, &hashPtr, &hlen);
    ssh_key_free(srvPubKey);
    if (rc < 0)
      throw SSHTunnelException("Can't calculate pubkey hash " + getError());

    hash.reset(hashPtr);

    std::unique_ptr<char, void (*)(char*)> hexa(ssh_get_hexa(hash.get(), hlen), [](char*ptr) { ssh_string_free_char(ptr); });
    fingerprint = hexa.get();
    int retVal = _session->isServerKnown();
    switch (retVal) {
      case SSH_SERVER_FILE_NOT_FOUND:
      case SSH_SERVER_NOT_KNOWN: {
        if (config.fingerprint.empty())
          return retVal;
        else {
          if (config.fingerprint == hexa.get()) {
            _session->writeKnownhost();
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
        if (ssh_pki_import_privkey_file(credentials.keyfile.c_str(), credentials.keypassword.c_str(), nullptr, nullptr,
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
