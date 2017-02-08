/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "output.h"
#include <base/file_functions.h>

namespace mtemplate {

  TemplateOutput::TemplateOutput() {
  }

  TemplateOutput::~TemplateOutput() {
  }

  //-----------------------------------------------------------------------------------
  //  TemplateOutputString stuff
  //-----------------------------------------------------------------------------------
  void TemplateOutputString::out(const base::utf8string &str) {
    _buffer += str;
  }

  const base::utf8string &TemplateOutputString::get() {
    return _buffer;
  }

  //-----------------------------------------------------------------------------------
  //  TemplateOutputFile stuff
  //-----------------------------------------------------------------------------------
  TemplateOutputFile::TemplateOutputFile(const base::utf8string &filename) : _file(filename.c_str(), "w+") {
  }

  void TemplateOutputFile::out(const base::utf8string &str) {
    fwrite(str.data(), 1, str.bytes(), _file.file());
  }

} //  namespace mtemplate
