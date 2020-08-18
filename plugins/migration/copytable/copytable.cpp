/*
 * Copyright (c) 2012, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <errno.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <cstdlib>
#include <cstdio>

#include <mysql.h>

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/sqlstring.h"

#include "copytable.h"
#include "converter.h"

#undef min

#include <boost/algorithm/string.hpp>

#ifdef __APPLE__
// All the functions in sql.h are deprecated, but we have no replacement atm.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

DEFAULT_LOG_DOMAIN("copytable");

#define TMP_TRIGGER_TABLE "wb_tmp_triggers"

#if defined(MYSQL_VERSION_MAJOR) && defined(MYSQL_VERSION_MINOR) && defined(MYSQL_VERSION_PATCH)
#define MYSQL_CHECK_VERSION(major, minor, micro)                                                         \
  (MYSQL_VERSION_MAJOR > (major) || (MYSQL_VERSION_MAJOR == (major) && MYSQL_VERSION_MINOR > (minor)) || \
   (MYSQL_VERSION_MAJOR == (major) && MYSQL_VERSION_MINOR == (minor) && MYSQL_VERSION_PATCH >= (micro)))
#else
#define MYSQL_CHECK_VERSION(major, minor, micro) false
#endif

// defined in SQL_SS_TIMESTAMPOFFSET and SQL_SS_TIME2 (SQLNCLI.h)
// required Installing SQL Server Native Client 
// https://docs.microsoft.com/en-us/sql/relational-databases/native-client/applications/installing-sql-server-native-client?view=sql-server-ver15
#ifndef SQL_SS_TIMESTAMPOFFSET 
#define SQL_SS_TIMESTAMPOFFSET -155
#endif
#ifndef SQL_SS_TIME2
#define SQL_SS_TIME2 -154
#endif


static const char *mysql_field_type_to_name(enum enum_field_types type) {
  switch (type) {
    case MYSQL_TYPE_DECIMAL:
      return "MYSQL_TYPE_DECIMAL";
    case MYSQL_TYPE_TINY:
      return "MYSQL_TYPE_TINY";
    case MYSQL_TYPE_SHORT:
      return "MYSQL_TYPE_SHORT";
    case MYSQL_TYPE_LONG:
      return "MYSQL_TYPE_LONG";
    case MYSQL_TYPE_FLOAT:
      return "MYSQL_TYPE_FLOAT";
    case MYSQL_TYPE_DOUBLE:
      return "MYSQL_TYPE_DOUBLE";
    case MYSQL_TYPE_NULL:
      return "MYSQL_TYPE_NULL";
    case MYSQL_TYPE_TIMESTAMP:
      return "MYSQL_TYPE_TIMESTAMP";
    case MYSQL_TYPE_LONGLONG:
      return "MYSQL_TYPE_LONGLONG";
    case MYSQL_TYPE_INT24:
      return "MYSQL_TYPE_INT24";
    case MYSQL_TYPE_DATE:
      return "MYSQL_TYPE_DATE";
    case MYSQL_TYPE_TIME:
      return "MYSQL_TYPE_TIME";
    case MYSQL_TYPE_DATETIME:
      return "MYSQL_TYPE_DATETIME";
    case MYSQL_TYPE_YEAR:
      return "MYSQL_TYPE_YEAR";
    case MYSQL_TYPE_NEWDATE:
      return "MYSQL_TYPE_NEWDATE";
    case MYSQL_TYPE_VARCHAR:
      return "MYSQL_TYPE_VARCHAR";
    case MYSQL_TYPE_BIT:
      return "MYSQL_TYPE_BIT";
    case MYSQL_TYPE_NEWDECIMAL:
      return "MYSQL_TYPE_NEWDECIMAL";
    case MYSQL_TYPE_ENUM:
      return "MYSQL_TYPE_ENUM";
    case MYSQL_TYPE_SET:
      return "MYSQL_TYPE_SET";
    case MYSQL_TYPE_TINY_BLOB:
      return "MYSQL_TYPE_TINY_BLOB";
    case MYSQL_TYPE_MEDIUM_BLOB:
      return "MYSQL_TYPE_MEDIUM_BLOB";
    case MYSQL_TYPE_LONG_BLOB:
      return "MYSQL_TYPE_LONG_BLOB";
    case MYSQL_TYPE_BLOB:
      return "MYSQL_TYPE_BLOB";
    case MYSQL_TYPE_VAR_STRING:
      return "MYSQL_TYPE_VAR_STRING";
    case MYSQL_TYPE_STRING:
      return "MYSQL_TYPE_STRING";
    case MYSQL_TYPE_GEOMETRY:
      return "MYSQL_TYPE_GEOMETRY";
    case MYSQL_TYPE_JSON:
      return "MYSQL_TYPE_JSON";
    default:
      return "UNKNOWN";
  }
}

static const char *odbc_type_to_name(SQLSMALLINT type) {
  switch (type) {
    case SQL_CHAR:
      return "SQL_CHAR";
    case SQL_VARCHAR:
      return "SQL_VARCHAR";
    case SQL_LONGVARCHAR:
      return "SQL_LONGVARCHAR";
    case SQL_WCHAR:
      return "SQL_WCHAR";
    case SQL_WVARCHAR:
      return "SQL_WVARCHAR";
    case SQL_WLONGVARCHAR:
      return "SQL_WLONGVARCHAR";
    case SQL_DECIMAL:
      return "SQL_DECIMAL";
    case SQL_NUMERIC:
      return "SQL_NUMERIC";
    case SQL_SMALLINT:
      return "SQL_SMALLINT";
    case SQL_INTEGER:
      return "SQL_INTEGER";
    case SQL_REAL:
      return "SQL_REAL";
    case SQL_FLOAT:
      return "SQL_FLOAT";
    case SQL_DOUBLE:
      return "SQL_DOUBLE";
    case SQL_BIT:
      return "SQL_BIT";
    case SQL_TINYINT:
      return "SQL_TINYINT";
    case SQL_BIGINT:
      return "SQL_BIGINT";
    case SQL_BINARY:
      return "SQL_BINARY";
    case SQL_VARBINARY:
      return "SQL_VARBINARY";
    case SQL_LONGVARBINARY:
      return "SQL_LONGVARBINARY";
    case SQL_TYPE_DATE:
      return "SQL_TYPE_DATE";
    case SQL_TYPE_TIME:
      return "SQL_TYPE_TIME";
    case SQL_TYPE_TIMESTAMP:
      return "SQL_TYPE_TIMESTAMP";
    case SQL_GUID:
      return "SQL_GUID";
    // case SQL_TYPE_UTCDATETIME: return "e SQL_TYPE_UTCDATETIME";
    // case SQL_TYPE_UTCTIME: return "e SQL_TYPE_UTCTIME";
    case SQL_INTERVAL_MONTH:
      return "SQL_INTERVAL_MONTH";
    case SQL_INTERVAL_YEAR:
      return "SQL_INTERVAL_YEAR";
    case SQL_INTERVAL_YEAR_TO_MONTH:
      return "SQL_INTERVAL_YEAR_TO_MONTH";
    case SQL_INTERVAL_DAY:
      return "SQL_INTERVAL_DAY";
    case SQL_INTERVAL_HOUR:
      return "SQL_INTERVAL_HOUR";
    case SQL_INTERVAL_MINUTE:
      return "SQL_INTERVAL_MINUTE";
    case SQL_INTERVAL_SECOND:
      return "SQL_INTERVAL_SECOND";
    case SQL_INTERVAL_DAY_TO_HOUR:
      return "SQL_INTERVAL_DAY_TO_HOUR";
    case SQL_INTERVAL_DAY_TO_MINUTE:
      return "SQL_INTERVAL_DAY_TO_MINUTE";
    case SQL_INTERVAL_DAY_TO_SECOND:
      return "SQL_INTERVAL_DAY_TO_SECOND";
    case SQL_INTERVAL_HOUR_TO_MINUTE:
      return "SQL_INTERVAL_HOUR_TO_MINUTE";
    case SQL_INTERVAL_HOUR_TO_SECOND:
      return "SQL_INTERVAL_HOUR_TO_SECOND";
    case SQL_INTERVAL_MINUTE_TO_SECOND:
      return "SQL_INTERVAL_MINUTE_TO_SECOND";
    case SQL_SS_TIMESTAMPOFFSET:
      return "SQL_SS_TIMESTAMPOFFSET";
    case SQL_SS_TIME2:
      return "SQL_SS_TIME2";
    default:
      return "UNKNOWN";
  }
}

std::string QueryBuilder::build_query() {
  std::string q;
  std::string where_cond;
  for (size_t i = 0; i < this->_where.size(); ++i) {
    if (i > 0)
      where_cond += " AND ";
    where_cond += base::strfmt("(%s)", this->_where[i].c_str());
  }

  if (this->_schema.empty())
    q = base::strfmt("SELECT %s FROM %s", this->_columns.c_str(), this->_table.c_str());
  else
    q = base::strfmt("SELECT %s FROM %s.%s", this->_columns.c_str(), this->_schema.c_str(), this->_table.c_str());

  if (!where_cond.empty())
    q += base::strfmt(" WHERE %s", +where_cond.c_str());
  if (!this->_orderby.empty())
    q += base::strfmt(" ORDER BY %s", +this->_orderby.c_str());
  if (!this->_limit.empty())
    q += base::strfmt(" LIMIT %s", +this->_limit.c_str());

  return q;
}

std::string ConnectionError::process(SQLRETURN retcode, SQLSMALLINT htype, SQLHANDLE handle) {
  SQLINTEGER i = 0;
  SQLINTEGER native;
  SQLCHAR state[7];
  SQLCHAR text[256];
  SQLSMALLINT len;
  SQLRETURN ret;
  std::string output;

  if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
    do {
      ret = SQLGetDiagRec(htype, handle, (SQLSMALLINT)++i, state, &native, text, sizeof(text), &len);
      if (SQL_SUCCEEDED(ret)) {
        char is[32];
        char natives[32];
        snprintf(is, sizeof(is), "%li", (long)i);
        snprintf(natives, sizeof(natives), "%li", (long)native);
        output.append((char *)state)
          .append(":")
          .append(is)
          .append(natives)
          .append(":")
          .append((char *)text)
          .append("\n");
      }
    } while (ret == SQL_SUCCESS);
  } else {
    output = base::strfmt("Error %i", retcode);
  }

  return output;
}

RowBuffer::RowBuffer(std::shared_ptr<std::vector<ColumnInfo> > columns,
                     std::function<void(int, const char *, size_t)> send_blob_data, size_t max_packet_size)
  : _current_field(0), _send_blob_data(send_blob_data) {
  for (std::vector<ColumnInfo>::const_iterator col = columns->begin(); col != columns->end(); ++col) {
    MYSQL_BIND bind;
    memset(&bind, 0, sizeof(bind));

    bind.buffer_type = col->target_type;
    // Only the PS data types are handled here
    switch (col->target_type) {
      case MYSQL_TYPE_TINY:
        bind.buffer_length = sizeof(char);
        break;
      case MYSQL_TYPE_YEAR:
      case MYSQL_TYPE_SHORT:
        bind.buffer_length = sizeof(short);
        break;
      case MYSQL_TYPE_INT24:
      case MYSQL_TYPE_LONG:
        bind.buffer_length = sizeof(int);
        break;
      case MYSQL_TYPE_LONGLONG:
        bind.buffer_length = sizeof(long long int);
        break;
      case MYSQL_TYPE_FLOAT:
        bind.buffer_length = sizeof(float);
        break;
      case MYSQL_TYPE_DOUBLE:
        bind.buffer_length = sizeof(double);
        break;
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_NEWDATE:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_TIMESTAMP:
        bind.buffer_length = sizeof(MYSQL_TIME);
        break;
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_BIT:
      case MYSQL_TYPE_JSON:
        if (!col->is_long_data)
          bind.buffer_length = (unsigned)col->source_length + 1;

        bind.length = (unsigned long *)malloc(sizeof(unsigned long));
        if (!bind.length)
          throw std::runtime_error("Could not allocate memory for row buffer");
        break;
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_GEOMETRY:
        // source_length is not reliable (and returns bogus value for access)
        // so we just use the max_packet_size value
        bind.buffer_length = (unsigned long)std::min(max_packet_size, (size_t)col->source_length + 1);
        bind.length = (unsigned long *)malloc(sizeof(unsigned long));
        if (!bind.length)
          throw std::runtime_error("Could not allocate memory for row buffer");
        break;
      case MYSQL_TYPE_NULL:
        bind.buffer_length = 0;
        break;
      default:
        throw std::logic_error(
          base::strfmt("Unhandled MySQL type %i for column '%s'", col->target_type, col->target_name.c_str()));
    }

#if MYSQL_VERSION_ID >= 80004
    typedef bool WB_BOOL;
#else
    typedef my_bool WB_BOOL;
#endif

    bind.error = (WB_BOOL *)malloc(sizeof(WB_BOOL));
    if (!bind.error)
      throw std::runtime_error("Could not allocate memory for row buffer");
    if (col->target_type != MYSQL_TYPE_NULL) {
      bind.is_null = (WB_BOOL *)malloc(sizeof(WB_BOOL));
      if (!bind.is_null) {
        if (bind.length) {
          free(bind.length);
          bind.length = NULL;
        }
        throw std::runtime_error("Could not allocate row buffer");
      }
    }
    if (bind.buffer_length > 0) {
      bind.buffer = malloc(bind.buffer_length);

      if (!bind.buffer) {
        if (bind.error) {
          free(bind.error);
          bind.error = NULL;
        }
        {
          free(bind.is_null);
          bind.is_null = NULL;
        }
        throw std::runtime_error(base::strfmt("Could not allocate %lu bytes for row buffer column of %s %s %i",
                                              bind.buffer_length, col->source_name.c_str(), col->source_type.c_str(),
                                              col->target_type));
      }
    } else
      bind.buffer = 0;
    bind.is_unsigned = col->is_unsigned;

    push_back(bind);
  }
}

RowBuffer::~RowBuffer() {
  for (std::vector<MYSQL_BIND>::iterator field = begin(); field != end(); ++field) {
    if (field->buffer)
      free(field->buffer);
    if (field->length)
      free(field->length);
    if (field->is_null)
      free(field->is_null);
    if (field->error)
      free(field->error);
  }
}

void RowBuffer::clear() {
  _current_field = 0;
}

void RowBuffer::prepare_add_string(char *&buffer, size_t &buffer_len, unsigned long *&length) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_STRING)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be string, was %s)",
                                        _current_field + 1, mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
  length = bind.length;
}

void RowBuffer::prepare_add_float(char *&buffer, size_t &buffer_len) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_FLOAT)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be float, was %s)", _current_field + 1,
                                        mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
}

void RowBuffer::prepare_add_double(char *&buffer, size_t &buffer_len) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_DOUBLE)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be double, was %s)",
                                        _current_field + 1, mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
}

void RowBuffer::prepare_add_bigint(char *&buffer, size_t &buffer_len) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_LONGLONG)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be bigint, was %s)",
                                        _current_field + 1, mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
}

void RowBuffer::prepare_add_long(char *&buffer, size_t &buffer_len) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_LONG)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be long, was %s)", _current_field + 1,
                                        mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
}

void RowBuffer::prepare_add_short(char *&buffer, size_t &buffer_len) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_SHORT)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be short, was %s)", _current_field + 1,
                                        mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
}

void RowBuffer::prepare_add_tiny(char *&buffer, size_t &buffer_len) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_TINY)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be char, was %s)", _current_field + 1,
                                        mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
}

void RowBuffer::prepare_add_time(char *&buffer, size_t &buffer_len) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_DATETIME && bind.buffer_type != MYSQL_TYPE_TIMESTAMP &&
      bind.buffer_type != MYSQL_TYPE_TIME && bind.buffer_type != MYSQL_TYPE_DATE &&
      bind.buffer_type != MYSQL_TYPE_NEWDATE)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be time, was %s)", _current_field + 1,
                                        mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
}

void RowBuffer::prepare_add_geometry(char *&buffer, size_t &buffer_len, unsigned long *&length) {
  MYSQL_BIND &bind(at(_current_field));
  if (bind.buffer_type != MYSQL_TYPE_GEOMETRY)
    throw std::logic_error(base::strfmt("Type mismatch fetching field %i (should be geometry, was %s)",
                                        _current_field + 1, mysql_field_type_to_name(bind.buffer_type)));

  buffer = (char *)bind.buffer;
  buffer_len = bind.buffer_length;
  length = bind.length;
}

void RowBuffer::finish_field(bool was_null) {
  *at(_current_field).is_null = was_null;

  _current_field++;
}

bool RowBuffer::check_if_blob() {
  if (at(_current_field).buffer_type == MYSQL_TYPE_BLOB)
    return true;
  return false;
}

enum enum_field_types RowBuffer::target_type(bool &unsig) {
  unsig = at(_current_field).is_unsigned != 0;
  return at(_current_field).buffer_type;
}

void RowBuffer::send_blob_data(const char *data, size_t length) {
  _send_blob_data(_current_field, data, length);
}

// -------------------------------------------------------------------------------------------------

CopyDataSource::CopyDataSource()
  : _block_size(0),
    _max_blob_chunk_size(64 * 1024),
    _max_parameter_size(0),
    _abort_on_oversized_blobs(false),
    _use_bulk_inserts(false),
    _get_field_lengths_from_target(false),
    _connection_timeout(0)
{
}

void CopyDataSource::set_max_blob_chunk_size(size_t size) {
  _max_blob_chunk_size = size;
  if (_blob_buffer.size() < size)
    _blob_buffer.resize(size);
}

void CopyDataSource::set_block_size(int bsize) {
  _block_size = bsize;
}

/*
 * get_where_condition : creates where condition for --resume parameter.
 * Parameters:
 * - pk_columns : vector of PK columns
 * - last_pk : vector of last PK value for each of PK column
 *
 * Remarks : For these columns and values ​​creates a condition to the WHERE clause
 *           to skip the rows that have already been copied.
 *           For one columns will produce:
 *             col1 > val1
 *           For two columns:
 *             col1 > val1 or (col1 = val1 and col2 > val2)
 *           For three columns:
 *             col1 > val1 or (col1 = val1 and col2 > val2) or (col1 = val1 and col2 = val2 and col3 > val3)
 *           And so on...
 */
