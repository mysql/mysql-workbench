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
