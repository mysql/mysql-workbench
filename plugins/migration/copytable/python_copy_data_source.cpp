/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _MSC_VER
#define HAVE_ROUND
#endif

#include "python_copy_data_source.h"
#include "copytable.h"

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/sqlstring.h"

#include "converter.h"

#include <boost/algorithm/string.hpp>

DEFAULT_LOG_DOMAIN("copytable");

PythonCopyDataSource::PythonCopyDataSource(const std::string &connstring, const std::string &password)
  : _password(password), _connection(NULL), _cursor(NULL), initialized(false) {
  // connstring comes as "pythonmodule://connection_parameters"
  std::vector<std::string> conn_parts = base::split(connstring, "://", 1);
  if (conn_parts.size() != 2)
    std::logic_error(base::strfmt("Wrong format for the connection string: '%s'", connstring.c_str()));
  _python_module = conn_parts[0];
  _connstring = conn_parts[1];

  _get_field_lengths_from_target = true;

#if defined(WIN32)
  char wbcopytablepath[2048];
  size_t pathsize = GetModuleFileName(NULL, wbcopytablepath, 2048);
  if (!pathsize)
    throw std::runtime_error("Could not get the full path to wbcopytables.exe");
  std::string basepath(wbcopytablepath, pathsize);
  _putenv(base::strfmt("PYTHONPATH=%s\\python;%s\\python\\DLLs;%s\\python\\lib", basepath.c_str(), basepath.c_str(),
                       basepath.c_str())
            .c_str());
  _putenv(base::strfmt("PYTHONHOME=%s\\python", basepath.c_str()).c_str());
#endif
}

PythonCopyDataSource::~PythonCopyDataSource() {
  PyGILState_STATE state = PyGILState_Ensure();
  Py_XDECREF(_cursor);
  Py_XDECREF(_connection);
  PyGILState_Release(state);
}

bool PythonCopyDataSource::pystring_to_string(PyObject *strobject, std::string &ret_string, bool convert = false) {
  if (strobject == Py_None) {
    ret_string = "";
    return true;
  }
  if (PyUnicode_Check(strobject)) {
    PyObject *ref = PyUnicode_AsUTF8String(strobject);
    if (ref) {
      const char *s;
      Py_ssize_t len;
      s = PyUnicode_AsUTF8AndSize(ref, &len);
      if (s)
        ret_string = std::string(s, len);
      else
        ret_string = "";
      Py_DECREF(ref);
      return true;
    }
    return false;
  }

  if (PyUnicode_Check(strobject)) {
    const char *s;
    Py_ssize_t len;
    s = PyUnicode_AsUTF8AndSize(strobject, &len);
    if (s)
      ret_string = std::string(s, len);
    else
      ret_string = "";
    return true;
  }

  if (convert) {
    PyObject *str = PyObject_Str(strobject);
    if (str) {
      bool ret = pystring_to_string(str, ret_string, false);
      Py_DECREF(str);
      return ret;
    }
  }

  return false;
}

