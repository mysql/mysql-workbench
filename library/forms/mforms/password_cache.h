/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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
