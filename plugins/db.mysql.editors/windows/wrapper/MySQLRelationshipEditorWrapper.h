/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "mysql_relationship_editor.h"

#pragma make_public(::RelationshipEditorBE)

namespace MySQL {
  namespace Grt {
    namespace Db {

    public
      enum class RelationshipVisibilityType {
        Visible = RelationshipEditorBE::Visible,
        Splitted = RelationshipEditorBE::Splitted,
        Hidden = RelationshipEditorBE::Hidden
      };

    public
      ref class MySQLRelationshipEditorWrapper : public BaseEditorWrapper {
      public:
        MySQLRelationshipEditorWrapper(MySQL::Grt::GrtValue ^ arglist);
        ~MySQLRelationshipEditorWrapper();

        RelationshipEditorBE *get_unmanaged_object();

        void set_caption(String ^ caption);
        String ^ get_caption();
        String ^ get_caption_long();
        void set_extra_caption(String ^ caption);
        String ^ get_extra_caption();
        String ^ get_extra_caption_long();

        String ^ get_right_table_name();
        String ^ get_left_table_name();
        String ^ get_right_table_info();
        String ^ get_left_table_info();

        String ^ get_left_table_fk();

        void set_left_mandatory(bool flag);
        bool get_left_mandatory();
        void set_right_mandatory(bool flag);
        bool get_right_mandatory();

        void set_to_many(bool flag);
        bool get_to_many();

        void set_comment(String ^ comment);
        String ^ get_comment();

        RelationshipVisibilityType get_visibility();
        void set_visibility(RelationshipVisibilityType v);

        void open_editor_for_left_table();
        void open_editor_for_right_table();

        bool get_is_identifying();
        void set_is_identifying(bool identifying);
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL
