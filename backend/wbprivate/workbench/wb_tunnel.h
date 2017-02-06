/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _WB_TUNNEL_H_
#define _WB_TUNNEL_H_

#include "python_context.h"
#include "cppdbc.h"
#include "wb_backend_public_interface.h"

class SSHTunnel;

namespace wb {
  class WBContext;

  class TunnelManager {
    wb::WBContext *_wb;
    grt::AutoPyObject _tunnel;

    friend class ::SSHTunnel;

    int lookup_tunnel(const char *server, const char *username, const char *target);
    int open_tunnel(const char *server, const char *username, const char *password, const char *keyfile,
                    const char *target);
    void wait_tunnel(int port);
    void set_keepalive(int port, int keepalive);

  public:
    TunnelManager(wb::WBContext *wb);
    ~TunnelManager();

    wb::WBContext *wb() {
      return _wb;
    }

    void start();
    void shutdown();

    std::shared_ptr<sql::TunnelConnection> create_tunnel(db_mgmt_ConnectionRef connectionProperties);

    bool get_message_for(int port, std::string &type, std::string &message);
  };
};

#endif