std::string CopyDataSource::get_where_condition(const std::vector<std::string> &pk_columns,
                                                const std::vector<std::string> &last_pk) {
  std::string where_cond;
  bool add_and = false;

  for (size_t i = 0; i < pk_columns.size(); ++i) {
    add_and = false;
    for (unsigned int j = 0; j < i; ++j) {
      add_and = true;
      if (j == 0)
        where_cond += " or (";
      else
        where_cond += " and ";
      where_cond += base::strfmt("%s = '%s'", pk_columns[j].c_str(), base::escape_sql_string(last_pk[j]).c_str());
    }
    if (add_and)
      where_cond += " and ";
    where_cond += base::strfmt("%s > '%s'", pk_columns[i].c_str(), base::escape_sql_string(last_pk[i]).c_str());
    if (add_and)
      where_cond += ")";
  }

  return where_cond;
}

// -------------------------------------------------------------------------------------------------

SQLSMALLINT ODBCCopyDataSource::odbc_type_to_c_type(SQLSMALLINT type, bool is_unsigned) {
  switch (type) {
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
      return SQL_C_CHAR;
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
      // FreeTDS converts the data to utf8
      return _force_utf8_input ? SQL_C_CHAR : SQL_C_WCHAR;
    case SQL_DECIMAL:
    case SQL_NUMERIC:
      return SQL_C_DOUBLE;
    case SQL_SMALLINT:
      return is_unsigned ? SQL_C_USHORT : SQL_C_SSHORT;
    case SQL_INTEGER:
      return is_unsigned ? SQL_C_ULONG : SQL_C_SLONG;
    case SQL_REAL:
      return SQL_C_FLOAT;
    case SQL_FLOAT:
      return SQL_C_FLOAT;
    case SQL_DOUBLE:
      return SQL_C_DOUBLE;
    case SQL_BIT:
      return SQL_C_BIT;
    case SQL_TINYINT:
      return is_unsigned ? SQL_C_UTINYINT : SQL_C_STINYINT;
    case SQL_BIGINT:
      return is_unsigned ? SQL_C_UBIGINT : SQL_C_SBIGINT;
    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
      return SQL_C_BINARY;
    case SQL_TYPE_DATE:
      return SQL_C_DATE;
    case SQL_TYPE_TIME:
      return SQL_C_TIME;
    case SQL_TYPE_TIMESTAMP:
      return SQL_C_TIMESTAMP;
    case SQL_GUID:
      return SQL_C_CHAR;
    // case SQL_TYPE_UTCDATETIME:
    // case SQL_TYPE_UTCTIME:
    case SQL_SS_TIMESTAMPOFFSET:
      logWarning("Not supported type [%s]\n", odbc_type_to_name(type));
      return _force_utf8_input ? SQL_C_CHAR : SQL_C_WCHAR;
    case SQL_SS_TIME2:
      return _force_utf8_input ? SQL_C_CHAR : SQL_C_WCHAR;
    case SQL_INTERVAL_MONTH:
    case SQL_INTERVAL_YEAR:
    case SQL_INTERVAL_YEAR_TO_MONTH:
    case SQL_INTERVAL_DAY:
    case SQL_INTERVAL_HOUR:
    case SQL_INTERVAL_MINUTE:
    case SQL_INTERVAL_SECOND:
    case SQL_INTERVAL_DAY_TO_HOUR:
    case SQL_INTERVAL_DAY_TO_MINUTE:
    case SQL_INTERVAL_DAY_TO_SECOND:
    case SQL_INTERVAL_HOUR_TO_MINUTE:
    case SQL_INTERVAL_HOUR_TO_SECOND:
    case SQL_INTERVAL_MINUTE_TO_SECOND:
    default:
      // convert to string and make the mysql side also auto-convert
      throw std::runtime_error(base::strfmt("Unhandled type %i", type));
      //// default just convert to string
      // return SQL_C_CHAR;
  }
}

