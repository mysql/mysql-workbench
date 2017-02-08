/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

namespace MySQL {
  namespace Forms {

  public
    class FileChooserWrapper : public ViewWrapper {
    private:
      mforms::FileChooserType type;

    protected:
      FileChooserWrapper(mforms::FileChooser *form, mforms::Form *owner);

      static bool create(mforms::FileChooser *backend, mforms::Form *owner, mforms::FileChooserType type,
                         bool show_hidden);
      static void set_title(mforms::FileChooser *backend, const std::string &title);
      static bool run_modal(mforms::FileChooser *backend);
      static void set_directory(mforms::FileChooser *backend, const std::string &path);
      static void set_path(mforms::FileChooser *backend, const std::string &path);
      static std::string get_directory(mforms::FileChooser *backend);
      static std::string get_path(mforms::FileChooser *backend);
      static void set_extensions(mforms::FileChooser *backend, const std::string &extensions,
                                 const std::string &default_extension, bool allow_all_file_types = true);
      static void add_selector_option(mforms::FileChooser *backend, const std::string &name, const std::string &label,
                                      const mforms::FileChooser::StringPairVector &options);
      static std::string get_selector_option_value(mforms::FileChooser *backend, const std::string &name);

    public:
      static void init();
    };
  };
};
