/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_TUNNEL_H_
#define _WB_TUNNEL_H_

#include "SSHTunnelManager.h"
#include "wb_backend_public_interface.h"
#include "driver_manager.h"
#include "base/threading.h"

class SSHTunnel;

namespace wb
{
  class TunnelManager
  {
    ssh::SSHTunnelManager *_manager;
    friend class ::SSHTunnel;

    std::map<int, std::pair<ssh::SSHConnectionConfig, base::refcount_t>> _portUsage;
    base::Mutex _usageMapMtx;
  public:
    TunnelManager();
    ~TunnelManager();

    void start();
    void shutdown();
    void portUsageIncrement(const ssh::SSHConnectionConfig &config);
    void portUsageDecrement(const ssh::SSHConnectionConfig &config);
    std::shared_ptr<sql::TunnelConnection> createTunnel(db_mgmt_ConnectionRef connectionProperties);
  };
};


#endif
