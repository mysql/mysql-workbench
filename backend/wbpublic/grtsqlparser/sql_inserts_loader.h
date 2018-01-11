/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SQL_INSERTS_LOADER_H_
#define _SQL_INSERTS_LOADER_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../wbpublic_public_interface.h"

/**
 * Defines interface to facilitate loading data from inserts SQL script.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_inserts_loader {
public:
  typedef std::shared_ptr<Sql_inserts_loader> Ref;

public:
  Sql_inserts_loader() {
  }
  virtual ~Sql_inserts_loader() {
  }

  virtual void load(const std::string &sql, const std::string &schema_name) = 0;

  typedef std::vector<std::string> Strings;
  typedef std::function<void(const std::string &, const std::pair<std::string, std::string> &, const Strings &,
                             const Strings &, const std::vector<bool> &)>
    Process_insert; // sql, schema_name, table_name, fields_names, fields_values
  void process_insert_cb(Process_insert cb) {
    _process_insert = cb;
  }

protected:
  Process_insert _process_insert;
};

#endif // _SQL_INSERTS_LOADER_H_
