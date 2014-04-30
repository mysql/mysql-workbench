/*
 * Copyright (c) 2013 Oracle and/or its affiliates. All rights reserved.
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

#ifndef _PYTHONCOPYDATASOURCE_H_
#define _PYTHONCOPYDATASOURCE_H_

#include <Python.h>
#include "copytable.h"

#undef tolower
#undef toupper

class PythonCopyDataSource : public CopyDataSource
{
  std::string _connstring;
  std::string _python_module;
  std::string _password;

  PyObject *_connection;
  PyObject *_cursor;
  boost::shared_ptr<std::vector<ColumnInfo> > _columns;
  std::vector<SQLSMALLINT> _column_types;
  size_t _column_count;

  bool initialized;

  void _init();
  bool pystring_to_string(PyObject *strobject, std::string &ret_string, bool convert);
public:
  PythonCopyDataSource(const std::string &connstring,
                     const std::string &password);
  virtual ~PythonCopyDataSource();

public:
  virtual size_t count_rows(const std::string &schema, const std::string &table, const CopySpec &spec);
  virtual boost::shared_ptr<std::vector<ColumnInfo> > begin_select_table(const std::string &schema, const std::string &table,
                                                                         const std::string &select_expression,
                                                                         const CopySpec &spec);
  virtual void end_select_table();
  virtual bool fetch_row(RowBuffer &rowbuffer);
};

#endif
