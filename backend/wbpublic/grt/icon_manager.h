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

#pragma once

#include "grt.h"

#include "wbpublic_public_interface.h"
#include <unordered_map>

namespace bec {
  typedef ssize_t IconId;

  enum IconSize { Icon11 = 11, Icon12 = 12, Icon16 = 16, Icon24 = 24, Icon32 = 32, Icon48 = 48, Icon64 = 64 };

  class WBPUBLICBACKEND_PUBLIC_FUNC IconManager {
    std::string _basedir;
    std::map<std::string, IconId> _icon_ids;
    std::map<IconId, std::string> _icon_files;
    std::vector<std::string> _search_path;

    std::unordered_map<std::string, std::string> _icon_paths;

    IconId _next_id;

    IconManager();

  public:
    static IconManager *get_instance();

    std::string get_icon_path(const std::string &file);

    IconId get_icon_id(const std::string &icon_file, IconSize size = Icon16, const std::string &extra_qualifier = "");

    IconId get_icon_id(const grt::ObjectRef &object, IconSize size = Icon16, const std::string &extra_qualifier = "");
    IconId get_icon_id(grt::MetaClass *metaclass, IconSize size = Icon16, const std::string &extra_qualifier = "");

    std::string get_icon_file(IconId icon);
    std::string get_icon_path(IconId icon);

    void set_basedir(const std::string &basedir);

    void add_search_path(const std::string &path);
  };
};
