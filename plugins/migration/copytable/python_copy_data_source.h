/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _PYTHONCOPYDATASOURCE_H_
#define _PYTHONCOPYDATASOURCE_H_

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "copytable.h"

#undef tolower
#undef toupper

class PythonCopyDataSource : public CopyDataSource {
  std::string _connstring;
  std::string _python_module;
  std::string _password;

  PyObject *_connection;
  PyObject *_cursor;
  std::shared_ptr<std::vector<ColumnInfo> > _columns;
  std::vector<SQLSMALLINT> _column_types;
  size_t _column_count;

  bool initialized;

  void _init();
  bool pystring_to_string(PyObject *strobject, std::string &ret_string, bool convert);

public:
  PythonCopyDataSource(const std::string &connstring, const std::string &password);
  virtual ~PythonCopyDataSource();

public:
  virtual size_t count_rows(const std::string &schema, const std::string &table,
                            const std::vector<std::string> &pk_columns, const CopySpec &spec,
                            const std::vector<std::string> &last_pkeys);
  virtual std::shared_ptr<std::vector<ColumnInfo> > begin_select_table(
    const std::string &schema, const std::string &table, const std::vector<std::string> &pk_columns,
    const std::string &select_expression, const CopySpec &spec, const std::vector<std::string> &last_pkeys);
  virtual void end_select_table();
  virtual bool fetch_row(RowBuffer &rowbuffer);
};

#endif
