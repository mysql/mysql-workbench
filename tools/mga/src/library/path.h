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

namespace mga {

  class ScriptingContext;

  // Implementation of the entire Node.js path module, except for posix/win32.
  class Path {
  public:
    static std::string dirname(std::string const& path);
    static std::string basename(std::string const& path);
    static std::string extname(std::string const& path);
    static std::string join(std::vector<std::string> const& parts);
    static std::string normalize(std::string const& path);
    static std::string relative(std::string const& from, std::string const& to);
    static std::string resolve(std::vector<std::string> const& parts);

    static bool isAbsolute(std::string const& path);

    static void activate(ScriptingContext &context, JSObject &exports);

    static bool _registered;
  };

} // namespace mga
