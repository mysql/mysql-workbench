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

#include "GrtTemplates.h"
#include "db_sql_editor_history_wr.h"

using namespace System::Windows::Forms;

using namespace MySQL::Forms;

namespace MySQL {
  namespace GUI {
    namespace Workbench {

      using namespace MySQL::Grt::Db;

      //----------------- DbSqlEditorHistoryWrapper ------------------------------------------------------

      DbSqlEditorHistoryWrapper::DbSqlEditorHistoryWrapper(Ref ref) : _ref(ref) {
        _entries_model = Ref2Ptr_<::VarGridModel, VarGridModelWrapper>(_ref->entries_model());
        _details_model = Ref2Ptr_<::VarGridModel, VarGridModelWrapper>(_ref->details_model());
      }

      //--------------------------------------------------------------------------------------------------

      DbSqlEditorHistoryWrapper::~DbSqlEditorHistoryWrapper() {
        delete _ref;
      }

      //--------------------------------------------------------------------------------------------------

      ContextMenuStrip ^ DbSqlEditorHistoryWrapper::get_details_context_menu() {
        return dynamic_cast<ContextMenuStrip ^>(
          ObjectMapper::GetManagedComponent(_ref->details_model()->get_context_menu()));
      }

      //--------------------------------------------------------------------------------------------------

    }; // namespace Workbench
  };   // namespace GUI
};     // namespace MySQL
