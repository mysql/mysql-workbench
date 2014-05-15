/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __ROUTINE_GROUP_EDITOR_H__
#define __ROUTINE_GROUP_EDITOR_H__

#include "DBObjectEditorWrapper.h"
#include "GrtTemplates.h"
#include "grtdb/editor_routinegroup.h"

namespace MySQL {
namespace Grt {
namespace Db {

public ref class RoutineGroupEditorBE : public DBObjectEditorWrapper
{
protected:
  RoutineGroupEditorBE(::bec::RoutineGroupEditorBE *inn);

public:
  ::bec::RoutineGroupEditorBE *get_unmanaged_object();
  String^ get_routines_sql();
  String^ get_routine_sql(MySQL::Grt::GrtValue^ routine);
  void set_routines_sql(String ^query, bool sync);
  List<String^>^ get_routines_names();
  void delete_routine_with_name(String^ name);
  void append_routine_with_id(String^ id);
  String^ get_routine_name(String^ id);
  String^ get_name();
  void set_name(String ^query);
  String^ get_comment();
  void set_comment(String ^query);
  void open_editor_for_routine_at_index(size_t index);
};

} // namespace Db
} // namespace Grt
} // namespace MySQL

#endif // __ROUTINE_GROUP_EDITOR_H__