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

#pragma once

#include "grtdb/editor_dbobject.h"

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC RoutineGroupEditorBE : public DBObjectEditorBE {
  public:
    RoutineGroupEditorBE(const db_RoutineGroupRef &group);

    virtual std::string get_title();

    virtual db_RoutineGroupRef get_routine_group() = 0;

    virtual std::string get_sql();
    std::string get_routine_sql(db_RoutineRef routine);

    virtual std::vector<std::string> get_routines_names();

    void delete_routine_with_name(const std::string &str);
    void remove_routine_by_index(size_t index);
    void append_routine_with_id(const std::string &id);

    void open_editor_for_routine_at_index(size_t index);

  private:
    std::string set_routine_newlines(const std::string &routine);
  };

} // namespace bec