ODBCCopyDataSource::ODBCCopyDataSource(SQLHENV env, const std::string &connstring, const std::string &password,
                                       bool force_utf8_input, const std::string &source_rdbms_type)
  : _connstring(connstring), _stmt(nullptr), _stmt_ok(false), _column_count(0), _source_rdbms_type(source_rdbms_type) {
  _blob_buffer = std::vector<char>(_max_blob_chunk_size);

  _force_utf8_input = force_utf8_input;

  SQLAllocHandle(SQL_HANDLE_DBC, env, &_dbc);

  // 5s timeout
  SQLSetConnectAttr(_dbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

  bool has_pwd = _connstring.find("PWD=") != std::string::npos;
  if (!has_pwd)
    _connstring.append(";PWD=");
  logInfo("Opening ODBC connection to [%s] '%s'\n", _source_rdbms_type.c_str(),
          (_connstring + (has_pwd ? "" : "XXX")).c_str());

  SQLRETURN ret = SQLDriverConnect(_dbc, NULL, (SQLCHAR *)(_connstring + (has_pwd ? "" : password)).c_str(), SQL_NTS,
                                   NULL, 0, NULL, SQL_DRIVER_COMPLETE);
  if (!SQL_SUCCEEDED(ret)) {
    logError("ODBC connection to '%s' failed\n", _connstring.c_str());
    throw ConnectionError("SQLDriverConnect", ret, SQL_HANDLE_DBC, _dbc);
  } else
    logInfo("ODBC connection to '%s' opened\n", _connstring.c_str());
}

ODBCCopyDataSource::~ODBCCopyDataSource() {
  if (_stmt_ok)
    SQLFreeHandle(SQL_HANDLE_ENV, _stmt);
  SQLFreeHandle(SQL_HANDLE_DBC, _dbc);
}

SQLRETURN ODBCCopyDataSource::get_wchar_buffer_data(RowBuffer &rowbuffer, int column) {
  unsigned long *out_length = NULL;
  SQLLEN len_or_indicator = 0;
  char *out_buffer = NULL;
  size_t out_buffer_len = 0;
  wchar_t tmpbuf[64 * 1024] = {0};

  if (typeid(wchar_t *) != typeid(SQLWCHAR *))
    logWarning(
      "Forcing wchar_t but SQLWCHAR is of different type which shouldn't happen. Potential problems during migration "
      "may occur.\n");

  SQLRETURN ret = SQLGetData(_stmt, column, _column_types[column - 1], tmpbuf, sizeof(tmpbuf), &len_or_indicator);
  // check if the data fits
  // if (len_or_indicator > out_buffer_len)
  //  ;
  rowbuffer.prepare_add_string(out_buffer, out_buffer_len, out_length);
  memset(out_buffer, 0, out_buffer_len);
  if (SQL_SUCCEEDED(ret)) {
    if (len_or_indicator == SQL_NO_TOTAL)
      throw std::runtime_error(base::strfmt("Got SQL_NO_TOTAL for string size during copy of column %i", column));

    if (len_or_indicator != SQL_NULL_DATA) {
      size_t outbuf_len = out_buffer_len;

      // convert data from UCS-2 to utf-8
      std::string s_outbuf = base::wstring_to_string(tmpbuf);

      outbuf_len = s_outbuf.size();
      if (outbuf_len > _max_blob_chunk_size - 1)
        throw std::logic_error("Output buffer size is greater than max blob chunk size.");

      // The following lengths are valid as length/indicator values:
      // - n, where n > 0,
      // - 0
      // - SQL_NTS. A string sent to the driver in the corresponding data buffer is null-terminated; this is a
      // convenient
      //            way for C programmers to pass strings without having to calculate their byte length.
      //            This value is legal only when the application sends data to the driver.
      if (s_outbuf.empty() && len_or_indicator > 0)
        throw std::logic_error(base::strfmt("Error during charset conversion of wstring: %s", strerror(errno)));

      if (len_or_indicator > 0)
        std::strcpy(out_buffer, s_outbuf.c_str());
      *out_length = (unsigned long)outbuf_len;
    }
    rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
  }
  return ret;
}

SQLRETURN ODBCCopyDataSource::get_date_time_data(RowBuffer &rowbuffer, int column, int type) {
  SQLRETURN ret;
  char *out_buffer;
  SQLLEN len_or_indicator;
  size_t out_buffer_len;
  char out_date[256] = { 0 };

  rowbuffer.prepare_add_time(out_buffer, out_buffer_len);
  ret = SQLGetData(_stmt, column, SQL_C_CHAR, &out_date, sizeof(out_date), &len_or_indicator);
  if (SQL_SUCCEEDED(ret)) {
    // When driver cannot determine the number of bytes of long data
    // still available to return in an output buffer it return SQL_NO_TOTAL
    if (len_or_indicator == SQL_NO_TOTAL)
      throw std::runtime_error(base::strfmt("Got SQL_NO_TOTAL for string size during copy of column %i", column));

    if (len_or_indicator != SQL_NULL_DATA)
      BaseConverter::convert_date_time(out_date, (MYSQL_TIME *)out_buffer, type);
    else
      ((MYSQL_TIME *)out_buffer)->time_type = MYSQL_TIMESTAMP_NONE;

    rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
  }

  return ret;
}

SQLRETURN ODBCCopyDataSource::get_char_buffer_data(RowBuffer &rowbuffer, int column) {
  unsigned long *out_length;
  SQLRETURN ret;
  SQLLEN len_or_indicator;
  char *out_buffer;
  size_t out_buffer_len;

  rowbuffer.prepare_add_string(out_buffer, out_buffer_len, out_length);
  ret = SQLGetData(_stmt, column, _column_types[column - 1], out_buffer, out_buffer_len, &len_or_indicator);
  // check if the data fits
  // if (len_or_indicator > out_buffer_len)
  //  ;
  if (len_or_indicator == SQL_NO_TOTAL)
    throw std::runtime_error(base::strfmt("Got SQL_NO_TOTAL for string size during copy of column %i", column));

  if (SQL_SUCCEEDED(ret)) {
    if (len_or_indicator != SQL_NULL_DATA)
      *out_length = (unsigned long)len_or_indicator;
    rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
  }
  return ret;
}

SQLRETURN ODBCCopyDataSource::get_geometry_buffer_data(RowBuffer &rowbuffer, int column) {
  unsigned long *out_length = NULL;
  SQLLEN len_or_indicator = 0;
  char *out_buffer = NULL;
  size_t out_buffer_len = 0;
  wchar_t tmpbuf[64 * 1024] = {0};

  if (typeid(wchar_t *) != typeid(SQLWCHAR *))
    logWarning(
      "Forcing wchar_t but SQLWCHAR is of different type which shouldn't happen. Potential problems during migration "
      "may occur.\n");

  SQLRETURN ret = SQLGetData(_stmt, column, SQL_C_WCHAR, tmpbuf, sizeof(tmpbuf), &len_or_indicator);

  rowbuffer.prepare_add_geometry(out_buffer, out_buffer_len, out_length);
  memset(out_buffer, 0, out_buffer_len);
  if (SQL_SUCCEEDED(ret)) {
    if (len_or_indicator == SQL_NO_TOTAL)
      throw std::runtime_error(base::strfmt("Got SQL_NO_TOTAL for string size during copy of column %i", column));

    if (len_or_indicator != SQL_NULL_DATA) {
      size_t outbuf_len = out_buffer_len;

      // convert data from UCS-2 to utf-8
      std::string s_outbuf = base::wstring_to_string(tmpbuf);
      outbuf_len = s_outbuf.size();
      if (outbuf_len > _max_blob_chunk_size - 1)
        throw std::logic_error("Output buffer size is greater than max blob chunk size.");

      if (s_outbuf.empty() && len_or_indicator > 0)
        throw std::logic_error(base::strfmt("Error during charset conversion of wstring: %s", strerror(errno)));

      if (len_or_indicator)
        std::strcpy(out_buffer, s_outbuf.c_str());

      *out_length = (unsigned long)outbuf_len;
    }
    rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
  }
  return ret;
}

size_t ODBCCopyDataSource::count_rows(const std::string &schema, const std::string &table,
                                      const std::vector<std::string> &pk_columns, const CopySpec &spec,
                                      const std::vector<std::string> &last_pkeys) {
  SQLHSTMT stmt;
  SQLRETURN ret;
  if (!SQL_SUCCEEDED(ret = SQLAllocHandle(SQL_HANDLE_STMT, _dbc, &stmt)))
    throw ConnectionError("SQLAllocHandle", ret, SQL_HANDLE_DBC, _dbc);

  QueryBuilder q;
  q.select_columns(base::strfmt("%s(*)", _source_rdbms_type == "Mssql" ? "count_big" : "count"));
  q.select_from_table(table, schema);

  switch (spec.type) {
    case CopyAll:
      if (spec.resume && last_pkeys.size())
        q.add_where(get_where_condition(pk_columns, last_pkeys));
      break;
    case CopyRange: {
      std::string start_expr, end_expr;
      if (spec.range_end < 0)
        end_expr = "";
      else
        end_expr = base::strfmt("%s <= %lli", spec.range_key.c_str(), spec.range_end);
      start_expr = base::strfmt("%s >= %lli", spec.range_key.c_str(), spec.range_start);
      if (!end_expr.empty())
        q.add_where(base::strfmt("%s AND %s", start_expr.c_str(), end_expr.c_str()));
      else
        q.add_where(start_expr);
      break;
    }
    case CopyCount: {
      if (spec.resume && last_pkeys.size())
        q.add_where(get_where_condition(pk_columns, last_pkeys));
      break;
    }
    case CopyWhere: {
      q.add_where(spec.where_expression);
      break;
    }
  }

  logDebug("Executing query: %s\n", q.build_query().c_str());
  if (!SQL_SUCCEEDED(ret = SQLExecDirect(stmt, (SQLCHAR *)q.build_query().c_str(), SQL_NTS)))
    throw ConnectionError("SQLExecDirect(" + q.build_query() + ")", ret, SQL_HANDLE_STMT, stmt);

  long long count = 0;
  if (SQL_SUCCEEDED(SQLFetch(stmt)))
    SQLGetData(stmt, 1, SQL_C_ULONG, &count, sizeof(count), NULL);

  SQLFreeHandle(SQL_HANDLE_STMT, stmt);

  if ((spec.type == CopyAll || spec.type == CopyWhere) && spec.max_count > 0 && spec.max_count < count)
    count = spec.max_count;

  return (size_t)count;
}

std::shared_ptr<std::vector<ColumnInfo> > ODBCCopyDataSource::begin_select_table(
  const std::string &schema, const std::string &table, const std::vector<std::string> &pk_columns,
  const std::string &select_expression, const CopySpec &spec, const std::vector<std::string> &last_pkeys) {
  std::shared_ptr<std::vector<ColumnInfo> > columns(new std::vector<ColumnInfo>());
  _columns = columns;
  _schema_name = schema;
  _table_name = table;

  _stmt_ok = true;
  SQLRETURN ret;
  if (!SQL_SUCCEEDED(ret = SQLAllocHandle(SQL_HANDLE_STMT, _dbc, &_stmt)))
    throw ConnectionError("SQLAllocHandle", ret, SQL_HANDLE_DBC, _dbc);

  std::string q;

  QueryBuilder select_query;
  select_query.select_columns(select_expression);
  select_query.select_from_table(table, schema);
  select_query.add_orderby(boost::algorithm::join(pk_columns, ", "));

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

  logDebug("Executing query: %s\n", q.c_str());
  if (!SQL_SUCCEEDED(ret = SQLExecDirect(_stmt, (SQLCHAR *)q.c_str(), SQL_NTS)))
    throw ConnectionError("SQLExecDirect(" + q + ")", ret, SQL_HANDLE_STMT, _stmt);

  SQLSMALLINT column_count;
  if (!SQL_SUCCEEDED(ret = SQLNumResultCols(_stmt, &column_count)))
    throw ConnectionError("SQLNumResultCols", ret, SQL_HANDLE_STMT, _stmt);
  _column_count = column_count;

  logDebug2("Columns from source table %s.%s (%i):\n", schema.c_str(), table.c_str(), column_count);
  for (int i = 1; i <= column_count; i++) {
    ColumnInfo info;
    SQLCHAR columnName[256];
    SQLSMALLINT nameLength;
    SQLSMALLINT dataType;
    SQLULEN columnSize;
    SQLSMALLINT decimalDigits;
    SQLSMALLINT nullablePtr;

    info.is_long_data = false;
    info.is_unsigned = false;

    if (SQL_SUCCEEDED(SQLDescribeCol(_stmt, i, columnName, sizeof(columnName), &nameLength, &dataType, &columnSize,
                                     &decimalDigits, &nullablePtr))) {
      bool is_unsigned = false;
      SQLCHAR typeName[256];
      SQLSMALLINT typeNameLength;

      SQLLEN attrvalue;
      if (SQL_SUCCEEDED(SQLColAttribute(_stmt, i, SQL_DESC_UNSIGNED, NULL, 0, NULL, &attrvalue)))
        is_unsigned = attrvalue == SQL_TRUE;

      if (SQL_SUCCEEDED(
            SQLColAttribute(_stmt, i, SQL_DESC_TYPE_NAME, typeName, sizeof(typeName), &typeNameLength, NULL)))
        info.source_type = std::string((char *)typeName, typeNameLength);

      info.source_name = (char *)columnName;
      info.source_length = columnSize;
      if (dataType == SQL_WCHAR || dataType == SQL_WLONGVARCHAR || dataType == SQL_WVARCHAR)
        info.source_length *= 4;

      if (dataType == SQL_LONGVARBINARY || dataType == SQL_LONGVARCHAR || dataType == SQL_WLONGVARCHAR)
        info.is_long_data = true;

      // TODO find out the MySQL equivalent of the type and fill here.. if its not used, then remove
      // info.mapped_source_type = dataType;
      info.is_unsigned = is_unsigned;

      logDebug2("%i - %s: %s %s (type=%s, len=%lli%s)\n", i, columnName, typeName, is_unsigned ? "UNSIGNED" : "",
                odbc_type_to_name(dataType), info.source_length, info.is_long_data ? ", long_data" : "");

      columns->push_back(info);

      _column_types.push_back(odbc_type_to_c_type(dataType, is_unsigned));
    } else
      throw ConnectionError("SQLDescribeCol", ret, SQL_HANDLE_STMT, _stmt);
  }

  return columns;
}

void ODBCCopyDataSource::end_select_table() {
  SQLFreeHandle(SQL_HANDLE_STMT, _stmt);
  _column_types.clear();
  _columns.reset();
  _stmt_ok = false;
}

bool ODBCCopyDataSource::fetch_row(RowBuffer &rowbuffer) {
  if (SQL_SUCCEEDED(SQLFetch(_stmt))) {
    for (int i = 1; i <= _column_count; i++) {
      SQLRETURN ret = 0;
      SQLLEN len_or_indicator;
      char *out_buffer;
      size_t out_buffer_len;

      // if this column is a blob, handle it as such
      if (rowbuffer.check_if_blob() || (*_columns)[i - 1].is_long_data) {
        ret = SQLGetData(_stmt, i, _column_types[i - 1], _blob_buffer.data(), _max_blob_chunk_size, &len_or_indicator);

        // Saves the column length, at the first call it is the total column size
        if (len_or_indicator > _max_parameter_size) {
          if (_abort_on_oversized_blobs)
            throw std::runtime_error(base::strfmt("oversized blob found in table %s.%s, size: %lli",
                                                  _schema_name.c_str(), _table_name.c_str(),
                                                  (long long)len_or_indicator));
          else {
            printf("oversized blob found in table %s.%s, size: %lli", _schema_name.c_str(), _table_name.c_str(),
                   (long long)len_or_indicator);
            rowbuffer.finish_field(true);
            continue;
          }
        } else {
          while (ret == SQL_SUCCESS_WITH_INFO) {
            SQLUSMALLINT i = 0;
            SQLINTEGER native;
            SQLCHAR state[7];
            SQLCHAR text[256];
            SQLSMALLINT len;

            ret = SQLGetDiagRec(SQL_HANDLE_STMT, _stmt, ++i, state, &native, text, sizeof(text), &len);

            // This should be done ONLY if no bulk updates
            // are being used
            if (native == 1014 && !_use_bulk_inserts)
              rowbuffer.send_blob_data(_blob_buffer.data(), len_or_indicator);

            // Unrecognized characters were changed to ?? but data was read
            else if (native == 2403) {
              logWarning("[%s - %ld]: %s\n", state, (long int)native, text);
              break;
            }

            ret =
              SQLGetData(_stmt, i, _column_types[i - 1], _blob_buffer.data(), _max_blob_chunk_size, &len_or_indicator);
          }

          if (ret == SQL_SUCCESS) {
            bool was_null = len_or_indicator == SQL_NULL_DATA;

            if (!was_null) {
              char *final_data = _blob_buffer.data();
              size_t final_length = len_or_indicator;

              // Convers the data to utf8 if needed
              if (_column_types[i - 1] == SQL_C_WCHAR && len_or_indicator > 0) {
                std::string outbuf = base::wstring_to_string((wchar_t *)_blob_buffer.data());
                // TODO take care of case where the utf8 data is bigger than _max_blob_chunk_size
                if (outbuf.size() > _max_blob_chunk_size - 1)
                  throw std::logic_error("Output buffer size is greater than max blob chunk size.");
                std::fill(_blob_buffer.begin(), _blob_buffer.end(), 0);
                std::strcpy(_blob_buffer.data(), outbuf.c_str());
                final_length = outbuf.size();
              }

              if (_use_bulk_inserts) {
                if (rowbuffer[i - 1].buffer_length)
                  free(rowbuffer[i - 1].buffer);

                *rowbuffer[i - 1].length = (unsigned long)final_length;
                rowbuffer[i - 1].buffer_length = (unsigned long)final_length;
                rowbuffer[i - 1].buffer = malloc(final_length);

                memcpy(rowbuffer[i - 1].buffer, final_data, final_length);
              } else
                rowbuffer.send_blob_data(final_data, final_length);
            }

            rowbuffer.finish_field(was_null);
          } else {
            rowbuffer.finish_field(true);
            throw ConnectionError("SQLGetData", ret, SQL_HANDLE_STMT, _stmt);
          }
          continue;
        }
      }

      switch (_column_types[i - 1]) {
        case SQL_C_BIT:
          rowbuffer.prepare_add_tiny(out_buffer, out_buffer_len);
          ret = SQLGetData(_stmt, i, SQL_C_STINYINT, out_buffer, out_buffer_len, &len_or_indicator);
          if (SQL_SUCCEEDED(ret))
            rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
          break;
        case SQL_C_FLOAT:
        case SQL_C_DOUBLE:
          if (rowbuffer[i - 1].buffer_type == MYSQL_TYPE_FLOAT) {
            rowbuffer.prepare_add_float(out_buffer, out_buffer_len);
            ret = SQLGetData(_stmt, i, SQL_C_FLOAT, out_buffer, out_buffer_len, &len_or_indicator);
            if (SQL_SUCCEEDED(ret))
              rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
           } else if (rowbuffer[i - 1].buffer_type == MYSQL_TYPE_STRING) {
              if (_column_types[i - 1] == SQL_C_WCHAR)
                ret = get_wchar_buffer_data(rowbuffer, i);
              else
                ret = get_char_buffer_data(rowbuffer, i);
          } else {
            rowbuffer.prepare_add_double(out_buffer, out_buffer_len);
            ret = SQLGetData(_stmt, i, SQL_C_DOUBLE, out_buffer, out_buffer_len, &len_or_indicator);
            if (SQL_SUCCEEDED(ret))
              rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
          }
          break;
        case SQL_C_DATE:
          ret = get_date_time_data(rowbuffer, i, MYSQL_TYPE_DATE);
          break;
        case SQL_C_TIME:
          ret = get_date_time_data(rowbuffer, i, MYSQL_TYPE_TIME);
          break;
        case SQL_C_TIMESTAMP:
          ret = get_date_time_data(rowbuffer, i, MYSQL_TYPE_TIMESTAMP);
          break;
        case SQL_C_UBIGINT:
        case SQL_C_SBIGINT:
          rowbuffer.prepare_add_bigint(out_buffer, out_buffer_len);
          ret = SQLGetData(_stmt, i, _column_types[i - 1], out_buffer, out_buffer_len, &len_or_indicator);
          if (SQL_SUCCEEDED(ret))
            rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
          break;
        case SQL_C_ULONG:
        case SQL_C_SLONG: {
          long tmp_buffer;
          bool unsig;
          enum enum_field_types target_type;
          ret = SQLGetData(_stmt, i, _column_types[i - 1], &tmp_buffer, sizeof(tmp_buffer), &len_or_indicator);
          if (SQL_SUCCEEDED(ret)) {
            switch ((target_type = rowbuffer.target_type(unsig))) {
              case MYSQL_TYPE_SHORT:
                rowbuffer.prepare_add_short(out_buffer, out_buffer_len);
                if ((unsig && (tmp_buffer < 0 || tmp_buffer > UINT16_MAX)) ||
                    (!unsig && (tmp_buffer > INT16_MAX || tmp_buffer < INT16_MIN)))
                  throw std::logic_error(base::strfmt("Range error fetching field %i (value %li, target is %s)", i,
                                                      tmp_buffer, mysql_field_type_to_name(target_type)));
                *(short *)out_buffer = (short)tmp_buffer;
                break;
              case MYSQL_TYPE_TINY:
                rowbuffer.prepare_add_tiny(out_buffer, out_buffer_len);
                if ((unsig && (tmp_buffer < 0 || tmp_buffer > UINT8_MAX)) ||
                    (!unsig && (tmp_buffer > INT8_MAX || tmp_buffer < INT8_MIN)))
                  throw std::logic_error(base::strfmt("Range error fetching field %i (value %li, target is %s)", i,
                                                      tmp_buffer, mysql_field_type_to_name(target_type)));
                *(char *)out_buffer = (char)tmp_buffer;
                break;
              default:
                rowbuffer.prepare_add_long(out_buffer, out_buffer_len);
                *(long *)out_buffer = tmp_buffer;
                break;
            }
            rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
          }
          break;
        }
        case SQL_C_USHORT:
        case SQL_C_SSHORT:
          rowbuffer.prepare_add_short(out_buffer, out_buffer_len);
          ret = SQLGetData(_stmt, i, _column_types[i - 1], out_buffer, out_buffer_len, &len_or_indicator);
          if (SQL_SUCCEEDED(ret))
            rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
          break;
        case SQL_C_UTINYINT:
        case SQL_C_STINYINT:
          rowbuffer.prepare_add_tiny(out_buffer, out_buffer_len);
          ret = SQLGetData(_stmt, i, _column_types[i - 1], out_buffer, out_buffer_len, &len_or_indicator);
          if (SQL_SUCCEEDED(ret))
            rowbuffer.finish_field(len_or_indicator == SQL_NULL_DATA);
          break;
        case SQL_C_WCHAR:
        case SQL_C_CHAR: {
          switch (rowbuffer[i - 1].buffer_type) {
            case MYSQL_TYPE_TIME:
            case MYSQL_TYPE_DATE:
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_NEWDATE:
              ret = get_date_time_data(rowbuffer, i, rowbuffer[i - 1].buffer_type);
              break;
            case MYSQL_TYPE_GEOMETRY:
              ret = get_geometry_buffer_data(rowbuffer, i);
              break;
            default:
              if (_column_types[i - 1] == SQL_C_WCHAR)
                ret = get_wchar_buffer_data(rowbuffer, i);
              else
                ret = get_char_buffer_data(rowbuffer, i);
              break;
          }
          break;
        }
        case SQL_C_BINARY: {
          bool was_null = true;
          // During the migration process some non standard data types are migrated as strings
          // Those will come as SQL_C_BINARY but will be migrated as NULL for now
          if (rowbuffer[i - 1].buffer_type != MYSQL_TYPE_STRING) {
            was_null = false;
            ret = get_char_buffer_data(rowbuffer, i);
          }

          rowbuffer.finish_field(was_null);
        } break;

        default:
          throw std::logic_error(base::strfmt("Unhandled type %i", _column_types[i - 1]));
      }
      if (!SQL_SUCCEEDED(ret)) {
        rowbuffer.finish_field(true);
        throw ConnectionError("SQLGetData", ret, SQL_HANDLE_STMT, _stmt);
      }
    }
    return true;
  }
  return false;
}

MySQLCopyDataSource::MySQLCopyDataSource(const std::string &hostname, int port, const std::string &username,
                                         const std::string &password, const std::string &socket,
                                         bool use_cleartext_plugin, unsigned int connection_timeout)
  : _select_stmt(NULL), _has_long_data(false) {
  this->_connection_timeout = connection_timeout;
  std::string host = hostname;
  mysql_init(&_mysql);

  if (port > 0) {
    // Forces usage of TCP connection if indicated on the connection
    // settings (a port is specified)
    int proto = MYSQL_PROTOCOL_TCP;
    mysql_options(&_mysql, MYSQL_OPT_PROTOCOL, &proto);

    logInfo("Connecting to MySQL server at %s:%i with user %s\n", hostname.c_str(), port, username.c_str());
  } else {
// Socket file/Named pipe connections

#if defined(WIN32)
    // Default local host connection in windows are done through shared memory
    // using "." forces using named pipe
    host = ".";
#else
    // Default local host connections in Linux are done through socket files
    host = "localhost";
#endif

    logInfo("Connecting to MySQL server using socket %s with user %s\n", socket.c_str(), username.c_str());
  }

  mysql_options(&_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &_connection_timeout);

#if MYSQL_VERSION_ID >= 80004
  if (use_cleartext_plugin)
    logWarning("Trying to use the ClearText plugin, but it's not supported by libmysqlclient\n");
#else

  #if MYSQL_VERSION_ID >= 50527
    my_bool use_cleartext = use_cleartext_plugin;
    mysql_options(&_mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN, &use_cleartext);
  #else
    if (use_cleartext_plugin)
      logWarning("Trying to use the ClearText plugin, but it's not supported by libmysqlclient\n");
  #endif

#endif

  if (!mysql_real_connect(&_mysql, host.c_str(), username.c_str(), password.c_str(), NULL, port, socket.c_str(),
                          CLIENT_COMPRESS)) {
    logError("Failed opening connection to MySQL: %s\n", mysql_error(&_mysql));
    throw ConnectionError("mysql_real_connect", &_mysql);
  }
  logInfo("Connection to MySQL opened\n");

  std::string q = "SET NAMES 'utf8'";
  if (mysql_real_query(&_mysql, q.data(), (unsigned long)q.length()) != 0)
    throw ConnectionError(q, &_mysql);
}

size_t MySQLCopyDataSource::count_rows(const std::string &schema, const std::string &table,
                                       const std::vector<std::string> &pk_columns, const CopySpec &spec,
                                       const std::vector<std::string> &last_pkeys) {
  std::string q = base::strfmt("USE %s", schema.c_str());

  if (mysql_query(&_mysql, q.data()) < 0)
    throw ConnectionError("mysql_query(" + q + ")", &_mysql);

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

  if (mysql_query(&_mysql, q.data()) != 0)
    throw ConnectionError("mysql_query(" + q + ")", &_mysql);

  MYSQL_RES *result;
  if ((result = mysql_use_result(&_mysql)) == NULL)
    throw ConnectionError("MySQL query", &_mysql);

  // Retrieves the row count...
  MYSQL_ROW row = mysql_fetch_row(result);

  long long count = 0;
  if (row)
    count = atol(row[0]);

  mysql_free_result(result);

  if ((spec.type == CopyAll || spec.type == CopyWhere) && spec.max_count > 0 && spec.max_count < count)
    count = spec.max_count;

  return (size_t)count;
}

std::shared_ptr<std::vector<ColumnInfo> > MySQLCopyDataSource::begin_select_table(
  const std::string &schema, const std::string &table, const std::vector<std::string> &pk_columns,
  const std::string &select_expression, const CopySpec &spec, const std::vector<std::string> &last_pkeys) {
  std::shared_ptr<std::vector<ColumnInfo> > columns(new std::vector<ColumnInfo>());

  _schema_name = schema;
  _table_name = table;

  std::string q = base::strfmt("USE %s", schema.c_str());

  if (mysql_query(&_mysql, q.data()) < 0)
    throw ConnectionError("mysql_query(" + q + ")", &_mysql);

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

  logDebug("Executing query: %s\n", q.c_str());
  MYSQL_STMT *stmt = mysql_stmt_init(&_mysql);
  if (stmt) {
    if (mysql_stmt_prepare(stmt, q.data(), (unsigned long)q.length()) == 0) {
      _select_stmt = stmt;

      MYSQL_RES *result = mysql_stmt_result_metadata(_select_stmt);

      if (result) {
        int column_count = mysql_num_fields(result);

        logDebug2("Columns from source table %s.%s (%i):\n", schema.c_str(), table.c_str(), column_count);

        MYSQL_FIELD *fields = mysql_fetch_fields(result);

        for (int i = 0; i < column_count; i++) {
          ColumnInfo info;

          info.source_name = fields[i].name;
          info.source_type = mysql_field_type_to_name(fields[i].type);
          info.source_length = fields[i].length;
          info.is_unsigned = false;
          info.is_long_data = false;

          info.is_long_data = fields[i].type == MYSQL_TYPE_TINY_BLOB || fields[i].type == MYSQL_TYPE_MEDIUM_BLOB ||
                              fields[i].type == MYSQL_TYPE_BLOB;

          if (info.is_long_data)
            _has_long_data = true;

          logDebug2("%i - %s: %s\n", i + 1, info.source_name.c_str(), info.source_type.c_str());
          columns->push_back(info);
        }

        if (mysql_stmt_execute(_select_stmt) != 0)
          throw ConnectionError("mysql_stmt_execute", &_mysql);
      } else
        throw ConnectionError("mysql_stmt_result_metadata", &_mysql);
    } else {
      if (mysql_stmt_close(stmt))
        throw ConnectionError("mysql_stmt_close", &_mysql);

      throw ConnectionError("mysql_stmt_prepare", &_mysql);
    }
  } else
    throw ConnectionError("mysql_stmt_init", &_mysql);

  return columns;
}

void MySQLCopyDataSource::end_select_table() {
  if (_select_stmt) {
    if (mysql_stmt_close(_select_stmt))
      throw ConnectionError("mysql_stmt_close", &_mysql);
    else
      _select_stmt = NULL;
  }
}

bool MySQLCopyDataSource::fetch_row(RowBuffer &rowbuffer) {
  bool ret_val = true;

  if (mysql_stmt_bind_result(_select_stmt, &(rowbuffer[0])) != 0)
    throw ConnectionError(base::strfmt("mysql_stmt_bind_result: %s", mysql_stmt_error(_select_stmt)), &_mysql);

  int errcode = mysql_stmt_fetch(_select_stmt);

  if (errcode != 0) {
    if (errcode == MYSQL_DATA_TRUNCATED /*&& _has_long_data*/) {
      for (size_t index = 0; index < rowbuffer.size(); index++) {
        if (*rowbuffer[index].error) {
          if (rowbuffer[index].buffer_type == MYSQL_TYPE_TINY_BLOB ||
              rowbuffer[index].buffer_type == MYSQL_TYPE_MEDIUM_BLOB ||
              rowbuffer[index].buffer_type == MYSQL_TYPE_LONG_BLOB || rowbuffer[index].buffer_type == MYSQL_TYPE_BLOB ||
              rowbuffer[index].buffer_type == MYSQL_TYPE_STRING ||
              rowbuffer[index].buffer_type == MYSQL_TYPE_GEOMETRY || rowbuffer[index].buffer_type == MYSQL_TYPE_JSON) {
            if (rowbuffer[index].buffer_length)
              free(rowbuffer[index].buffer);

            rowbuffer[index].buffer_length = *rowbuffer[index].length;

            if (_max_parameter_size >= 0 && rowbuffer[index].buffer_length > (unsigned long long)_max_parameter_size) {
              if (_abort_on_oversized_blobs)
                throw std::runtime_error(base::strfmt("oversized blob found in table %s.%s, size: %lli",
                                                      _schema_name.c_str(), _table_name.c_str(),
                                                      (long long)rowbuffer[index].buffer_length));
              else {
                printf("oversized blob found in table %s.%s, size: %lli", _schema_name.c_str(), _table_name.c_str(),
                       (long long)rowbuffer[index].buffer_length);
                *rowbuffer[index].is_null = true;
                continue;
              }
            } else {
              rowbuffer[index].buffer = malloc(rowbuffer[index].buffer_length);

              mysql_stmt_fetch_column(_select_stmt, &rowbuffer[index], (unsigned int)index, 0);
            }
          } else
            ret_val = false;
        }
      }
    } else
      ret_val = false;

    // BLOBs will be sent ONLY when not doing bulk inserts
    if (ret_val && _has_long_data && !_use_bulk_inserts) {
      // Sends the BLOBS...
      rowbuffer.clear();
      for (size_t index = 0; index < rowbuffer.size(); index++) {
        if (rowbuffer.check_if_blob())
          rowbuffer.send_blob_data((const char *)rowbuffer[index].buffer, rowbuffer[index].buffer_length);

        // Advances the current field pointer insied row buffer
        rowbuffer.finish_field((*rowbuffer[index].is_null) == 1);
      }
    }
  }

  return ret_val;
}

MySQLCopyDataSource::~MySQLCopyDataSource() {
  if (_select_stmt)
    mysql_stmt_close(_select_stmt);
  mysql_close(&_mysql);
}

// -------------------------------------------------------------------------------------------------

void MySQLCopyDataTarget::init() {
  /*
   As of MySQL 5.1.57, the max_long_data_size system variable controls the maximum size of parameter
   values that can be sent with mysql_stmt_send_long_data(). If this variable not set at server startup,
   the default is the value of the max_allowed_packet system variable. max_long_data_size is deprecated.
   In MySQL 5.6, it is removed and the maximum parameter size is controlled by max_allowed_packet.
   */
  get_server_version();

  // find out the max packet size taken by the connection
  get_server_value("max_allowed_packet", _max_allowed_packet);
  logDebug("Detected max_allowed_packet=%lu\n", _max_allowed_packet);

  if (_major_version == 5 &&
      ((_minor_version > 1 && _minor_version < 6) || (_minor_version == 1 && _build_version >= 57))) {
    // find out the max parameter size on a prepared statement
    get_server_value("max_long_data_size", _max_long_data_size);
    logDebug("Detected max_long_data_size=%lu\n", _max_long_data_size);
  } else
    _max_long_data_size = _max_allowed_packet;

  std::string q = "SET NAMES 'utf8'";
  if (mysql_real_query(&_mysql, q.data(), (unsigned long)q.length()) != 0)
    throw ConnectionError(q, &_mysql);

  // the source data will come in a charset that's not utf-8, so we let the server do the conversion
  if (!_incoming_data_charset.empty()) {
    logInfo("Setting charset for source data to %s\n", _incoming_data_charset.c_str());
    q = base::sqlstring("SET character_set_client=?", 0) << _incoming_data_charset;
    if (mysql_real_query(&_mysql, q.data(), (unsigned long)q.length()) != 0)
      throw ConnectionError(q, &_mysql);
  }

  q = "SET FOREIGN_KEY_CHECKS=0";
  if (mysql_real_query(&_mysql, q.data(), (unsigned long)q.length()) != 0)
    throw ConnectionError(q, &_mysql);

  // some DBs (like MS Access) have sequence/auto-increment values start at 0
  // by default, mysql will change that to 1, so when the actual row numbered 1 comes it will be duplicated
  if (mysql_query(&_mysql, "SET SESSION SQL_MODE=CONCAT('NO_AUTO_VALUE_ON_ZERO,', @@SQL_MODE)") != 0) {
    logWarning("Error changing sql_mode: %s\n", mysql_error(&_mysql));
  }
}

std::vector<std::string> MySQLCopyDataTarget::get_last_pkeys(const std::vector<std::string> &pk_columns,
                                                             const std::string &schema, const std::string &table) {
  std::vector<std::string> ret;
  std::string order_by_cond;
  if (pk_columns.empty())
    throw std::logic_error("Get last copied row: Cannot get last copied record from table with no PK.");

  for (size_t i = 0; i < pk_columns.size(); ++i) {
    order_by_cond += base::strfmt("%s DESC", pk_columns[i].c_str());
    if (i < pk_columns.size() - 1)
      order_by_cond += ",";
  }

  const std::string q =
    base::strfmt("SELECT %s FROM %s.%s ORDER BY %s LIMIT 0,1", boost::algorithm::join(pk_columns, ", ").c_str(),
                 schema.c_str(), table.c_str(), order_by_cond.c_str());
  if (mysql_query(&_mysql, q.data()) != 0)
    throw ConnectionError("mysql_query(" + q + ")", &_mysql);

  MYSQL_RES *result;
  if ((result = mysql_use_result(&_mysql)) == NULL)
    throw ConnectionError("mysql_use_result", &_mysql);

  MYSQL_ROW row = mysql_fetch_row(result);

  if (row)
    for (size_t i = 0; i < pk_columns.size(); ++i)
      ret.push_back(row[i]);

  mysql_free_result(result);

  MYSQL_STMT *stmt = mysql_stmt_init(&_mysql);
  if (stmt) {
    if (mysql_stmt_prepare(stmt, q.data(), (unsigned long)q.length()) == 0) {
      MYSQL_RES *meta = mysql_stmt_result_metadata(stmt);
      if (meta) {
        MYSQL_FIELD *fields = mysql_fetch_fields(meta);
        std::string column_value;
        for (size_t i = 0; i < ret.size(); ++i) {
          if (fields[i].type == MYSQL_TYPE_TIMESTAMP && base::hasSuffix(ret[i], ".000000"))
            column_value += base::strfmt("%s: %s", pk_columns[i].c_str(), ret[i].substr(ret[i].length() - 7).c_str());
          else
            column_value += base::strfmt("%s: %s", pk_columns[i].c_str(), ret[i].c_str());
          if (i < pk_columns.size() - 1)
            column_value += ",";
        }
        logInfo("Resuming copy of table %s.%s. Starting on record with keys: %s\n", schema.c_str(), table.c_str(),
                column_value.c_str());
        mysql_free_result(meta);
      } else {
        ConnectionError err("mysql_stmt_result_metadata", stmt);
        mysql_stmt_close(stmt);
        throw err;
      }
    } else {
      ConnectionError err("mysql_stmt_prepare", stmt);
      mysql_stmt_close(stmt);
      throw err;
    }
    mysql_stmt_close(stmt);
  } else
    throw ConnectionError("mysql_stmt_init", &_mysql);
  return ret;
}

MYSQL_RES *MySQLCopyDataTarget::get_server_value(const std::string &variable) {
  std::string q = "SHOW VARIABLES LIKE '" + variable + "'";
  if (mysql_real_query(&_mysql, q.data(), (unsigned long)q.length()) < 0)
    throw ConnectionError(q, &_mysql);

  MYSQL_RES *result;
  if ((result = mysql_use_result(&_mysql)) == NULL)
    throw ConnectionError("MySQL query", &_mysql);

  return result;
}

void MySQLCopyDataTarget::get_server_value(const std::string &variable, std::string &value) {
  MYSQL_RES *result = get_server_value(variable);

  MYSQL_ROW row = mysql_fetch_row(result);

  if (row)
    value = row[1];

  mysql_free_result(result);
}

void MySQLCopyDataTarget::get_server_value(const std::string &variable, unsigned long &value) {
  MYSQL_RES *result = get_server_value(variable);

  MYSQL_ROW row = mysql_fetch_row(result);

  if (row) {
    unsigned long *lengths = mysql_fetch_lengths(result);
    sscanf(std::string(row[1], lengths[1]).c_str(), "%lu", &value);
  }

  mysql_free_result(result);
}

void MySQLCopyDataTarget::get_server_version() {
  std::string version;

  get_server_value("version", version);

  std::vector<std::string> parsed_version = base::split(version, ".");

  _major_version = base::atoi<int>(parsed_version[0], 0);
  if (parsed_version.size() > 1)
    _minor_version = base::atoi<int>(parsed_version[1], 0);

  if (parsed_version.size() > 2)
    _build_version = base::atoi<int>(parsed_version[2], 0);

  logDebug("Detected server version=%s\n", version.c_str());
}

bool MySQLCopyDataTarget::is_mysql_version_at_least(const int _major, const int _minor, const int _build) {
  return _major_version > _major || (_major_version == _major && _minor_version > _minor) ||
         (_major_version == _major && _minor_version == _minor && _build_version >= _build);
}

std::string MySQLCopyDataTarget::ps_query() {
  std::string q("INSERT INTO ");
  q.append(base::strfmt("%s.%s", _schema.c_str(), _table.c_str())).append(" (");
  for (std::vector<ColumnInfo>::const_iterator iter = _columns->begin(); iter != _columns->end(); ++iter) {
    if (iter != _columns->begin())
      q.append(", ").append(base::sqlstring("!", 0) << iter->target_name);
    else
      q.append(base::sqlstring("!", 0) << iter->target_name);
  }

  q.append(") VALUES ");

  // On Prepared statemnts a sample record with the wildcards needs to be set
  // On bulk inserts the real records will be appended later
  if (!_use_bulk_inserts) {
    q.append("(");
    for (std::vector<ColumnInfo>::const_iterator iter = _columns->begin(); iter != _columns->end(); ++iter) {
      if (iter != _columns->begin())
        q.append(", ?");
      else
        q.append("?");
    }
    q.append(")");
  }
  return q;
}

enum enum_field_types MySQLCopyDataTarget::field_type_to_ps_param_type(enum enum_field_types ftype) {
  // convert the resultset types to the PS param types
  switch (ftype) {
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      ftype = MYSQL_TYPE_DOUBLE;
      break;
    case MYSQL_TYPE_TINY:
      ftype = MYSQL_TYPE_TINY;
      break;
    case MYSQL_TYPE_SHORT:
      ftype = MYSQL_TYPE_SHORT;
      break;
    case MYSQL_TYPE_LONG:
      ftype = MYSQL_TYPE_LONG;
      break;
    case MYSQL_TYPE_FLOAT:
      ftype = MYSQL_TYPE_FLOAT;
      break;
    case MYSQL_TYPE_DOUBLE:
      ftype = MYSQL_TYPE_DOUBLE;
      break;
    case MYSQL_TYPE_NULL:
      ftype = MYSQL_TYPE_NULL;
      break;
    case MYSQL_TYPE_TIMESTAMP:
      ftype = MYSQL_TYPE_TIMESTAMP;
      break;
    case MYSQL_TYPE_LONGLONG:
      ftype = MYSQL_TYPE_LONGLONG;
      break;
    case MYSQL_TYPE_INT24:
      ftype = MYSQL_TYPE_LONG;
      break;
    case MYSQL_TYPE_DATE:
      ftype = MYSQL_TYPE_DATE;
      break;
    case MYSQL_TYPE_TIME:
      ftype = MYSQL_TYPE_TIME;
      break;
    case MYSQL_TYPE_DATETIME:
      ftype = MYSQL_TYPE_DATETIME;
      break;
    case MYSQL_TYPE_YEAR:
      ftype = MYSQL_TYPE_SHORT;
      break;
    case MYSQL_TYPE_NEWDATE:
      ftype = MYSQL_TYPE_DATE;
      break;
    case MYSQL_TYPE_VARCHAR:
      ftype = MYSQL_TYPE_STRING;
      break;
    case MYSQL_TYPE_BIT:
      ftype = MYSQL_TYPE_BIT;
      break;
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_SET:
      ftype = MYSQL_TYPE_STRING;
      break;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      ftype = MYSQL_TYPE_BLOB;
      break;
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
      ftype = MYSQL_TYPE_STRING;
      break;
    case MYSQL_TYPE_GEOMETRY:
      if (base::tolower(_source_rdbms_type) == "mysql")
        ftype = MYSQL_TYPE_BLOB;
      else
        ftype = MYSQL_TYPE_GEOMETRY;
      break;
    default:
      break;
  }
  return ftype;
}

MySQLCopyDataTarget::MySQLCopyDataTarget(const std::string &hostname, int port, const std::string &username,
                                         const std::string &password, const std::string &socket,
                                         bool use_cleartext_plugin, const std::string &app_name,
                                         const std::string &incoming_charset, const std::string &source_rdbms_type,
                                         const unsigned int connection_timeout)
  : _insert_stmt(NULL),
    _max_allowed_packet(1000000),
    _max_long_data_size(1000000), // 1M default
    _row_buffer(NULL),
    _major_version(0),
    _minor_version(0),
    _build_version(0),
    _use_bulk_inserts(true),
    _bulk_insert_buffer(this),
    _bulk_insert_record(this),
    _bulk_insert_batch(0),
    _source_rdbms_type(source_rdbms_type),
    _connection_timeout(connection_timeout) {
  std::string host = hostname;
  _truncate = false;

  _incoming_data_charset = incoming_charset;
  if (base::tolower(_incoming_data_charset) == "cp1252" || base::tolower(_incoming_data_charset) == "windows-1252")
    _incoming_data_charset = "latin1";

  mysql_init(&_mysql);

#if MYSQL_VERSION_ID >= 50606
  if (is_mysql_version_at_least(5, 6, 6))
    mysql_options4(&_mysql, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", app_name.c_str());
#endif

  // _bulk_insert_record is used to prepare a single record string, the connection
  // is needed to escape binary data properly
  _bulk_insert_record.set_connection(&_mysql);

  if (port > 0) {
    // Forces usage of TCP connection if indicated on the connection
    // settings (a port is specified)
    int proto = MYSQL_PROTOCOL_TCP;
    mysql_options(&_mysql, MYSQL_OPT_PROTOCOL, &proto);

    logInfo("Connecting to MySQL server at %s:%i with user %s\n", hostname.c_str(), port, username.c_str());
  } else {
// Socket file/Named pipe connections

#if defined(WIN32)
    // Default local host connection in windows are done through shared memory
    // using "." forces using named pipe
    host = ".";
#else
    // Default local host connections in Linux are done through socket files
    host = "localhost";
#endif

    logInfo("Connecting to MySQL server using socket %s with user %s\n", socket.c_str(), username.c_str());
  }
  mysql_options(&_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &_connection_timeout);


#if MYSQL_VERSION_ID >= 80004
  if (use_cleartext_plugin)
    logWarning("Trying to use the ClearText plugin, but it's not supported by libmysqlclient\n");
#else

  #if MYSQL_VERSION_ID >= 50527
    my_bool use_cleartext = use_cleartext_plugin;
    mysql_options(&_mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN, &use_cleartext);
  #else
    if (use_cleartext_plugin)
      logWarning("Trying to use the ClearText plugin, but it's not supported by libmysqlclient\n");
  #endif

#endif

  if (!mysql_real_connect(&_mysql, host.c_str(), username.c_str(), password.c_str(), NULL, port, socket.c_str(),
                          CLIENT_COMPRESS)) {
    logError("Failed opening connection to MySQL: %s\n", mysql_error(&_mysql));
    throw ConnectionError("mysql_real_connect", &_mysql);
  }
  logInfo("Connection to MySQL opened\n");

  init();
}

MySQLCopyDataTarget::~MySQLCopyDataTarget() {
  delete _row_buffer;
  if (_insert_stmt)
    mysql_stmt_close(_insert_stmt);
  mysql_close(&_mysql);
}

void MySQLCopyDataTarget::set_truncate(bool flag) {
  _truncate = flag;
}

void MySQLCopyDataTarget::get_generated_columns(const std::string &schema, const std::string &table,
                                                std::vector<std::string> &gc) {
  gc.clear();
  std::string q = base::strfmt(
    "SELECT COLUMN_NAME FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s' AND EXTRA like "
    "'%%GENERATED%%';",
    base::unquote(schema).c_str(), base::unquote(table).c_str());

  if (mysql_query(&_mysql, q.data()) < 0)
    throw ConnectionError("mysql_query(" + q + ")", &_mysql);

  MYSQL_RES *result;
  if ((result = mysql_use_result(&_mysql)) == NULL)
    throw ConnectionError("Getting Generated Columns", &_mysql);

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)))
    gc.push_back(row[0]);

  mysql_free_result(result);
}

