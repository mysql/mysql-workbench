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

#include "grtdb/editor_dbobject.h"

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC SchemaEditorBE : public DBObjectEditorBE {
  public:
    SchemaEditorBE(const db_SchemaRef& schema);

    virtual std::string get_title();

    virtual db_SchemaRef get_schema() = 0;

    virtual void set_name(const std::string& name);

    // table options
    virtual void set_schema_option_by_name(const std::string& name, const std::string& value);
    virtual std::string get_schema_option_by_name(const std::string& name);
  };
};
