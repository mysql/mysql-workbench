/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_routine_editor.h"
#include "GrtTemplates.h"
#include "grtdb/db_object_helpers.h"

using namespace System;
using namespace System::Collections::Generic;

namespace MySQL {
namespace Grt {
namespace Db {

public ref class MySQLRoutineEditorWrapper : public RoutineEditorWrapper
{
protected:
  MySQLRoutineEditorWrapper(::MySQLRoutineEditorBE *inn)
    : RoutineEditorWrapper(inn)
  {}

public:
  MySQLRoutineEditorWrapper(MySQL::Grt::GrtManager^ grtm, MySQL::Grt::GrtValue^ arglist)
  : RoutineEditorWrapper(
      new ::MySQLRoutineEditorBE(grtm->get_unmanaged_object(), 
      db_mysql_RoutineRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0))
        )
      )
  {
  }

  ::MySQLRoutineEditorBE *get_unmanaged_object()
  { return static_cast<::MySQLRoutineEditorBE *>(inner); }
  
  void load_routine_sql();
  void commit_changes();
};

} // namespace Db
} // namespace Grt
} // namespace MySQL