void MySQLCopyDataTarget::set_target_table(const std::string &schema, const std::string &table,
                                           std::shared_ptr<std::vector<ColumnInfo> > columns) {
  _schema = schema;
  _table = table;
  _columns = columns;

  // create a PS and prepare it, which will get us the metadata we need without
  // actually executing the query
  std::string q = base::strfmt("SELECT * FROM %s.%s", schema.c_str(),
                               table.c_str()); // base::sqlstring("SELECT * FROM !.!", 0) << schema << table;
  MYSQL_STMT *stmt = mysql_stmt_init(&_mysql);
  if (stmt) {
    if (mysql_stmt_prepare(stmt, q.data(), (unsigned long)q.length()) == 0) {
      MYSQL_RES *meta = mysql_stmt_result_metadata(stmt);
      if (meta) {
        int column_count = mysql_num_fields(meta);
        MYSQL_FIELD *fields = mysql_fetch_fields(meta);

        std::vector<std::string> generated_columns;
        if (column_count > (int)columns->size())
          get_generated_columns(schema, table, generated_columns);

        int gc_count = (int)generated_columns.size();
        std::string target_name;

        if ((column_count - gc_count) == (int)columns->size()) {
          logDebug2("Columns from target table %s.%s (%i) [skipped: %i]:\n", schema.c_str(), table.c_str(),
                    column_count - gc_count, gc_count);
          unsigned short idx_mod = 0;
          for (int i = 0; i < column_count; i++) {
            target_name = std::string(fields[i].name, fields[i].name_length);
            if (std::find(generated_columns.begin(), generated_columns.end(), target_name) != generated_columns.end()) {
              logDebug2("%i - %s: GENERATED [SKIPPED]\n", i + 1, target_name.c_str());
              idx_mod++;
              continue;
            }
            (*columns)[i - idx_mod].target_name = target_name;
            (*columns)[i - idx_mod].target_type = field_type_to_ps_param_type(fields[i].type);
            // We can't trust source drivers to report signed/unsigned values, so we take that from the target MySQL
            // table:
            (*columns)[i - idx_mod].is_unsigned = (fields[i].flags & UNSIGNED_FLAG) != 0;
            if (_get_field_lengths_from_target)
              (*columns)[i - idx_mod].source_length = fields[i].length;
            logDebug2("%i - %s: %s\n", i + 1, (*columns)[i - idx_mod].target_name.c_str(),
                      mysql_field_type_to_name((*columns)[i - idx_mod].target_type));
          }
        } else {
          mysql_free_result(meta);
          mysql_stmt_close(stmt);
          throw std::runtime_error(
            base::strfmt("Table %s.%s has wrong number of columns in target DB (%i, expected %i)", schema.c_str(),
                         table.c_str(), column_count, (int)columns->size()));
        }
        mysql_free_result(meta);
      } else {
        ConnectionError err("mysql_stmt_result_metadata", stmt);
        mysql_stmt_close(stmt);
        throw err;
      }
    } else {
      ConnectionError err("mysql_stmt_prepare", stmt);
      mysql_stmt_close(stmt);
      throw err;
    }
    mysql_stmt_close(stmt);
  } else
    throw ConnectionError("mysql_stmt_init", &_mysql);

  if (_truncate) {
    logInfo("Truncating table %s.%s\n", schema.c_str(), table.c_str());
    if (mysql_query(&_mysql, base::strfmt("TRUNCATE %s.%s", schema.c_str(), table.c_str()).c_str()) != 0)
      logWarning("Error executing TRUNCATE %s.%s: %s\n", schema.c_str(), table.c_str(), mysql_error(&_mysql));
  }

  // TODO: Bulk inserts should be disabled when a single record can be bigger than the max_packet_size
  _use_bulk_inserts = true;
  if (_use_bulk_inserts) {
    _bulk_insert_buffer.reset(_max_allowed_packet);
    _bulk_insert_record.reset(_max_allowed_packet);
  }
}

