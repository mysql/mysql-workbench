/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQL_SQL_EDITOR_BE_H_
#define _MYSQL_SQL_EDITOR_BE_H_

#include "db_mysql_sqlide_public_interface.h"
#include "sqlide/sql_editor_be.h"

class DB_MYSQL_SQLIDE_PUBLIC_FUNC Mysql_sql_editor : public Sql_editor
{
protected:
  virtual bool fill_auto_completion_keywords(std::vector<std::pair<int, std::string> > &entries,
    AutoCompletionWantedParts parts, bool upcase_keywords);
public:
  Mysql_sql_editor(db_mgmt_RdbmsRef rdbms, GrtVersionRef version);
  virtual ~Mysql_sql_editor();

};


#endif /* _MYSQL_SQL_EDITOR_BE_H_ */
