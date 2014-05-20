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

#pragma once

#include "grtdb/editor_dbobject.h"

namespace bec {

class WBPUBLICBACKEND_PUBLIC_FUNC RoutineEditorBE : public DBObjectEditorBE
{
public:
  RoutineEditorBE(GRTManager *grtm, const db_RoutineRef &routine); 

  virtual std::string get_title();
  virtual db_RoutineRef get_routine() = 0;
  virtual std::string get_sql();
};

} // namespace bec 
