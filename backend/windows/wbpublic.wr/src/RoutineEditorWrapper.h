/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
  namespace Grt {
    namespace Db {

    public
      ref class RoutineEditorWrapper : public DBObjectEditorWrapper {
      protected:
        RoutineEditorWrapper(::bec::RoutineEditorBE *inn);

      public:
        ::bec::RoutineEditorBE *get_unmanaged_object();
        String ^ get_sql();
        void set_sql(String ^ query);
        String ^ get_name();
        void set_name(String ^ name);
        String ^ get_comment();
        void set_comment(String ^ comment);
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL
