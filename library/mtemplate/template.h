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
#include "types.h"
#include "dictionary.h"
#include "modifier.h"
#include "output.h"

#include <string>

namespace mtemplate {

  class MTEMPLATELIBRARY_PUBLIC_FUNC Template {
  protected:
    TemplateDocument _document;

  public:
    Template(TemplateDocument document);
    ~Template();

    void expand(DictionaryInterface *dict, TemplateOutput *output);
    void dump(int indent = 0);
  };

  MTEMPLATELIBRARY_PUBLIC_FUNC Template *GetTemplate(const base::utf8string &path, PARSE_TYPE type = DO_NOT_STRIP);

} //  namespace mtemplate
