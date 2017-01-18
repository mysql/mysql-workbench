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

#ifndef _SCHEMA_EDITOR_WRAPPER_H_
#define _SCHEMA_EDITOR_WRAPPER_H_

#include "grtdb/editor_schema.h"
#include "DBObjectEditorWrapper.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

    public
      ref class SchemaEditorWrapper : public DBObjectEditorWrapper {
      protected:
        SchemaEditorWrapper(::bec::SchemaEditorBE *inn);

      public:
        ::bec::SchemaEditorBE *get_unmanaged_object();
        void set_schema_option_by_name(System::String ^ name, System::String ^ value);
        String ^ get_schema_option_by_name(String ^ name);
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL

#endif // _SCHEMA_EDITOR_WRAPPER_H_