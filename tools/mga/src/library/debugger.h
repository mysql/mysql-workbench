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

#pragma once

#include "duktape.h"

#include "common.h"

namespace mga {

  class ScriptingContext;
  
  // Connectivity support for the duktape debugger.
  class DebugAdapter {
  public:
    DebugAdapter(ScriptingContext *context, int port = 9091);

    bool isOpen();
    void close();
  private:
#ifdef _MSC_VER
    long long _serverSocket;
    long long _clientSocket;
    long long _port;
#else
    int _serverSocket;
    int _clientSocket;
    int _port;
#endif
    
    void initializeAndWait();

    static void debuggerDetached(duk_context *, void *data);
    static duk_size_t readCallback(void *data, char *buffer, duk_size_t length);
    static duk_size_t writeCallback(void *data, const char *buffer, duk_size_t length);
    static duk_size_t peekCallback(void *data);
  };

} // namespace mga