void MySQLCopyDataTarget::send_long_data(int column, const char *data, size_t length) {
  if (mysql_stmt_send_long_data(_insert_stmt, column, data, (unsigned long)length)) {
    std::string error = base::strfmt("Error sending long data: %s", mysql_stmt_error(_insert_stmt));
    throw std::logic_error(error);
  }
}

void MySQLCopyDataTarget::begin_inserts() {
  MYSQL_STMT *stmt;

  // Initialize variables for non prepared insert statement
  _bulk_insert_query = ps_query();
  _init_bulk_insert = true;
  _bulk_record_count = 0;

  // The RowBuffer is used by the CopyDataSources to store in it the data read from the
  // database, once the data is loaded in it, it is used for both bulk inserts
  // and prepared statements
  if (_row_buffer)
    delete _row_buffer;

  _row_buffer = new RowBuffer(_columns, std::bind(&MySQLCopyDataTarget::send_long_data, this, std::placeholders::_1,
                                                  std::placeholders::_2, std::placeholders::_3),
                              _max_allowed_packet);

  if (!_use_bulk_inserts) {
    stmt = mysql_stmt_init(&_mysql);
    if (!stmt)
      throw ConnectionError("mysql_stmt_init", &_mysql);

    if (mysql_stmt_prepare(stmt, _bulk_insert_query.data(), (unsigned long)_bulk_insert_query.length()) != 0) {
      mysql_stmt_close(stmt);
      throw ConnectionError("mysql_stmt_prepare", stmt);
    }
    if (mysql_stmt_param_count(stmt) != _columns->size()) {
      mysql_stmt_close(stmt);
      throw std::logic_error("Unexpected parameter count for PS returned by MySQL");
    }

    if (mysql_stmt_bind_param(stmt, &(*_row_buffer)[0]) != 0)
      throw ConnectionError("mysql_stmt_bind_param", stmt);

    _insert_stmt = stmt;
  }
}

