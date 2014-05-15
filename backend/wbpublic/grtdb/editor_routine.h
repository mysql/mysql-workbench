/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _EDITOR_ROUTINE_H_
#define _EDITOR_ROUTINE_H_

#include "grtdb/editor_dbobject.h"

namespace bec {

class WBPUBLICBACKEND_PUBLIC_FUNC RoutineEditorBE : public DBObjectEditorBE
{
protected:
  db_RoutineRef _routine;

public:
  RoutineEditorBE(GRTManager *grtm, const db_RoutineRef &routine, const db_mgmt_RdbmsRef &rdbms); 

  virtual std::string get_title();

  virtual db_DatabaseObjectRef get_dbobject() { return get_routine(); }
  virtual db_RoutineRef get_routine() { return _routine; }

  virtual std::string get_sql();
  virtual void set_sql(const std::string &sql, bool sync);
  
  std::string get_sql_definition_header();
  virtual std::string get_sql_template(const std::string &template_name, int &cursor_pos);
  
  std::string get_formatted_sql_for_editing(int &cursor_pos);
  
  grt::ValueRef parse_sql(grt::GRT*, grt::StringRef sql);

protected:
  virtual std::string get_object_type();
};

} // namespace bec 

#endif /* _EDITOR_ROUTINE_H_ */
