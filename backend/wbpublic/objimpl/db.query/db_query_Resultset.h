/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DB_QUERY_RESULTSET_H_
#define _DB_QUERY_RESULTSET_H_

#include <grts/structs.db.query.h>
#include "sqlide/recordset_be.h"
#include <cppconn/resultset.h>

db_query_ResultsetRef WBPUBLICBACKEND_PUBLIC_FUNC grtwrap_recordset(GrtObjectRef owner, Recordset::Ref rset);
db_query_ResultsetRef WBPUBLICBACKEND_PUBLIC_FUNC grtwrap_recordset(GrtObjectRef owner,
                                                                    std::shared_ptr<sql::ResultSet> result);

class WBPUBLICBACKEND_PUBLIC_FUNC db_query_Resultset::ImplData {
protected:
  ImplData(db_query_ResultsetRef aself);

  db_query_Resultset *self;

public:
  std::map<std::string, ssize_t> column_by_name;

  virtual ~ImplData();

  virtual void refresh() = 0;
  virtual grt::StringRef sql() const = 0;
  virtual grt::IntegerRef currentRow() const = 0;
  virtual grt::IntegerRef rowCount() const = 0;
  virtual grt::DoubleRef floatFieldValue(ssize_t column) = 0;
  virtual grt::DoubleRef floatFieldValueByName(const std::string &column) = 0;
  virtual grt::IntegerRef goToFirstRow() = 0;
  virtual grt::IntegerRef goToLastRow() = 0;
  virtual grt::IntegerRef goToRow(ssize_t row) = 0;
  virtual grt::IntegerRef intFieldValue(ssize_t column) = 0;
  virtual grt::IntegerRef intFieldValueByName(const std::string &column) = 0;
  virtual grt::IntegerRef nextRow() = 0;
  virtual grt::IntegerRef previousRow() = 0;
  virtual grt::IntegerRef saveFieldValueToFile(ssize_t column, const std::string &file) = 0;
  virtual grt::StringRef stringFieldValue(ssize_t column) = 0;
  virtual grt::StringRef stringFieldValueByName(const std::string &column) = 0;
  virtual grt::StringRef geoStringFieldValue(ssize_t column) = 0;
  virtual grt::StringRef geoStringFieldValueByName(const std::string &column) = 0;
  virtual grt::StringRef geoJsonFieldValue(ssize_t column) = 0;
  virtual grt::StringRef geoJsonFieldValueByName(const std::string &column) = 0;
};

class WBPUBLICBACKEND_PUBLIC_FUNC WBRecordsetResultset : public db_query_Resultset::ImplData {
public:
  size_t cursor;
  std::shared_ptr<Recordset> recordset;

  WBRecordsetResultset(db_query_ResultsetRef aself, std::shared_ptr<Recordset> rset);
  virtual grt::StringRef sql() const;
  virtual grt::IntegerRef currentRow() const;
  virtual grt::IntegerRef rowCount() const;
  virtual grt::DoubleRef floatFieldValue(ssize_t column);
  virtual grt::DoubleRef floatFieldValueByName(const std::string &column);
  virtual grt::IntegerRef goToFirstRow();
  virtual grt::IntegerRef goToLastRow();
  virtual grt::IntegerRef goToRow(ssize_t row);
  virtual grt::IntegerRef intFieldValue(ssize_t column);
  virtual grt::IntegerRef intFieldValueByName(const std::string &column);
  virtual grt::IntegerRef nextRow();
  virtual grt::IntegerRef previousRow();

  virtual void refresh();
  virtual grt::StringRef stringFieldValue(ssize_t column);
  virtual grt::StringRef stringFieldValueByName(const std::string &column);
  virtual grt::StringRef geoStringFieldValue(ssize_t column);
  virtual grt::StringRef geoStringFieldValueByName(const std::string &column);
  virtual grt::StringRef geoJsonFieldValue(ssize_t column);
  virtual grt::StringRef geoJsonFieldValueByName(const std::string &column);
  virtual grt::IntegerRef saveFieldValueToFile(ssize_t column, const std::string &file);
};
#endif
