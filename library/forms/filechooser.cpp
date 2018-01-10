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

#include "base/log.h"

#include "mforms/mforms.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE)

using namespace mforms;

std::string FileChooser::last_directory = "";

FileChooser::FileChooser(FileChooserType type, bool show_hidden) {
  _filechooser_impl = &ControlFactory::get_instance()->_filechooser_impl;

  _filechooser_impl->create(this, 0, type, show_hidden);
  if (!FileChooser::last_directory.empty())
    _filechooser_impl->set_directory(this, last_directory);
}

FileChooser::FileChooser(mforms::Form *owner, FileChooserType type, bool show_hidden) {
  _filechooser_impl = &ControlFactory::get_instance()->_filechooser_impl;

  _filechooser_impl->create(this, owner, type, show_hidden);

  if (!FileChooser::last_directory.empty())
    _filechooser_impl->set_directory(this, last_directory);
}

void FileChooser::set_title(const std::string &title) {
  _filechooser_impl->set_title(this, title);
}

bool FileChooser::run_modal() {
  bool retval = _filechooser_impl->run_modal(this);
  if (retval) {
    std::string path = _filechooser_impl->get_path(this);
    if (!path.empty())
      FileChooser::last_directory = base::dirname(path);
  }
  return retval;
}

void FileChooser::set_directory(const std::string &path) {
  _filechooser_impl->set_directory(this, path);
}

void FileChooser::set_path(const std::string &path) {
  _filechooser_impl->set_path(this, path);
}

std::string FileChooser::get_path() {
  return _filechooser_impl->get_path(this);
}

std::string FileChooser::get_directory() {
  return _filechooser_impl->get_directory(this);
}

void FileChooser::set_extensions(const std::string &extensions, const std::string &default_extension,
                                 bool allow_all_file_types) {
  _filechooser_impl->set_extensions(this, extensions, default_extension, allow_all_file_types);
}

void FileChooser::add_selector_option(const std::string &name, const std::string &label,
                                      const StringPairVector &options) {
  std::vector<std::string> values;
  for (StringPairVector::const_iterator i = options.begin(); i != options.end(); ++i)
    values.push_back(i->first);
  _selector_options[name] = values;
  _filechooser_impl->add_selector_option(this, name, label, options);
}

void FileChooser::add_selector_option(const std::string &name, const std::string &label, const std::string &options) {
  std::vector<std::pair<std::string, std::string> > olist(split_extensions(options, false));
  std::vector<std::string> values;
  for (std::vector<std::pair<std::string, std::string> >::const_iterator i = olist.begin(); i != olist.end(); ++i)
    values.push_back(i->first);
  _selector_options[name] = values;
  _filechooser_impl->add_selector_option(this, name, label, olist);
}

std::string FileChooser::get_selector_option_value(const std::string &name) {
  return _filechooser_impl->get_selector_option_value(this, name);
}

FileChooser::StringPairVector FileChooser::split_extensions(const std::string &extensions, bool file_extensions) {
  StringPairVector exts;
  std::string::size_type s, e;
  std::string label, pattern;
  std::string part;

  s = 0;
  do {
    e = extensions.find('|', s);
    if (e == std::string::npos) {
      printf("ERROR: extension list %s contains errors\n", extensions.c_str());
      return exts;
    }
    label = extensions.substr(s, e - s);
    s = e + 1;
    e = extensions.find('|', s);
    if (e == std::string::npos)
      pattern = extensions.substr(s);
    else {
      pattern = extensions.substr(s, e - s);
      s = e + 1;
    }

    if (pattern[0] != '*' && file_extensions) {
      logWarning("ERROR: extension list %s contains errors (file extension pattern should start with *)\n",
                 extensions.c_str());
      continue;
    }
    exts.push_back(std::make_pair(label, pattern));
  } while (e != std::string::npos);

  return exts;
}