void PythonCopyDataSource::_init() // This has to be executed from the same thread that copies the data
{
  if (initialized)
    return;

  PyGILState_STATE state = PyGILState_Ensure();
  PyObject *pDBModule = PyImport_ImportModule(_python_module.c_str());
  if (!pDBModule || pDBModule == Py_None) {
    if (PyErr_Occurred())
      PyErr_Print();
    PyGILState_Release(state);
    throw ConnectionError("Could not load Python module", _python_module.c_str());
  }

  std::string full_connection_string(_connstring);
  base::replaceStringInplace(full_connection_string, "%password%", _password);
  PyObject *pAstModule = PyImport_ImportModule("ast");
  PyObject *pLiteralEvalFunction = PyObject_GetAttrString(pAstModule, "literal_eval");
  PyObject *pLiteralEvalArgs = Py_BuildValue("(s)", full_connection_string.c_str());

  PyObject *params = PyObject_CallObject(pLiteralEvalFunction, pLiteralEvalArgs);

  PyObject *pConnectFunction = PyObject_GetAttrString(pDBModule, "connect");
  if (PyErr_Occurred()) // Could not convert to a python literal using ast.literal_eval(full_connection_string),
  {                     // use the connection string as a python string arg to connect()
    Py_XDECREF(params);
    params = pLiteralEvalArgs;
    Py_INCREF(pLiteralEvalArgs);
    PyErr_Clear();
  }

  if (pConnectFunction && PyCallable_Check(pConnectFunction)) {
    if (PyUnicode_Check(params) || PyUnicode_Check(params)) {
      PyObject *connection_arg = PyTuple_Pack(1, params);
      _connection = PyObject_CallObject(pConnectFunction, connection_arg);
      Py_DECREF(connection_arg);
    } else if (PyDict_Check(params))
      _connection = PyObject_Call(pConnectFunction, Py_None, params);
    else if (PyTuple_Check(params))
      _connection = PyObject_CallObject(pConnectFunction, params);
    else
      throw std::runtime_error(
        base::strfmt("The connection string %s does not represent a Python str, dict or tuple literal. Aborting...",
                     _connstring.c_str()));

    if (PyErr_Occurred()) {
      PyErr_Print();
      PyGILState_Release(state);
      throw std::runtime_error(
        base::strfmt("Could not successfully call %s.connect(%s)\n", _python_module.c_str(), _connstring.c_str()));
    }

    if (_connection == NULL || _connection == Py_None) {
      Py_DECREF(pConnectFunction);
      Py_DECREF(pDBModule);
      PyErr_Print();
      PyGILState_Release(state);
      throw ConnectionError("Connection error", "Could not get a connect object");
    }
    // Here we have a valid connection object
    logInfo("Connection to '%s' opened\n", _connstring.c_str());
    _cursor = PyObject_CallMethod(_connection, (char *)"cursor", NULL);
    if (_cursor == NULL || _cursor == Py_None) {
      PyGILState_Release(state);
      throw std::runtime_error("Could not get a cursor to the DB connection\n");
    }
  } else if (PyErr_Occurred()) {
    PyErr_Print();
    PyGILState_Release(state);
    throw std::runtime_error(base::strfmt("Cannot find function %s.connect\n", _python_module.c_str()));
  }
  initialized = true;
  Py_XDECREF(params);
  Py_DECREF(pLiteralEvalArgs);
  Py_DECREF(pLiteralEvalFunction);
  Py_DECREF(pAstModule);
  Py_XDECREF(pConnectFunction);
  // Py_DECREF(pDBModule);
  PyGILState_Release(state);
}

size_t PythonCopyDataSource::count_rows(const std::string &schema, const std::string &table,
                                        const std::vector<std::string> &pk_columns, const CopySpec &spec,
                                        const std::vector<std::string> &last_pkeys) {
  _init();

  PyGILState_STATE state = PyGILState_Ensure();

  std::string q;
  if (!_schema_name.empty() && base::trim(_schema_name, "`\"'") != "def") {
    q = base::strfmt("USE %s", schema.c_str());
    PyObject_CallMethod(_cursor, (char *)"execute", (char *)"(s)", q.c_str());
    if (PyErr_Occurred()) {
      PyErr_Print();
      logWarning("The query \"USE %s\" failed\n", schema.c_str());
    }
  }

  switch (spec.type) {
    case CopyAll:
      if (spec.resume && last_pkeys.size())
        q = base::strfmt("SELECT count(*) FROM %s WHERE %s", table.c_str(),
                         get_where_condition(pk_columns, last_pkeys).c_str());
      else
        q = base::strfmt("SELECT count(*) FROM %s", table.c_str());
      break;
    case CopyRange: {
      std::string start_expr, end_expr;
      if (spec.range_end < 0)
        end_expr = "";
      else
        end_expr = base::strfmt("%s <= %lli", spec.range_key.c_str(), spec.range_end);
      start_expr = base::strfmt("%s >= %lli", spec.range_key.c_str(), spec.range_start);
      if (!end_expr.empty())
        q =
          base::strfmt("SELECT count(*) FROM %s WHERE %s AND %s", table.c_str(), start_expr.c_str(), end_expr.c_str());
      else
        q = base::strfmt("SELECT count(*) FROM %s WHERE %s", table.c_str(), start_expr.c_str());
      break;
    }
    case CopyCount: {
      if (spec.resume && last_pkeys.size())
        q = base::strfmt("SELECT count(*) FROM %s WHERE %s LIMIT %lli", table.c_str(),
                         get_where_condition(pk_columns, last_pkeys).c_str(), spec.row_count);
      else
        q = base::strfmt("SELECT count(*) FROM %s LIMIT %lli", table.c_str(), spec.row_count);
      break;
    }
    case CopyWhere: {
      q = base::strfmt("SELECT count(*) FROM %s WHERE %s", table.c_str(), spec.where_expression.c_str());
      break;
    }
  }

  if (PyObject_CallMethod(_cursor, (char *)"execute", (char *)"(s)", q.c_str()) == NULL) {
    PyGILState_Release(state);
    throw ConnectionError("Python DB API Module query error", "Query '" + q + "' failed");
  }

  PyObject *row = PyObject_CallMethod(_cursor, (char *)"fetchone", NULL);

  if (!row || !PySequence_Check(row) || PySequence_Size(row) != 1) {
    if (PyErr_Occurred())
      PyErr_Print();
    PyGILState_Release(state);
    throw ConnectionError("Python DB API Module query error", "The query '" + q + "' returned unexpected results");
  }

  PyObject *element = PySequence_GetItem(row, 0);
  size_t count = (size_t)PyLong_AsUnsignedLongLongMask(element);
  Py_DECREF(element);
  Py_DECREF(row);

  PyGILState_Release(state);

  if ((spec.type == CopyAll || spec.type == CopyWhere) && spec.max_count > 0 && spec.max_count < (long long)count)
    count = (size_t)spec.max_count;

  return count;
}

