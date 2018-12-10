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
#include <errno.h>
#include <fcntl.h>
#ifndef _MSC_VER
#include <poll.h>
#endif
#include <string.h>
#include <thread>
#include <map>
#include <mutex>
#include <vector>
#include "SSHCommon.h"
#include "SSHSession.h"

namespace ssh {
  class WBSSHLIBRARY_PUBLIC_FUNC SSHTunnelHandler : public SSHThread {
  public:
    SSHTunnelHandler(uint16_t localPort, int localSocket, std::shared_ptr<ssh::SSHSession> session);
    ~SSHTunnelHandler();
    int getLocalSocket() const;
    int getLocalPort() const;
    SSHConnectionConfig getConfig() const;

    void handleConnection();
    void handleNewConnection(int incomingSocket);
    void transferDataFromClient(int sock, ssh::Channel *chan);
    void transferDataToClient(int sock, ssh::Channel *chan);

    std::unique_ptr<ssh::Channel> openTunnel();
    void prepareTunnel(int clientSocket);

  protected:
    virtual void run() override;

    std::shared_ptr<SSHSession> _session;
    uint16_t _localPort;
    int _localSocket;
    std::map<int, std::unique_ptr<ssh::Channel>> _clientSocketList;
    int _pollTimeout;
    ssh_event _event;
    std::vector<int> _sockRemovalList;
    std::recursive_mutex _newConnMtx;
    std::vector<int> _newConnection;
  };

} /* namespace ssh */
