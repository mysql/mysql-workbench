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

#include "utilities.h"
#include "process.h"
#include "scripting-context.h"

#include "path.h"

using namespace mga;

//----------------- Path -----------------------------------------------------------------------------------------------

/**
 * Extracts the folder part of the given path. Returns an empty string if no path separator could be found.
 */
std::string Path::dirname(std::string const& path) {
  auto lastSeparator = path.rfind("/");
#ifdef _MSC_VER
  auto lastBackslash = path.rfind("\\");
  if (lastSeparator != std::string::npos) {
    if (lastBackslash != std::string::npos)
      lastSeparator = std::max<std::size_t>(lastSeparator, lastBackslash);
  } else {
    lastSeparator = lastBackslash;
  }
#endif

  if (lastSeparator != std::string::npos)
    return path.substr(0, lastSeparator);
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Extracts the filename part of the given path (the part after the last separator).
 *
 * Returns the given path if no separator could be found.
 */
std::string Path::basename(std::string const& path) {
  auto lastSeparator = path.rfind("/");
  if (lastSeparator != std::string::npos)
    return path.substr(lastSeparator + 1);

#ifdef _MSC_VER
  lastSeparator = path.rfind("\\");
  if (lastSeparator != std::string::npos)
    return path.substr(lastSeparator + 1);
#endif

  return path;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the extension of a given path.
 * Considers special cases like:
 *   path/.config/myconfig
 */
std::string Path::extname(std::string const& path) {
  std::string::size_type p = path.rfind('.');
  if (p != std::string::npos) {
    std::string ext(path.substr(p));
    if (ext.find('/') != std::string::npos || ext.find('\\') != std::string::npos)
      return "";
    return ext;
  }
  
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Join the given sub path elements into a single path and normalize the result.
 */
std::string Path::join(std::vector<std::string> const& parts) {
  return normalize(Utilities::concat(parts, "/"));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Implements the behavior as described for Node.js (https://nodejs.org/api/path.html#path_path_resolve_paths).
 */
std::string Path::resolve(std::vector<std::string> const& parts) {
  if (parts.empty())
    return Process::cwd();

  std::string result;
  for (ssize_t i = static_cast<ssize_t>(parts.size()) - 1; i >= 0; --i) {
    if (parts[static_cast<size_t>(i)].empty())
      continue;
    if (result.empty())
      result = parts[static_cast<size_t>(i)];
    else
      result = join({ parts[static_cast<size_t>(i)], result });
    if (isAbsolute(result))
      break;
  }

  // Check if it's empty even after having empty entries in the vector.
  if (result.empty())
    return Process::cwd();

  result = normalize(result);
  if (!isAbsolute((result)))
    result = join({ Process::cwd(), result });

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts all backward slashes to forward slashes, resolves "." and ".." as well as empty path segments
 * (multiple slashes in a row).
 *
 * Returns the current dir (".") if the resulting path is empty.
 *
 * Note: paths with leading slash are relative on Windows, but will be treated here as absolute paths.
 */
std::string Path::normalize(std::string const& path) {
  if (path.empty())
    return ".";

  if (path.size() < 2)
    return path;

  std::vector<std::string> segments = Utilities::splitBySet(path, "/\\");
  if (segments.size() < 2)
    return path;

  std::vector<std::string> normalizedSegments;

  size_t index = 0;
  if (path.substr(0, 2) == R"(\\)") {
    normalizedSegments.push_back(R"(\\)"); // Server path on Windows.
    index += 2;
  } else if (segments[0].empty()) {
    normalizedSegments.push_back("/"); // For an absolute path.
    ++index;
  }

  // Parent folder references (..) are resolved as long as we have parent folders.
  // Oherwise they are taken over as-is.
  while (index < segments.size()) {
    if (segments[index] == "..") {
      switch (normalizedSegments.size()) {
        case 0: // Nothing to resolve. Take over the reference.
          normalizedSegments.push_back("..");
          break;
        case 1: // When the path is absolute ignore the reference.
          if (normalizedSegments.back() == "/")
            break;
          // else fall-through
        default:
          normalizedSegments.pop_back();
          break;
      }
    } else if (!segments[index].empty() && segments[index] != ".")
      normalizedSegments.push_back(segments[index]);

    ++index;
  }

  std::string result;
  for (size_t i = 0; i < normalizedSegments.size(); ++i) {
    result += normalizedSegments[i];
    if (i == 0 && (result == R"(\\)" || result == R"(/)" || result == R"(\)"))
      continue;
    else if (i < normalizedSegments.size() - 1)
      result += "/";
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Implements the behavior as described for Node.js (https://nodejs.org/api/path.html#path_path_relative_from_to).
 */
std::string Path::relative(std::string const& from, std::string const& to) {
  std::string start = from.empty() ? Process::cwd() : normalize(from);
  std::string target = to.empty() ? Process::cwd() : normalize(to);

  if (start == target) {
    return ".";
  }
  
  std::vector<std::string> startElements = Utilities::splitBySet(start, "\\/");
  std::vector<std::string> targetElements = Utilities::splitBySet(target, "\\/");

  size_t i = 0;
  while (i < startElements.size() && i < targetElements.size()) {
    if (startElements[i] != targetElements[i]) // Should probably be a full Unicode comparison. Can duktape help here?
      break;
    ++i;
  }

  std::string result;
  for (size_t j = i; j < startElements.size(); ++j)
    result += "../";

  for (size_t j = i; j < targetElements.size(); ++j)
    result += targetElements[j] + "/";

  result.resize(result.size() - 1); // Remove trailing slash.
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Determines if a path is an absolute path.
 */
bool Path::isAbsolute(std::string const& path) {
  if (path.empty())
    return false;

#ifdef _MSC_VER
  if (path.size() < 2)
    return false;

  if (Utilities::hasPrefix(path, "//") || Utilities::hasPrefix(path, "\\\\") || path[1] == ':')
    return true;

  return false;
#else
  return path[0] == '/';
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void Path::activate(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineFunction({ "dirname" }, 1, [](JSExport *, JSValues &args) {
    args.pushResult(Path::dirname(args.get(0)));
  });

  exports.defineFunction({ "basename" }, 1, [](JSExport *, JSValues &args) {
    args.pushResult(Path::basename(args.get(0)));
  });

  exports.defineFunction({ "extname" }, 1, [](JSExport *, JSValues &args) {
    args.pushResult(Path::extname(args.get(0)));
  });

  exports.defineFunction({ "isAbsolute" }, 1, [](JSExport *, JSValues &args) {
    args.pushResult(Path::isAbsolute(args.get(0)));
  });

  exports.defineFunction({ "join" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    std::vector<std::string> parts;
    for (size_t i = 0; i < args.size(); ++i) {
      parts.push_back(args.as(ValueType::String, i));
    }

    args.pushResult(Path::join(parts));
  });

  exports.defineFunction({ "normalize" }, 1, [](JSExport *, JSValues &args) {
    args.pushResult(Path::normalize(args.get(0)));
  });

  exports.defineFunction({ "relative" }, 2, [](JSExport *, JSValues &args) {
    std::string from = args.get(0);
    std::string to = args.get(1);
    args.pushResult(Path::relative(from, to));
  });

  exports.defineFunction({ "resolve" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    std::vector<std::string> parts;
    for (size_t i = 0; i < args.size(); ++i) {
      if (!args.is(ValueType::String, i))
        args.context()->throwScriptingError(ScriptingError::Type, "Only strings are supported");
      parts.push_back(args.as(ValueType::String, i));
    }

    args.pushResult(Path::resolve(parts));
  });

#ifdef _MSC_VER
  exports.defineProperty("delimiter", ";");
  exports.defineProperty("sep", "\\");
#else
  exports.defineProperty("delimiter", ":");
  exports.defineProperty("sep", "/");
#endif
}

//----------------------------------------------------------------------------------------------------------------------

bool Path::_registered = []() {
  ScriptingContext::registerModule("path", &Path::activate);
  return true;
}();
