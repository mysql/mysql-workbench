/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <cstddef>

namespace mforms {
  class PasswordCache {
    char *storage;
    size_t storage_len;
    size_t storage_size;

    PasswordCache();
    static PasswordCache instance;

    const char *find_password(const std::string &service, const std::string &account);
    size_t find_block(const std::string &service, const std::string &account);

  public:
    static PasswordCache *get();

    ~PasswordCache();

    void add_password(const std::string &service, const std::string &account, const char *password);
    void remove_password(const std::string &service, const std::string &account);

    bool get_password(const std::string &service, const std::string &account, std::string &ret_password);
  };
};
