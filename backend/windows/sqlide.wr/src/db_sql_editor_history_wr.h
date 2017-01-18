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

#ifndef __DB_SQL_EDITOR_HISTORY_WR_H__
#define __DB_SQL_EDITOR_HISTORY_WR_H__

#include "sqlide/db_sql_editor_history_be.h"

namespace MySQL {
  namespace GUI {
    namespace Workbench {

      using namespace MySQL::Grt;
      using namespace MySQL::Grt::Db;

    public
      ref class DbSqlEditorHistoryWrapper {
      public:
        typedef ManagedRef<::DbSqlEditorHistory> ^ Ref;

        DbSqlEditorHistoryWrapper(Ref ref);
        Ref ref() {
          return _ref;
        }

      private:
        Ref _ref;

        ~DbSqlEditorHistoryWrapper();

      public:
        VarGridModelWrapper ^ entries_model() { return _entries_model; } VarGridModelWrapper ^
          details_model() { return _details_model; } private : VarGridModelWrapper ^ _entries_model;
        VarGridModelWrapper ^ _details_model;

      public:
        void current_entry(int index) {
          _ref->current_entry(index);
        }
        int current_entry() {
          return _ref->current_entry();
        }

        System::Windows::Forms::ContextMenuStrip ^ get_details_context_menu();
      };

    }; // namespace Workbench
  };   // namespace GUI
};     // namespace MySQL

#endif // __DB_SQL_EDITOR_HISTORY_WR_H__