int MySQLCopyDataTarget::end_inserts(bool flush) {
  int ret_val = 0;

  // When doing bulk inserts it is possible that some records are still pending on the
  // _bulk_insert_buffer or _bulk_insert_record so they need to be inserted
  if (_use_bulk_inserts) {
    if (flush) {
      if (_bulk_insert_buffer.length)
        ret_val = do_insert(true);
      else if (_bulk_insert_record.length) {
        _init_bulk_insert = true;
        ret_val = do_insert(true);
      }
    }
  } else {
    if (_insert_stmt)
      mysql_stmt_close(_insert_stmt);
    _insert_stmt = NULL;
  }

  return ret_val;
}

int MySQLCopyDataTarget::do_insert(bool final) {
  int ret_val = 0;

  if (_use_bulk_inserts) {
    bool add_comma = true;

    if (_init_bulk_insert) {
      add_comma = false;
      _init_bulk_insert = false;

      _bulk_insert_buffer.append(_bulk_insert_query.c_str(), _bulk_insert_query.length());

      if (_bulk_insert_record.length) {
        _bulk_insert_buffer.append(_bulk_insert_record.buffer, _bulk_insert_record.length);
        _bulk_insert_record.reset(_max_allowed_packet);
        _bulk_record_count++;
        add_comma = true;
      }
    }

    // This will be disabled if still have records but they still fit into the buffer
    bool do_insert = true;

    // If it is not the last insert (there still pending records)
    // Then continues with the formatting
    if (!final) {
      // Formats the next record into _bulk_insert_record
      if (format_bulk_record()) {
        // Next record + 1 as the comma also counts
        if (_bulk_insert_buffer.space_left() >= (_bulk_insert_record.length + (add_comma ? 1 : 0))) {
          if (add_comma)
            _bulk_insert_buffer.append(",", 1);

          _bulk_insert_buffer.append(_bulk_insert_record.buffer, _bulk_insert_record.length);
          _bulk_insert_record.reset(_max_allowed_packet);
          _bulk_record_count++;

          // Forces the insert when the max number of records has been reached
          do_insert = _bulk_record_count == _bulk_insert_batch;
        }
      } else {
        throw std::runtime_error("Found record bigger than max_allowed_packet");
      }
    }

    if (do_insert) {
      ret_val = _bulk_record_count;
      _init_bulk_insert = true;
      if (mysql_real_query(&_mysql, _bulk_insert_buffer.buffer, (unsigned long)_bulk_insert_buffer.length) != 0) {
        _bulk_insert_buffer.buffer[_bulk_insert_buffer.length] = 0;
        logInfo("Statement execution failed: %s:\n%s\n", mysql_error(&_mysql), _bulk_insert_buffer.buffer);

        throw ConnectionError("Inserting Data", &_mysql);
      }
      _bulk_insert_buffer.reset(_max_allowed_packet);
      _bulk_record_count = 0;
    }
  } else {
    if (mysql_stmt_execute(_insert_stmt) != 0)
      throw ConnectionError("mysql_stmt_execute", _insert_stmt);

    ret_val = 1;
  }

  return ret_val;
}

