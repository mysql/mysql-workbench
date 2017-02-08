/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
