/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench/wb_context.h"

static wb::WBOptions *options;

#include "../stub_app.h"

using namespace mforms;
using namespace mforms::stub;

static std::list<std::string> directory_names;

//----------------------------------------------------------------------------------------------------------------------

static void scan_dir_names(const std::string &dir) {
  GError *error = NULL;
  GDir *d = g_dir_open(dir.c_str(), 0, &error);
  if (d) {
    directory_names.push_back(dir);
    const char *f;
    while ((f = g_dir_read_name(d))) {
      if (g_file_test((dir + "/" + f).c_str(), G_FILE_TEST_IS_DIR)) {
        scan_dir_names(dir + "/" + f);
      }
    }
    g_dir_close(d);
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string AppWrapper::get_resource_path(App *app, const std::string &file) {
  if (directory_names.empty()) {
    std::string basedir = (options != NULL) ? options->basedir + "/" : "";
    scan_dir_names(basedir + "data");
    scan_dir_names(basedir + "modules");
#ifdef __APPLE__
    scan_dir_names(basedir);
#else
    scan_dir_names(basedir + "images");
#endif
  }

  if (file.empty())
    return "./";

  std::string f;
  for (std::list<std::string>::const_iterator i = directory_names.begin(); i != directory_names.end(); ++i) {
    if (g_file_test((*i + "/" + file).c_str(), G_FILE_TEST_EXISTS))
      return *i + "/" + file;
  }

  return f;
}

//----------------------------------------------------------------------------------------------------------------------

void AppWrapper::init(wb::WBOptions *theOptions) {
  options = theOptions;

  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_app_impl.set_status_text = &AppWrapper::set_status_text;

  f->_app_impl.get_resource_path = &AppWrapper::get_resource_path;
  f->_app_impl.get_application_bounds = &AppWrapper::get_application_bounds;

  f->_app_impl.enter_event_loop = &AppWrapper::enter_event_loop;
  f->_app_impl.exit_event_loop = &AppWrapper::exit_event_loop;
  f->_app_impl.get_executable_path = &AppWrapper::getExecutablePath;
  f->_app_impl.backing_scale_factor = &AppWrapper::backingScaleFactor;
}

//----------------------------------------------------------------------------------------------------------------------
