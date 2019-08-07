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

#pragma once

#include "SSHTunnelManager.h"
#include "wb_backend_public_interface.h"
#include "driver_manager.h"
#include "base/threading.h"

namespace wb {
  class TunnelManager {
  public:
    TunnelManager();
    ~TunnelManager();

    void start();
    void shutdown();
    void portUsageIncrement(const ssh::SSHConnectionConfig &config);
    void portUsageDecrement(const ssh::SSHConnectionConfig &config);
    std::shared_ptr<SSHTunnel> createTunnel(db_mgmt_ConnectionRef connectionProperties);

  private:
    ssh::SSHTunnelManager *_manager;

    std::map<int, std::pair<ssh::SSHConnectionConfig, base::refcount_t>> _portUsage;
    base::Mutex _usageMapMtx;
  };

  class SSHTunnel {
  private:
    TunnelManager *_tm;
    ssh::SSHConnectionConfig _config;

  public:
    SSHTunnel(TunnelManager *tm, const ssh::SSHConnectionConfig &config) : _tm(tm), _config(config) {
      _tm->portUsageIncrement(_config);
    }

    virtual ~SSHTunnel() {
      disconnect();
    }

    void connect(db_mgmt_ConnectionRef connectionProperties) {
      if (_config.localport == 0)
        throw std::runtime_error("Could not connect SSH tunnel");
    }

    void disconnect() {
      _tm->portUsageDecrement(_config);
    }

    const ssh::SSHConnectionConfig getConfig() const { return _config; }
  };

};
