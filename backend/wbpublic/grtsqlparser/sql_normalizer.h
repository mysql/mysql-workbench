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

#ifndef _SQL_NORMALIZER_H_
#define _SQL_NORMALIZER_H_

#include "wbpublic_public_interface.h"
#include <string>
#include <memory>

/**
 * Defines interface to normalize provided SQL statement.
 * Normalization means transformation of all forms of equivalent SQL statements to the same basis form.
 * Addresses:
 * <li>optional braces
 * <li>optional default values
 * <li>forced quotation of identifiers
 * <li>etc.
 *
 * Used by SQL synchronization.
 *
 * @ingroup sqlparser
 */
class WBPUBLICBACKEND_PUBLIC_FUNC Sql_normalizer {
public:
  typedef std::shared_ptr<Sql_normalizer> Ref;
  virtual ~Sql_normalizer() {
  }

protected:
  Sql_normalizer() : _delimiter("\\") {
  }

public:
  void delimiter(std::string val) {
    _delimiter = val;
  }

  virtual std::string normalize(const std::string &sql, const std::string &schema_name) = 0;
  virtual std::string remove_inter_token_spaces(const std::string &text);

protected:
  std::string _delimiter;
};

#endif // _SQL_NORMALIZER_H_
