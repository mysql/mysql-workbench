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
