/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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