bool MySQLCopyDataTarget::format_bulk_record() {
  bool ret_val = true;
  _bulk_insert_record.append("(", 1);

  for (size_t index = 0; ret_val && index < _row_buffer->size() - 1; index++) {
    ret_val = append_bulk_column(index);
    _bulk_insert_record.append(",", 1);
  }

  if (ret_val) {
    ret_val = append_bulk_column(_row_buffer->size() - 1);

    if (ret_val)
      ret_val = _bulk_insert_record.append(")", 1);
  }

  return ret_val;
}

bool MySQLCopyDataTarget::append_bulk_column(size_t col_index) {
  std::string data;
  bool ret_val = true;

  if (*(*_row_buffer)[col_index].is_null)
    ret_val = _bulk_insert_record.append("NULL", 4);
  else {
    switch ((*_row_buffer)[col_index].buffer_type) {
      case MYSQL_TYPE_NULL:
        ret_val = _bulk_insert_record.append("NULL", 4);
        break;
      case MYSQL_TYPE_TINY:
        if ((*_row_buffer)[col_index].is_unsigned) {
          unsigned char *val_char = (unsigned char *)(*_row_buffer)[col_index].buffer;
          data = base::strfmt("%u", *val_char);
        } else {
          char *val_char = (char *)(*_row_buffer)[col_index].buffer;
          data = base::strfmt("%d", *val_char);
        }
        ret_val = _bulk_insert_record.append(data.data(), data.length());
        break;
      case MYSQL_TYPE_SHORT:
      case MYSQL_TYPE_YEAR:
        if ((*_row_buffer)[col_index].is_unsigned) {
          unsigned short *val_short = (unsigned short *)(*_row_buffer)[col_index].buffer;
          data = base::strfmt("%u", *val_short);
        } else {
          short *val_short = (short *)(*_row_buffer)[col_index].buffer;
          data = base::strfmt("%d", *val_short);
        }
        ret_val = _bulk_insert_record.append(data.data(), data.length());
        break;
      case MYSQL_TYPE_INT24:
      case MYSQL_TYPE_LONG:
        if ((*_row_buffer)[col_index].is_unsigned) {
          unsigned int *val_int = (unsigned int *)(*_row_buffer)[col_index].buffer;
          data = base::strfmt("%u", *val_int);
        } else {
          int *val_int = (int *)(*_row_buffer)[col_index].buffer;
          data = base::strfmt("%i", *val_int);
        }
        ret_val = _bulk_insert_record.append(data.data(), data.length());
        break;
      case MYSQL_TYPE_LONGLONG:
        if ((*_row_buffer)[col_index].is_unsigned) {
          unsigned long long int *val_llint = (unsigned long long int *)(*_row_buffer)[col_index].buffer;
          data = base::strfmt("%llu", *val_llint);
        } else {
          long long int *val_llint = (long long int *)(*_row_buffer)[col_index].buffer;
          data = base::strfmt("%lli", *val_llint);
        }
        ret_val = _bulk_insert_record.append(data.data(), data.length());
        break;
      case MYSQL_TYPE_FLOAT: {
        float *val_float = (float *)(*_row_buffer)[col_index].buffer;
        data = base::strfmt("%f", *val_float);
        ret_val = _bulk_insert_record.append(data.data(), data.length());
      } break;
      case MYSQL_TYPE_DOUBLE: {
        double *val_double = (double *)(*_row_buffer)[col_index].buffer;
        data = base::strfmt("%f", *val_double);
        ret_val = _bulk_insert_record.append(data.data(), data.length());
      } break;
      case MYSQL_TYPE_BIT: {
        // As managed as string, an additional byte is added to the length, so
        // we remove that here to know the real legth in bytes
        std::div_t length = std::div((int)(*_row_buffer)[col_index].buffer_length - 1, 8);

        if (length.rem)
          ++length.quot;

        unsigned long long uval = 0;
        unsigned int shift = 0;

        for (int index = 1; index <= length.quot; index++) {
          uval += (((unsigned char *)(*_row_buffer)[col_index].buffer)[length.quot - index]) << shift;
          shift += 8;
        }

        std::string numeric_value = base::strfmt("%llu", uval);

        ret_val = _bulk_insert_record.append(numeric_value.data(), numeric_value.length());
        break;
      }
      case MYSQL_TYPE_DECIMAL:
      case MYSQL_TYPE_NEWDECIMAL:
        ret_val = _bulk_insert_record.append_escaped((char *)(*_row_buffer)[col_index].buffer,
                                                     *(*_row_buffer)[col_index].length);
        break;
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_ENUM:
      case MYSQL_TYPE_SET:
      case MYSQL_TYPE_JSON:
        _bulk_insert_record.append("'", 1);
        if ((*_columns)[col_index].source_type == "decimal") {
            ret_val = _bulk_insert_record.append((char *)(*_row_buffer)[col_index].buffer);
        }
        else {
            ret_val = _bulk_insert_record.append_escaped((char *)(*_row_buffer)[col_index].buffer,
                                                         *(*_row_buffer)[col_index].length);
        }
        _bulk_insert_record.append("'", 1);
        break;
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_NEWDATE:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_TIMESTAMP: {
        MYSQL_TIME *ts = (MYSQL_TIME *)(*_row_buffer)[col_index].buffer;
        switch (ts->time_type) {
          case MYSQL_TIMESTAMP_DATETIME:
            if (_major_version >= 6 || (_major_version == 5 && _minor_version >= 7) ||
                (_major_version == 5 && _minor_version == 6 && _build_version >= 4))
              data = base::strfmt("'%04d-%02d-%02d %02d:%02d:%02d.%06lu'", ts->year, ts->month, ts->day, ts->hour,
                                  ts->minute, ts->second, ts->second_part);
            else
              data = base::strfmt("'%04d-%02d-%02d %02d:%02d:%02d'", ts->year, ts->month, ts->day, ts->hour, ts->minute,
                                  ts->second);
            break;
          case MYSQL_TIMESTAMP_DATE:
            data = base::strfmt("'%04d-%02d-%02d'", ts->year, ts->month, ts->day);
            break;
          case MYSQL_TIMESTAMP_TIME:
            if (_major_version >= 6 || (_major_version == 5 && _minor_version >= 7) ||
                (_major_version == 5 && _minor_version == 6 && _build_version >= 4))
              data = base::strfmt("'%02d:%02d:%02d.%06lu'", ts->hour, ts->minute, ts->second, ts->second_part);
            else
              data = base::strfmt("'%02d:%02d:%02d'", ts->hour, ts->minute, ts->second);
            break;
          default:
            data = "''";
            break;
        }

        ret_val = _bulk_insert_record.append(data.data(), data.length());
      } break;
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
        _bulk_insert_record.append("'", 1);
        ret_val = _bulk_insert_record.append_escaped((char *)(*_row_buffer)[col_index].buffer,
                                                     *(*_row_buffer)[col_index].length);
        _bulk_insert_record.append("'", 1);
        break;

#if MYSQL_VERSION_ID > 50600
      case MYSQL_TYPE_TIMESTAMP2:
      case MYSQL_TYPE_DATETIME2:
      case MYSQL_TYPE_TIME2:
#endif
#if MYSQL_VERSION_ID > 80016
      case MYSQL_TYPE_TYPED_ARRAY: /* Used only for replication. */
#endif
        // TODO: implement handling
        break;
      case MYSQL_TYPE_GEOMETRY:
        if (_major_version >= 6 || (_major_version == 5 && _minor_version >= 7) ||
            (_major_version == 5 && _minor_version == 6 && _build_version >= 6))
          _bulk_insert_record.append("ST_GeomFromText('");
        else
          _bulk_insert_record.append("GeomFromText('");
        ret_val = _bulk_insert_record.append_escaped((char *)(*_row_buffer)[col_index].buffer,
                                                     *(*_row_buffer)[col_index].length);
        _bulk_insert_record.append("')");
        break;
#if MYSQL_VERSION_ID > 80021
      case MYSQL_TYPE_INVALID:
        // TODO: added to fix the build. Need to check how to handle this.
        break;
      case MYSQL_TYPE_BOOL:
        // TODO: added to fix the build. Need to check how to handle this. In the current version this is just a placeholder.
        break;
#endif
      
    }
  }

  return ret_val;
}

RowBuffer &MySQLCopyDataTarget::row_buffer() {
  return *_row_buffer;
}

long long MySQLCopyDataTarget::get_max_value(const std::string &key) {
  std::string q = base::sqlstring("SELECT max(!) FROM !.!", 0) << key << _schema << _table;
  mysql_query(&_mysql, q.c_str());
  return 0;
}

void MySQLCopyDataTarget::get_triggers_for_schema(const std::string &schema,
                                                  std::map<std::string, std::string> &triggers) {
  // Now pulls the trigger names
  logDebug("Retrieving trigger list\n");
  std::string get_trigger_list = base::sqlstring("SHOW TRIGGERS FROM !", 0) << schema;
  if (mysql_query(&_mysql, get_trigger_list.data()) != 0)
    throw ConnectionError("Querying Trigger List", &_mysql);

  MYSQL_RES *result;
  if ((result = mysql_use_result(&_mysql)) == NULL)
    throw ConnectionError("Getting Trigger List", &_mysql);

  // Gets the trigger names so they can be backed up
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)))
    triggers[row[0]] = "";

  mysql_free_result(result);
}

