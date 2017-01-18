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

#include "mysql_schema_editor.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

    public
      ref class MySQLSchemaEditorWrapper : public SchemaEditorWrapper {
      public:
        MySQLSchemaEditorWrapper(GrtValue ^ arglist);
        ~MySQLSchemaEditorWrapper();

        MySQLSchemaEditorBE *get_unmanaged_object();
        bool is_new_object();
        void refactor_catalog_upon_schema_rename(System::String ^ old_name, System::String ^ new_name);

        bool refactor_possible();
        void refactor_catalog();
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL
