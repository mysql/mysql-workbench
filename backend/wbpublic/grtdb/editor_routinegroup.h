/* 
 * Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _EDITOR_ROUTINE_GROUP_H_
#define _EDITOR_ROUTINE_GROUP_H_

#include "grtdb/editor_dbobject.h"
#include "grt/tree_model.h"
#include "grts/structs.workbench.physical.h"
#include "wbpublic_public_interface.h"

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC RoutineGroupEditorBE : public DBObjectEditorBE
  {
  protected: 
    db_RoutineGroupRef _group;
    bool _has_syntax_error;

  public:
    RoutineGroupEditorBE(GRTManager *grtm, const db_RoutineGroupRef &group, const db_mgmt_RdbmsRef &rdbms);

    virtual std::string get_title();

    virtual db_DatabaseObjectRef get_dbobject() { return get_routine_group(); }
    virtual db_RoutineGroupRef get_routine_group() { return _group; }

    virtual std::string get_routines_sql();
    //virtual std::string RoutineGroupEditorBE::get_routine_name(const std::string& id);
    virtual std::string get_routine_name(const std::string& id);
    virtual std::vector<std::string> get_routines_names();
    virtual void set_routines_sql(const std::string &sql, bool sync);
    grt::ValueRef parse_sql(grt::GRT*, grt::StringRef sql);
    bool has_syntax_error() { return _has_syntax_error; }

    virtual MySQLEditor::Ref get_sql_editor();
    
    virtual std::string get_routine_sql(db_RoutineRef routine);

    virtual void delete_routine_with_name(const std::string& str);
    virtual void remove_routine_by_index(size_t index);
    virtual void append_routine_with_id(const std::string& id);
    void open_editor_for_routine_at_index(size_t index);
  
  private:
    std::string set_routine_newlines(const std::string &routine);

  protected:
    virtual std::string get_object_type();
};

} // namespace bec

#endif /* _EDITOR_ROUTINE_GROUP_H_ */
