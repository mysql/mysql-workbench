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
