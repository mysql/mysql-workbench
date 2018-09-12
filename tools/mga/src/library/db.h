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

#include "jsexport.h"
#include "common.h"
#include <cppconn/connection.h>

namespace mga {

  class DbResult;

  class DbConnection : public JSExport {
  public:
    DbConnection() = delete;
    DbConnection(std::unique_ptr<sql::Connection> conn);

    static void registerInContext(ScriptingContext &ctx, JSObject &exports);

    void disconnect();
    bool isConnected();
    DbResult* query(const std::string &query);
    void executeScript(const std::string &script, const std::string &delimiter, bool ignoreErrors = false);

  protected:
    void doExecuteScript(const std::string &content, const std::string &delimiter, bool ignoreErrors);

    static JSVariant getter(ScriptingContext *, JSExport *instance, std::string const& key);
    static void setter(ScriptingContext *, JSExport *instance, std::string const& key, JSVariant const& value);

    virtual void finalize(ScriptingContext *context) override;

  private:
    std::unique_ptr<sql::Connection> _connection;
  };

  class ScriptingContext;

  class Db : public JSExport {
  public:
    static void activate(ScriptingContext &context, JSObject &exports);
    static void threadCleanup();

    static sql::Driver* mysqlDriver;

    static bool _registered;
  private:
    static void establishConnection(JSExport *, JSValues &args);
  };

}
