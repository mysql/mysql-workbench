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

  typedef std::function<bool(std::string const&)> FilterFunction;

  struct CopyOptions {
    bool overwrite = true; // If false and target exists the operation silently fails. Use errorOnExist to change that.
    bool errorOnExist = false;       // If set to true and overwrite to false throws an exception if the target exists.
    bool dereference = false;        // Dereference symlinks.
    bool preserveTimestamps = false; // Set target modification and access times to that of the source.
    FilterFunction filter = [](std::string const& fileName) { // TODO: use filter actually
      std::ignore = fileName;
      return true;
    }; // File filter when copying a folder.
  };

  struct CreateOptions {
    bool append = false; // When true, the content is append at the end of the file.
    bool failOnExist = false; // Will throw if the file already exists.
    bool truncate = true; // When true, the file is truncated before write.
  };

  struct MoveOptions {
    bool overwrite = false; // Overwrite existing files or folders.
  };

  class ScriptingContext;

  // File system related functionality. Modeled after both the standard fs + fs-extra modules.
  class FS {
  public:
    static std::string contentFromFile(std::string const& fileName);

    static void copy(std::string const& source, std::string const& target, CopyOptions options);
    static void emptyDir(std::string const& directory);
    static void ensureDir(std::string const& directory);
    static bool isDir(std::string const& path);
    static bool isFile(std::string const& path);
    static bool isSymlink(std::string const& path);
    static void move(std::string const& source, std::string const& target, MoveOptions options);
    static void outputFile(std::string const& fileName, std::string const& content, CreateOptions createOptions);
    static bool pathExists(std::string const& path);
    static void remove(std::string const& path);
    static std::vector<std::string> readDir(std::string const& directory);
    static void chmod(std::string const& path, int mode);

    static void activate(ScriptingContext &context, JSObject &exports);

    static bool _registered;
  };

} // namespace mga