bool MySQLCopyDataTarget::get_trigger_definitions_for_schema(const std::string &schema,
                                                             std::map<std::string, std::string> &triggers) {
  bool success = true;
  std::map<std::string, std::string>::iterator index, end = triggers.end();

  // Pulls every trigger unless an error is found
  for (index = triggers.begin(); success && index != end; index++) {
    bool trigger_pulled = false;

    logDebug("Retrieving trigger definition for: %s\n", index->first.c_str());

    std::string get_trigger = base::sqlstring("SHOW CREATE TRIGGER !.!", 0) << schema << index->first;
    if (mysql_query(&_mysql, get_trigger.data()) == 0) {
      MYSQL_RES *result;
      if ((result = mysql_use_result(&_mysql)) != NULL) {
        MYSQL_ROW row;
        row = mysql_fetch_row(result);

        if (row) {
          triggers[index->first] = row[2];
          trigger_pulled = true;
        }

        mysql_free_result(result);
      }
    }

    // Updates the exit flag
    success = trigger_pulled;
  }

  // Will return TRUE only if all the triggers were successfully pulled
  return success;
}

void MySQLCopyDataTarget::backup_triggers_for_schema(const std::string &schema) {
  bool created_table = false;
  std::string tmp_trigger_table(TMP_TRIGGER_TABLE);
  std::map<std::string, std::string> triggers;
  std::map<std::string, std::string>::iterator index, end;

  // Ensures there are triggers to backup
  get_triggers_for_schema(schema, triggers);

  if (!triggers.empty()) {
    logInfo("Disabling triggers for schema '%s'...", schema.c_str());

    // Creates the trigger backup table
    logDebug("\nCreating temporary trigger table: %s\n", tmp_trigger_table.c_str());
    std::string create_trigger_backup_table =
      base::sqlstring("CREATE TABLE !.! ( ! VARCHAR(100) NOT NULL, ! MEDIUMTEXT, PRIMARY KEY (!))", 0)
      << schema << TMP_TRIGGER_TABLE << "trigger_name"
      << "trigger_sql"
      << "trigger_name";

    if (mysql_query(&_mysql, create_trigger_backup_table.data()) != 0) {
      // If the backup table already exists then it is ok to continue
      if (mysql_errno(&_mysql) != 1050)
        throw ConnectionError("Unable to create trigger backup table", &_mysql);
      else
        logInfo("The trigger backup table already existed on %s\n", schema.c_str());
    } else
      created_table = true;

    end = triggers.end();

    if (get_trigger_definitions_for_schema(schema, triggers)) {
      // Backups the triggers
      for (index = triggers.begin(); index != end; index++) {
        if (!index->second.empty()) {
          logDebug("Backing up trigger definition for: %s\n", index->first.c_str());
          std::string insert_trigger = base::sqlstring("INSERT INTO !.! VALUES (?,?)", 0)
                                       << schema << TMP_TRIGGER_TABLE << index->first << index->second;

          if (mysql_query(&_mysql, insert_trigger.data()) != 0) {
            // An error inserting the trigger because already exists is OK
            // It may be there from a previous run
            if (mysql_errno(&_mysql) != 1062) {
              // If the error is different and the backup table was created
              // on this run, it will be dropped
              if (created_table)
                drop_trigger_backups(schema);

              throw ConnectionError("Backing Up Trigger", &_mysql);
            } else
              logInfo("The trigger %s was already in the backup\n", index->first.c_str());
          }
        }
      }

      // Now proceeds to remove the triggers
      for (index = triggers.begin(); index != end; index++) {
        logDebug("Dropping trigger: %s\n", index->first.c_str());
        std::string drop_trigger = base::sqlstring("DROP TRIGGER !.!", 0) << schema << index->first;
        if (mysql_query(&_mysql, drop_trigger.data()) != 0) {
          // If an error occurs and the backup table was created on this
          // run, the table is dropped
          if (created_table)
            drop_trigger_backups(schema);

          throw ConnectionError("Dropping Trigger", &_mysql);
        }
      }
    }

    logInfo("Successfully backed up %lu triggers.\n", (unsigned long)triggers.size());
  }
}

void MySQLCopyDataTarget::drop_trigger_backups(const std::string &schema) {
  logDebug("Deleting trigger backups\n");

  std::string drop_trigger_table = base::sqlstring("DROP TABLE !.!", 0) << schema << TMP_TRIGGER_TABLE;

  if (mysql_query(&_mysql, drop_trigger_table.data()) != 0)
    throw ConnectionError("Dropping trigger backups", &_mysql);
}

void MySQLCopyDataTarget::backup_triggers(std::set<std::string> &schemas) {
  std::set<std::string>::const_iterator index;
  std::set<std::string>::const_iterator end = schemas.end();

  for (index = schemas.begin(); index != end; index++)
    backup_triggers_for_schema(base::unquote_identifier(*index));
}

void MySQLCopyDataTarget::restore_triggers(std::set<std::string> &schemas) {
  std::set<std::string>::const_iterator index;
  std::set<std::string>::const_iterator end = schemas.end();

  for (index = schemas.begin(); index != end; index++) {
    std::vector<std::string> trigger_name;
    std::vector<std::string> trigger_sql;
    std::string a_schema = base::unquote_identifier(*index);

    logInfo("Re-enabling triggers for schema '%s'\n", a_schema.c_str());
    logDebug("Retrieving trigger definitions\n");
    std::string get_trigger_definition = base::sqlstring("SELECT * FROM !.!", 0) << a_schema << TMP_TRIGGER_TABLE;

    if (mysql_query(&_mysql, get_trigger_definition.data()) != 0) {
      // It is ok if the table does not exist, it indicates there are
      // no triggers to restore
      if (mysql_errno(&_mysql) != 1146)
        throw ConnectionError("Querying Trigger Definitions", &_mysql);
      else
        logInfo("No triggers found for '%s'\n", a_schema.c_str());
    } else {
      MYSQL_RES *result;
      if ((result = mysql_use_result(&_mysql)) == NULL)
        throw ConnectionError("Getting Trigger Definitions", &_mysql);

      // Gets the trigger names so they can be backed up
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(result))) {
        trigger_name.push_back(row[0]);
        trigger_sql.push_back(row[1]);
      }

      mysql_free_result(result);

      // Restores the triggers
      logDebug("Configuring active schema\n");
      std::string select_database = base::sqlstring("USE !", 0) << a_schema;
      if (mysql_query(&_mysql, select_database.data()) != 0)
        throw ConnectionError("Selecting Database", &_mysql);

      int restored = 0;
      for (size_t trigger_index = 0; trigger_index != trigger_sql.size(); trigger_index++) {
        logDebug("Restoring trigger %s\n", trigger_name.at(trigger_index).c_str());

        std::string trigger_def(trigger_sql.at(trigger_index));
        if (mysql_query(&_mysql, trigger_def.c_str()) != 0) {
          // It is ok having an error for duplicated triggers
          if (mysql_errno(&_mysql) != 1235)
            throw ConnectionError("Restoring trigger", &_mysql);
          else
            logInfo("Trigger %s already existed, skipping...\n", trigger_name.at(trigger_index).c_str());
        } else
          restored++;
      }

      // Drops the trigger backup table
      drop_trigger_backups(a_schema);

      logInfo("Trigger Restore: %d succeeded, %lu failed\n", restored, (unsigned long)(trigger_sql.size() - restored));
    }
  }
}

TaskQueue::TaskQueue() {
}

void TaskQueue::add_task(const TableParam &task) {
  base::MutexLock lock(_task_mutex);
  _tasks.push_back(task);
}

bool TaskQueue::get_task(TableParam &task) {
  bool ret_val = false;

  base::MutexLock lock(_task_mutex);

  if (_tasks.size() > 0) {
    ret_val = true;
    task = _tasks.front();
    _tasks.erase(_tasks.begin());
  }

  return ret_val;
}

CopyDataTask::CopyDataTask(const std::string name, CopyDataSource *psource, MySQLCopyDataTarget *ptarget,
                           TaskQueue *ptasks, bool show_progress)
  : _source(psource), _target(ptarget) {
  _name = name;
  _tasks = ptasks;
  _show_progress = show_progress;

  _thread = base::create_thread(&CopyDataTask::thread_func, this);
}

gpointer CopyDataTask::thread_func(gpointer data) {
  CopyDataTask *self = (CopyDataTask *)data;

  TableParam tparam;

  while (self->_tasks->get_task(tparam)) {
    self->copy_table(tparam);
  }

  return NULL;
}

void CopyDataTask::copy_table(const TableParam &task) {
  std::shared_ptr<std::vector<ColumnInfo> > columns;

  long long i = 0, total = 0;
  int inserted_records;

  time_t start = time(NULL);
  try {
    std::vector<std::string> last_pkeys;
    if (task.copy_spec.resume)
      last_pkeys = _target->get_last_pkeys(task.target_pk_columns, task.target_schema, task.target_table);
    total =
      _source->count_rows(task.source_schema, task.source_table, task.source_pk_columns, task.copy_spec, last_pkeys);
    columns = _source->begin_select_table(task.source_schema, task.source_table, task.source_pk_columns,
                                          task.select_expression, task.copy_spec, last_pkeys);

    printf("BEGIN:%s.%s:Copying %li columns of %lli rows from table %s.%s\n", task.target_schema.c_str(),
           task.target_table.c_str(), (long)columns->size(), total, task.source_schema.c_str(),
           task.source_table.c_str());
    fflush(stdout);

    _target->set_get_field_lengths_from_target(_source->get_get_field_lengths_from_target());

    _target->set_target_table(task.target_schema, task.target_table, columns);

    _source->set_bulk_inserts(_target->bulk_inserts());

    _target->begin_inserts();
    while (_source->fetch_row(_target->row_buffer())) {
      inserted_records = _target->do_insert();
      i += inserted_records;

      if (_show_progress && inserted_records)
        report_progress(task.target_schema, task.target_table, i, total);

      _target->row_buffer().clear();

      if ((task.copy_spec.type == CopyCount && i >= task.copy_spec.row_count) ||
          (task.copy_spec.max_count > 0 && i >= task.copy_spec.max_count))
        break;
    }

    inserted_records = _target->end_inserts();
    i += inserted_records;

    if (_show_progress && inserted_records) {
      report_progress(task.target_schema, task.target_table, i, total);
    }

    _source->end_select_table();
  } catch (std::exception &e) {
    printf("ERROR:%s.%s:%s\n", task.target_schema.c_str(), task.target_table.c_str(), e.what());
    fflush(stdout);
    _target->end_inserts(false);
    _source->end_select_table();
  }

  time_t end = time(NULL);
  if (i != total)
    printf("ERROR:%s.%s:Failed copying %lli rows\n", task.target_schema.c_str(), task.target_table.c_str(), total - i);
  else
    printf("END:%s.%s:Finished copying %lli rows in %im%02is\n", task.target_schema.c_str(), task.target_table.c_str(),
           i, (int)((end - start) / 60), (int)((end - start) % 60));
  fflush(stdout);
}

void CopyDataTask::report_progress(const std::string &schema, const std::string &table, long long current,
                                   long long total) {
  printf("PROGRESS:%s.%s:%lli:%lli\n", schema.c_str(), table.c_str(), current, total);
  fflush(stdout);
}

CopyDataTask::~CopyDataTask() {
}

void MySQLCopyDataTarget::InsertBuffer::reset(size_t size) {
  length = 0;
  last_insert_length = 0;

  if (buffer) {
    if (size == this->size)
      return;
    free(buffer);
  }
  this->size = size;
  buffer = (char *)malloc(size);
  if (!buffer)
    throw std::runtime_error(base::strfmt("Not enough memory to allocate insert buffer of size %li", (long)size));
}

void MySQLCopyDataTarget::InsertBuffer::end_insert() {
  last_insert_length = length;
}

bool MySQLCopyDataTarget::InsertBuffer::append(const char *data, size_t dlength) {
  if (dlength > space_left())
    return false;
  memcpy(buffer + length, data, dlength);
  length += dlength;
  return true;
}

bool MySQLCopyDataTarget::InsertBuffer::append(const char *data) {
  return append(data, strlen(data));
}

bool MySQLCopyDataTarget::InsertBuffer::append_escaped(const char *data, size_t dlength) {
  // We need to check for the worst case scenario where all the
  // characters are escaped
  if ((dlength * 2) > space_left())
    return false;

  // This function is used to create a legal SQL string that you can use in an SQL statement
  // This is needed because the escaping depends on the character set in use by the server
  unsigned long ret_length = 0;



#if MYSQL_VERSION_ID >= 50706
  if (_target->is_mysql_version_at_least(5, 7, 6))
    ret_length += mysql_real_escape_string_quote(_mysql, buffer + length, data, (unsigned long)dlength, '\'');
  else
    ret_length += mysql_real_escape_string(_mysql, buffer + length, data, (unsigned long)dlength);
#else
  ret_length += mysql_real_escape_string(_mysql, buffer + length, data, (unsigned long)dlength);
#endif

  if (ret_length != (unsigned long)-1)
    length += ret_length;
  else
    throw std::runtime_error("mysql_real_escape_string: Cannot convert data to legal SQL string.");

  return true;
}

size_t MySQLCopyDataTarget::InsertBuffer::space_left() {
  return size - length;
}