std::shared_ptr<std::vector<ColumnInfo> > PythonCopyDataSource::begin_select_table(
  const std::string &schema, const std::string &table, const std::vector<std::string> &pk_columns,
  const std::string &select_expression, const CopySpec &spec, const std::vector<std::string> &last_pkeys) {
  _init();

  std::shared_ptr<std::vector<ColumnInfo> > columns(new std::vector<ColumnInfo>());

  _columns = columns;
  _schema_name = schema;
  _table_name = table;

  std::string q;

  PyGILState_STATE state = PyGILState_Ensure();

  if (!_cursor)
    std::runtime_error("No python cursor available");

  if (!_schema_name.empty() && _schema_name != "def") {
    q = base::strfmt("USE %s", schema.c_str());
    PyObject_CallMethod(_cursor, (char *)"execute", (char *)"(s)", q.c_str());
    if (PyErr_Occurred()) {
      PyErr_Print();
      logWarning("The query \"USE %s\" failed\n", schema.c_str());
    }
  }

  QueryBuilder select_query;
  select_query.select_columns(select_expression);
  select_query.select_from_table(table);
  select_query.add_orderby(boost::algorithm::join(pk_columns, ", "));

  if (spec.type == CopyCount || spec.max_count > 0)
    select_query.add_limit(base::strfmt("%lli", spec.row_count));
  if (spec.resume && last_pkeys.size())
    select_query.add_where(get_where_condition(pk_columns, last_pkeys));
  if (spec.type == CopyRange) {
    select_query.add_where(base::strfmt("%s >= %lli", spec.range_key.c_str(), spec.range_start));
    if (spec.range_end >= 0)
      select_query.add_where(base::strfmt("%s <= %lli", spec.range_key.c_str(), spec.range_end));
  }
  if (spec.type == CopyWhere)
    select_query.add_where(spec.where_expression);

  q = select_query.build_query();

  if (PyObject_CallMethod(_cursor, (char *)"execute", (char *)"(s)", q.c_str()) == NULL) {
    PyGILState_Release(state);
    throw ConnectionError("Python DB API Module query error", "Query '" + q + "' failed");
  }

  PyObject *desc = PyObject_GetAttrString(_cursor, "description");

  if (!desc || !PySequence_Check(desc)) {
    if (PyErr_Occurred())
      PyErr_Print();
    PyGILState_Release(state);
    throw ConnectionError("Python DB API Module query error",
                          "Could not get a resulset descriptor for the query '" + q + "' returned unexpected results");
  }

  for (int i = 0; i < PySequence_Length(desc); i++) {
    PyObject *column_info_tuple = PySequence_GetItem(desc, i);
    if (!column_info_tuple || !PySequence_Check(column_info_tuple) || PySequence_Length(column_info_tuple) < 2) {
      if (PyErr_Occurred())
        PyErr_Print();
      PyGILState_Release(state);
      throw ConnectionError("Python DB API Module query error",
                            "Resulset descriptor for the query '" + q + "' has the wrong format");
    }

    ColumnInfo info;

    PyObject *item = PySequence_GetItem(column_info_tuple, 0);
    pystring_to_string(item, info.source_name);
    Py_DECREF(item);

    item = PySequence_GetItem(column_info_tuple, 1);
    pystring_to_string(item, info.source_type);
    Py_DECREF(item);
    info.source_length = 0;   // The actual value will be taken from the target
    info.is_unsigned = false; // The actual value will be taken from the target
    info.is_long_data = false;

    info.is_long_data = false;

    columns->push_back(info);
    Py_DECREF(column_info_tuple);
  }

  Py_DECREF(desc);
  _column_count = columns->size();
  PyGILState_Release(state);
  return columns;
}

