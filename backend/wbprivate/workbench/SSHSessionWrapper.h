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

#pragma once

#include "SSHCommon.h"
#include "SSHSession.h"
#include "SSHSftp.h"
#include "base/any.h"
#include "objimpl/db.mgmt/db_mgmt_SSHConnection.h"

namespace ssh {
  class SSHSessionWrapper : public db_mgmt_SSHConnection::ImplData {
    std::shared_ptr<SSHSession> _session;
    SSHConnectionConfig _config;
    SSHConnectionCredentials _credentials;
    std::shared_ptr<SSHSftp> _sftp;
    int _sessionPoolHandle;
    bool _isClosing;
    base::Semaphore _canClose;
  public:
    SSHSessionWrapper(const SSHConnectionConfig &config, const SSHConnectionCredentials &credentials);
    SSHSessionWrapper(const db_mgmt_ConnectionRef connectionProperties);
    SSHSessionWrapper(const db_mgmt_ServerInstanceRef serverInstanceProperties);
    virtual ~SSHSessionWrapper();
    virtual void disconnect() override;
    virtual grt::IntegerRef isConnected() override;
    virtual grt::IntegerRef connect() override;
    virtual grt::DictRef executeCommand(const std::string &command) override;
    virtual grt::DictRef executeSudoCommand(const std::string &command, const std::string &user) override;
    static std::tuple<ssh::SSHConnectionConfig, ssh::SSHConnectionCredentials> getConnectionInfo(
        db_mgmt_ConnectionRef connectionProperties);
    static std::tuple<ssh::SSHConnectionConfig, ssh::SSHConnectionCredentials> getConnectionInfo(
            db_mgmt_ServerInstanceRef serverInstanceProperties);

    static std::string fillupAuthInfo(ssh::SSHConnectionConfig &config, ssh::SSHConnectionCredentials &credentials,
                                      bool resetPassword);
    virtual grt::IntegerRef cd(const std::string &directory) override;
    virtual void get(const std::string &src, const std::string &dest) override;
    virtual grt::StringRef getContent(const std::string &src) override;
    virtual grt::DictListRef ls(const std::string &path) override;
    virtual void mkdir(const std::string &directory) override;
    virtual db_mgmt_SSHFileRef open(const std::string &path) override;
    virtual void put(const std::string &src, const std::string &dest) override;
    virtual grt::StringRef pwd() override;
    virtual void rmdir(const std::string &directory) override;
    virtual void setContent(const std::string &path, const std::string &content) override;
    virtual grt::DictRef stat(const std::string &path) override;
    virtual void unlink(const std::string &file) override;
    virtual grt::IntegerRef fileExists(const std::string &path) override;
  protected:
    mutable base::RecMutex _timeoutMutex;
    base::RecMutexLock lockTimeout();
    void makeSessionPoll();
    bool pollSession();

  };

} /* namespace ssh */
