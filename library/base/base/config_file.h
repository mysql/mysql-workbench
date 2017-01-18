/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include <vector>
#include <fstream>
#include <string>

#include "common.h"

namespace base {

  enum ConfigFileFlags {
    AutoCreateNothing = 0,
    AutoCreateSections = 1, // Automatically create sections if they don't exist and get a value.
    AutoCreateKeys = 2      // Create a key if written to but does not exist yet.
  };

  inline ConfigFileFlags operator|(ConfigFileFlags a, ConfigFileFlags b) {
    return ConfigFileFlags((int)a | (int)b);
  }

  class BASELIBRARY_PUBLIC_FUNC ConfigurationFile {
  public:
    ConfigurationFile(ConfigFileFlags flags);
    ConfigurationFile(std::string file_name, ConfigFileFlags flags);
    virtual ~ConfigurationFile();

    bool load(const std::string &file_name);
    bool save(const std::string &file_name);

    void clear_includes(const std::string &section_name);
    void add_include(const std::string &section_name, const std::string &include);
    void add_include_dir(const std::string &section_name, const std::string &include);
    std::vector<std::string> get_includes(const std::string &section_name);

    std::string get_value(std::string key, std::string section = "");
    double get_float(std::string key, std::string section = "");
    int get_int(std::string key, std::string section = "");
    bool get_bool(std::string key, std::string section = "");

    bool set_value(std::string key, std::string value, std::string section = "");
    bool set_float(std::string key, float fValue, std::string section = "");
    bool set_int(std::string key, int nValue, std::string section = "");
    bool set_bool(std::string key, bool bValue, std::string section = "");
    bool set_key_pre_comment(std::string key, std::string comment, std::string section = "");
    bool set_key_post_comment(std::string key, std::string comment, std::string section = "");
    bool set_section_comment(std::string section, std::string comment);

    bool delete_key(std::string key, std::string from_section = "");
    bool delete_section(std::string section);

    bool create_key(std::string key, std::string value, std::string pre_comment = "", std::string post_comment = "",
                    std::string section = "");
    bool create_section(std::string section_name, std::string comment = "");

    int section_count();
    int key_count();
    int key_count_for_secton(const std::string &section_name);
    void clear();
    bool is_dirty();
    bool has_key(const std::string &key, const std::string &section);
    bool has_section(const std::string &section_name);

  private:
    class Private;
    Private *data;
  };

} // namespace base

#endif // _CONFIG_FILE_H_
