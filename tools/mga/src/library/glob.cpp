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

#include "path.h"
#include "filesystem.h"
#include "utilities.h"
#include "process.h"
#include "scripting-context.h"

#include "glob.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Recursively matches all files in the folder tree given by directory.
 */
void globInFolder(std::string const& pattern, std::string const& directory, std::vector<std::string> &files) {
  for (auto &entry : FS::readDir(directory)) {
    if (entry == "node_modules")
      continue;

    std::string subPath = Path::join({ directory, entry });
    if (FS::isDir(subPath))
      globInFolder(pattern, subPath, files);
    else {
#ifdef _MSC_VER
      std::wstring convertedPath = Utilities::s2ws(subPath);
      std::wstring convertedPattern = Utilities::s2ws(pattern);
      if (PathMatchSpec(convertedPath.c_str(), convertedPattern.c_str()))
        files.push_back(subPath);
      else
        continue;
#else
      if (fnmatch(pattern.c_str(), subPath.c_str(), 0) == 0)
        files.push_back(subPath);
#endif
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns all file names in the given folder (or below that) which match the given glob pattern
 */
std::vector<std::string> Glob::glob(std::string const& pattern, std::string const& directory) {
  std::vector<std::string> result;
  globInFolder(pattern, directory, result);
  std::sort(result.begin(), result.end());

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Glob::activate(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineFunction({ "sync" }, 2, [](JSExport *, JSValues &args) {
    std::string pattern = args.get(0); // Options are ignored for now.

    try {
      JSArray result(args.context());
      for (auto &entry : Glob::glob(pattern, Process::cwd())) {
        result.addValue(entry);
      }
      args.pushResult(result);
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });
}

//----------------------------------------------------------------------------------------------------------------------

bool Glob::_registered = []() {
  ScriptingContext::registerModule("glob", &Glob::activate);
  return true;
}();
