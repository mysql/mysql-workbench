/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _STUB_FILECHOOSER_H_
#define _STUB_FILECHOOSER_H_

#include "stub_mforms.h"
#include "stub_view.h"

#define RESPONSE_OK 1
#define RESPONSE_CANCEL 0

namespace mforms {
  namespace stub {

    class FileChooserWrapper : public ViewWrapper {
      static std::vector<std::string> split_string(const std::string &s, const std::string &sep) {
        std::vector<std::string> parts;
        std::string ss = s;

        std::string::size_type p;

        if (s.empty())
          return parts;

        p = ss.find(sep);
        while (!ss.empty() && p != std::string::npos) {
          parts.push_back(ss.substr(0, p));
          ss = ss.substr(p + sep.size());

          p = ss.find(sep);
        }
        parts.push_back(ss);

        return parts;
      }

      static bool create(FileChooser *self, mforms::Form *owner, FileChooserType type, bool show_hidden) {
        return true;
      }

      static void set_title(::mforms::FileChooser *self, const std::string &title) {
      }

      static bool show_modal(::mforms::FileChooser *self) {
        return true;
      }

      static void set_directory(FileChooser *self, const std::string &path) {
      }

      static std::string get_directory(FileChooser *self) {
        return "";
      }

      static std::string get_path(FileChooser *self) {
        return "";
      }

      static void set_extensions(FileChooser *self, const std::string &extensions, const std::string &default_extension,
                                 bool allow_all_file_types = true) {
      }

      static void setPath(FileChooser *self, const std::string &path) {

      }

      static void addSelectorOption(FileChooser *self, const std::string &name, const std::string &label,
                                  const std::vector<std::pair<std::string, std::string> > &options) {

      }

      static std::string getSelectorOptionValue(FileChooser *self, const std::string &name) {
        return "";
      }

      FileChooserWrapper(::mforms::FileChooser *form, ::mforms::FileChooserType type) : ViewWrapper(form) {
      }

      virtual ~FileChooserWrapper() {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_filechooser_impl.create = &FileChooserWrapper::create;
        f->_filechooser_impl.set_title = &FileChooserWrapper::set_title;
        f->_filechooser_impl.run_modal = &FileChooserWrapper::show_modal;
        f->_filechooser_impl.set_extensions = &FileChooserWrapper::set_extensions;
        f->_filechooser_impl.set_directory = &FileChooserWrapper::set_directory;
        f->_filechooser_impl.get_directory = &FileChooserWrapper::get_directory;
        f->_filechooser_impl.get_path = &FileChooserWrapper::get_path;
        f->_filechooser_impl.set_path = &FileChooserWrapper::setPath;
        f->_filechooser_impl.add_selector_option = &FileChooserWrapper::addSelectorOption;
        f->_filechooser_impl.get_selector_option_value = &FileChooserWrapper::getSelectorOptionValue;


      }
    };
  };
};

#endif
