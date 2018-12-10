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

#ifndef _MSC_VER
#include <glib.h>
#include <algorithm>
#endif

#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "icon_manager.h"
#include "common.h"
#ifdef __APPLE__
#include "mforms/app.h"
#endif

/**
 * @file  icon_manager.cpp
 * @brief
 */

using namespace bec;

IconManager::IconManager() {
  gchar *tmp = g_get_current_dir();
  _basedir = tmp;
  g_free(tmp);

  _next_id = 1;
  /* do not hardcode stuff
    add_search_path(".");
    add_search_path("./images");
    add_search_path("./images/grt");
    add_search_path("./images/grt/structs");
    add_search_path("./images/icons");
  */
}

void IconManager::set_basedir(const std::string &basedir) {
  _basedir = basedir;
}

IconManager *IconManager::get_instance() {
  static IconManager inst;

  return &inst;
}

static std::string get_icon_file_for_size(const std::string &aicon_file, IconSize size,
                                          const std::string &extra_qualifier) {
  std::string file;
  std::string icon_file = aicon_file;

  if (!extra_qualifier.empty())
    icon_file = base::replaceString(icon_file, "$", extra_qualifier + ".$");

  if (icon_file.find('$') != std::string::npos) {
    // strip the .png
    file = icon_file.substr(0, icon_file.rfind('$'));

    switch (size) {
      case Icon11:
        file += "11x11";
        break;
      case Icon12:
        file += "12x12";
        break;
      case Icon16:
        file += "16x16";
        break;
      case Icon24:
        file += "24x24";
        break;
      case Icon32:
        file += "32x32";
        break;
      case Icon48:
        file += "48x48";
        break;
      case Icon64:
        file += "64x64";
        break;
    }
    file += icon_file.substr(icon_file.rfind('$') + 1);
  } else
    file = icon_file;

  return file;
}

std::string IconManager::get_icon_path(const std::string &file) {
  std::unordered_map<std::string, std::string>::const_iterator it = _icon_paths.find(file);
  if (it != _icon_paths.end())
    return it->second;

  for (std::vector<std::string>::const_iterator i = _search_path.begin(); i != _search_path.end(); ++i) {
    std::string path = _basedir + G_DIR_SEPARATOR + *i + G_DIR_SEPARATOR + file;

#ifdef __APPLE__
    std::string mac_path;

    mac_path = base::strip_extension(path) + "_mac.png";
    if (g_file_test(mac_path.c_str(), G_FILE_TEST_EXISTS)) {
      _icon_paths[file] = mac_path;
      return mac_path;
    }
#endif

    if (g_file_test(path.c_str(), G_FILE_TEST_EXISTS)) {
      _icon_paths.insert(std::make_pair(file, path));
      return path;
    }
  }
  _icon_paths.insert(std::make_pair(file, ""));

  return "";
}

IconId IconManager::get_icon_id(const std::string &icon_file, IconSize size, const std::string &extra_qualifier) {
  std::map<std::string, IconId>::iterator it;
  std::string file = get_icon_file_for_size(icon_file, size, extra_qualifier);

  if ((it = _icon_ids.find(file)) != _icon_ids.end()) {
    return it->second;
  }
  _icon_files[_next_id] = file;
  _icon_ids[file] = _next_id;
  return _next_id++;
}

IconId IconManager::get_icon_id(const grt::ObjectRef &object, IconSize size, const std::string &extra_qualifier) {
  return get_icon_id(object.get_metaclass(), size, extra_qualifier);
}

IconId IconManager::get_icon_id(grt::MetaClass *metaclass, IconSize size, const std::string &extra_qualifier) {
  grt::MetaClass *parent, *gstruct;
  std::string file, path;

  parent = metaclass;

  do {
    gstruct = parent;

    file = gstruct->get_attribute("icon");
    if (file.empty())
      file = std::string(gstruct->name()) + ".$.png";

    file = get_icon_file_for_size(file, size, extra_qualifier);

    path = get_icon_path(file);

    parent = gstruct->parent();
  } while (path.empty() && parent);

  std::map<std::string, IconId>::iterator it;
  if ((it = _icon_ids.find(file)) != _icon_ids.end()) {
    return it->second;
  }
  _icon_files[_next_id] = file;
  _icon_ids[file] = _next_id;
  return _next_id++;
}

std::string IconManager::get_icon_file(IconId icon) {
  if (icon == 0)
    return "";

  return _icon_files[icon];
}

std::string IconManager::get_icon_path(IconId icon) {
  std::string file = get_icon_file(icon);
  if (file.empty())
    return "";

  return get_icon_path(file);
}

void IconManager::add_search_path(const std::string &path) {
  std::string npath;

#ifdef _MSC_VER
  npath = base::replaceString(path, "/", G_DIR_SEPARATOR_S);
#else
  npath = path;
#endif

  if (std::find(_search_path.begin(), _search_path.end(), npath) == _search_path.end() &&
      g_file_test((_basedir + G_DIR_SEPARATOR + npath).c_str(), G_FILE_TEST_IS_DIR))
    _search_path.push_back(npath);
}
