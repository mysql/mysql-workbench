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

#pragma once

#include "common.h"

#include "base/utf8string.h"
#include "base/file_utilities.h"
// class FILE;

namespace mtemplate {

  struct MTEMPLATELIBRARY_PUBLIC_FUNC TemplateOutput {
    TemplateOutput();
    virtual ~TemplateOutput();

    virtual void out(const base::utf8string &str) = 0;
  };

  class MTEMPLATELIBRARY_PUBLIC_FUNC TemplateOutputString : public TemplateOutput {
    base::utf8string _buffer;

  public:
    virtual void out(const base::utf8string &str);

    const base::utf8string &get();
  };

  class MTEMPLATELIBRARY_PUBLIC_FUNC TemplateOutputFile : public TemplateOutput {
    base::FileHandle _file;

  public:
    TemplateOutputFile(const base::utf8string &filename);
    virtual void out(const base::utf8string &str);
  };

} //  namespace mtemplate