void PythonCopyDataSource::end_select_table() {
}

bool PythonCopyDataSource::fetch_row(RowBuffer &rowbuffer) {
  PyGILState_STATE state = PyGILState_Ensure();
  if (!_cursor || _cursor == Py_None) {
    if (PyErr_Occurred())
      PyErr_Print();
    logError("No cursor object available while attempting to fetch a row. Skipping table %s\n", _table_name.c_str());
    PyGILState_Release(state);
    return false;
  }

  PyObject *row = PyObject_CallMethod(_cursor, (char *)"fetchone", NULL);
  if (row == NULL || row == Py_None) {
    PyGILState_Release(state);
    return false;
  }

  char *buffer;
  size_t buffer_len;
  PyObject *element;

  for (size_t i = 0; i < _column_count; ++i) {
    element = PySequence_GetItem(row, i);
    if (rowbuffer.check_if_blob() || (*_columns)[i].is_long_data || (*_columns)[i].target_type == MYSQL_TYPE_GEOMETRY) {
      if (element == Py_None) {
        rowbuffer.finish_field(true);
        Py_DECREF(element);
        continue;
      }
      if (PyUnicode_Check(element)) {
        PyObject *element_ref = element;
        element = PyUnicode_AsUTF8String(element);
        Py_DECREF(element_ref);
        if (element == NULL || PyErr_Occurred()) {
          if (PyErr_Occurred())
            PyErr_Print();
          logError(
            "An error occurred while encoding unicode data as UTF-8 in a long field object at column %s.%s. Skipping "
            "table!\n.",
            _table_name.c_str(), (*_columns)[i].source_name.c_str());
          PyGILState_Release(state);
          return false;
        }
      } else if (!PyObject_CheckBuffer(element)) // Old-style buffers are the interface specified in PEP 249 for BLOB data.
                                                 // Attempt to convert.
      {
        PyObject *element_copy = element;
        // element = PyBuffer_FromObject(element, 0, Py_END_OF_BUFFER); // FIXME: WL-12709 fix buffer
        Py_DECREF(element_copy);
        if (PyErr_Occurred()) {
          PyErr_Print();
          Py_XDECREF(element);
          logError("Unexpected value for BLOB object at column %s.%s. Skipping table!\n.", _table_name.c_str(),
                   (*_columns)[i].source_name.c_str());
          PyGILState_Release(state);
          return false;
        }
      }
      Py_buffer view;
      //Py_ssize_t blob_read_buffer_len;
      int res = PyObject_GetBuffer(element, &view, PyBUF_SIMPLE);
      if (res != 0) {
        if (PyErr_Occurred()) {
          PyErr_Print();
        }
        logError("Could not get a read buffer for the BLOB column %s.%s. Skipping table!\n", _table_name.c_str(),
                 (*_columns)[i].source_name.c_str());
        Py_DECREF(element);
        PyGILState_Release(state);
        return false;
      }
      if (view.len > _max_parameter_size) {
        if (_abort_on_oversized_blobs) {

          PyGILState_Release(state);
          throw std::runtime_error(base::strfmt("oversized blob found in table %s.%s, size: %lu", _schema_name.c_str(),
                                                _table_name.c_str(), (long unsigned int)view.len));
          PyBuffer_Release(&view);
        } else {
          logError("Oversized blob found in table %s.%s, size: %lu", _schema_name.c_str(), _table_name.c_str(),
                   (long unsigned int)view.len);
          rowbuffer.finish_field(true);
          Py_DECREF(element);
          PyBuffer_Release(&view);
          continue;
        }
      } else { // Proceed to copy from the buffer
        Py_ssize_t copied_bytes = 0;
        if (!view.len) { // empty buffer
          rowbuffer[i].buffer_length = *rowbuffer[i].length = (unsigned long)view.len;
          rowbuffer[i].buffer = nullptr;
        }
        while (copied_bytes < view.len) {
          Py_ssize_t this_pass_size = std::min(view.len - copied_bytes, (Py_ssize_t)_max_blob_chunk_size);
          // ---- Begin Section: This will fail if multiple passes are done. TODO: Fix this.
          if (_use_bulk_inserts) {
            if (rowbuffer[i].buffer_length)
              free(rowbuffer[i].buffer);

            *rowbuffer[i].length = (unsigned long)view.len;
            rowbuffer[i].buffer_length = (unsigned long)view.len;
            rowbuffer[i].buffer = malloc(view.len);

            memcpy(rowbuffer[i].buffer, view.buf, view.len);
          } else
            rowbuffer.send_blob_data((const char*)view.buf + copied_bytes, this_pass_size);
          // ---- End Section
          copied_bytes += this_pass_size;         
        }
        rowbuffer.finish_field(false);
        PyBuffer_Release(&view);
        Py_DECREF(element);
        continue;
      }
    }
    bool was_null = element == Py_None;
    enum enum_field_types target_type = (*_columns)[i].target_type;
    bool is_unsigned = (*_columns)[i].is_unsigned;
    switch (target_type) {
      case MYSQL_TYPE_TINY:
        rowbuffer.prepare_add_tiny(buffer, buffer_len);
        if (!was_null) {
          if (is_unsigned)
            *((unsigned char *)buffer) = (unsigned char)PyLong_AsLong(element);
          else
            *buffer = (char)PyLong_AsLong(element);
        }
        rowbuffer.finish_field(was_null);
        break;
      case MYSQL_TYPE_YEAR:
      case MYSQL_TYPE_SHORT:
        rowbuffer.prepare_add_short(buffer, buffer_len);
        if (!was_null) {
          if (is_unsigned)
            *((unsigned short *)buffer) = (unsigned short)PyLong_AsLong(element);
          else
            *((short *)buffer) = (short)PyLong_AsLong(element);
        }
        rowbuffer.finish_field(was_null);
        break;
      case MYSQL_TYPE_INT24:
      case MYSQL_TYPE_LONG:
        rowbuffer.prepare_add_long(buffer, buffer_len);
        if (!was_null) {
          if (is_unsigned)
            *((unsigned long *)buffer) = PyLong_AsUnsignedLongMask(element);
          else
            *((long *)buffer) = PyLong_AsLong(element);
        }
        rowbuffer.finish_field(was_null);
        break;
      case MYSQL_TYPE_LONGLONG:
        rowbuffer.prepare_add_bigint(buffer, buffer_len);
        if (!was_null) {
          if (is_unsigned)
            *((unsigned long long *)buffer) =
              PyLong_Check(element) ? PyLong_AsUnsignedLongLongMask(element) : PyLong_AsUnsignedLongLong(element);
          else
            *((long long *)buffer) = PyLong_AsLongLong(element);
        }
        rowbuffer.finish_field(was_null);
        break;
      case MYSQL_TYPE_FLOAT:
        rowbuffer.prepare_add_float(buffer, buffer_len);
        if (!was_null)
          *((float *)buffer) = (float)PyFloat_AsDouble(element);
        rowbuffer.finish_field(was_null);
        break;
      case MYSQL_TYPE_DOUBLE:
        rowbuffer.prepare_add_double(buffer, buffer_len);
        if (!was_null)
          *((double *)buffer) = PyFloat_AsDouble(element);
        rowbuffer.finish_field(was_null);
        break;
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_NEWDATE:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_TIMESTAMP:
        rowbuffer.prepare_add_time(buffer, buffer_len);
        // The select query can yield these fields as Unicode/strings or as datetime.* objects
        if (element == Py_None) // element is NULL
          ((MYSQL_TIME *)buffer)->time_type = MYSQL_TIMESTAMP_NONE;
        else {
          if (PyObject_HasAttrString(element, "isoformat")) // element is a python datetime.* object
          {
            PyObject *old_ref = element;
            element = PyObject_CallMethod(
              element, (char *)"isoformat",
              NULL); // Will return an ISO 8601 string representation of the date/time/datetime object
            Py_DECREF(old_ref);
          }
          if (PyUnicode_Check(element) ||
              PyUnicode_Check(element)) // element is a string (sqlite sends time data as strings)
          {
            std::string elem_str;
            pystring_to_string(element, elem_str);
            BaseConverter::convert_date_time(elem_str.c_str(), (MYSQL_TIME *)buffer, rowbuffer[i].buffer_type);
          } else {
            PyGILState_Release(state);
            throw std::logic_error(
              base::strfmt("Wrong python type for date/time/datetime column %s found in table %s.%s: "
                           "A string or datetime.* object is expected",
                           (*_columns)[i].source_name.c_str(), _schema_name.c_str(), _table_name.c_str()));
          }
        }

        rowbuffer.finish_field(was_null);
        break;
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_BIT:
        unsigned long *length;
        rowbuffer.prepare_add_string(buffer, buffer_len, length);

        if (!was_null) {
          // Target type can be MYSQL_TYPE_STRING for decimal columns and yet values can be ints or floats
          // If that's the case, get str(element) for insertion:
          if (PyFloat_Check(element) || PyLong_Check(element) || PyLong_Check(element)) {
            PyObject *elem_ref = element;
            element = PyObject_Str(element);
            Py_DECREF(elem_ref);
          }

          if (PyUnicode_Check(element)) {
            PyObject *ref = PyUnicode_AsUTF8String(element);
            if (ref) {
              const char *s;
              Py_ssize_t len;
              s = PyUnicode_AsUTF8AndSize(ref, &len);
              if (buffer_len < (size_t)len) {
                logError("Truncating data in column %s from %lul to %lul. Possible loss of data.\n",
                         (*_columns)[i].source_name.c_str(), (long unsigned int)len, (long unsigned int)buffer_len);
                len = buffer_len;
              }
              memcpy(buffer, s, len);
              *length = (unsigned long)len;
              Py_DECREF(ref);
            } else {
              logError("Could not convert unicode string to UTF-8\n");
              PyGILState_Release(state);
              return false;
            }
          } else if (PyUnicode_Check(element)) {
            const char *s;
            Py_ssize_t len;
            s = PyUnicode_AsUTF8AndSize(element, &len);
            if (buffer_len < (size_t)len) {
              logError("Truncating data in column %s from %lul to %lul. Possible loss of data.\n",
                       (*_columns)[i].source_name.c_str(), (long unsigned int)len, (long unsigned int)buffer_len);
              len = buffer_len;
            }
            memcpy(buffer, s, len);
            *length = (unsigned long)len;
          } else // Neither a PyUnicode nor a PyString object. This should be an error:
          {
            logError(
              "The python object for column %s is neither a PyUnicode nor a PyString object. Skipping table...\n",
              (*_columns)[i].source_name.c_str());
            PyGILState_Release(state);
            return false;
          }
        }
        rowbuffer.finish_field(was_null);
        break;
      case MYSQL_TYPE_NULL:
        rowbuffer[i].buffer_length = 0;
        break;
      default:
        Py_DECREF(element);
        PyGILState_Release(state);
        throw std::logic_error(base::strfmt("Unhandled MySQL type %i for column '%s'", (*_columns)[i].target_type,
                                            (*_columns)[i].target_name.c_str()));
    }
    Py_DECREF(element);
  }
  PyGILState_Release(state);
  return true;
}
