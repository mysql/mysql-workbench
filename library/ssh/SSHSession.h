/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#pragma once

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#else
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
#include <libssh/libsshpp.hpp>
#include <libssh/sftp.h>
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#else
#pragma warning(pop)
#endif

#include "SSHCommon.h"
#include "base/any.h"
#include "base/threading.h"
#include <memory>

namespace ssh {
  class WBSSHLIBRARY_PUBLIC_FUNC SSHSession {
    ssh::Session *_session;
    SSHConnectionConfig _config;
    SSHConnectionCredentials _credentials;
    bool _isConnected;
    ssh_event _event;
    mutable base::Mutex _sessionMutex;
  public:
    static std::shared_ptr<SSHSession> createSession();
    virtual ~SSHSession();
    std::tuple<SSHReturnType, base::any> connect(const SSHConnectionConfig &config,
                                                 const SSHConnectionCredentials &credentials);

    void pollEvent();
    void disconnect();
    bool isConnected() const;
    SSHConnectionConfig getConfig() const;
    ssh::Session* getSession() const;
    std::tuple<std::string, std::string, int> execCmd(std::string command, std::size_t logSize = LOG_SIZE_100MB);
    std::tuple<std::string, std::string, int> execCmdSudo(std::string command, std::string password,
                                                          std::string passwordQuery = "EnterPasswordHere",
                                                          std::size_t logSize = LOG_SIZE_100MB);

    base::MutexLock lockSession();
    void reconnect();
  protected:
    SSHSession();
    SSHSession(const SSHSession& ses) = delete;
    SSHSession(const SSHSession&& ses) = delete;
    SSHSession &operator =(SSHSession&) = delete;
    int verifyKnownHost(const ssh::SSHConnectionConfig &config, std::string &fingerprint);
    void authenticateUser(const SSHConnectionCredentials &credentials);
    void authPassword(const std::string &password);
    void authAutoPubkey();
    void handleAuthReturn(int auth);
    bool openChannel(ssh::Channel *chann);
  };

} /* namespace ssh */
