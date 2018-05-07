/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MSC_VER

#include <sql.h>
#include <sqlext.h>

#include <errno.h>
#include <stdlib.h>

#include <vector>
#include <set>
#include <map>
#include <string>
#include <stdexcept>
#include <memory>
#include <functional>

#ifdef __APPLE
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif

#endif

#include "converter.h"
#include "glib.h"
#include "base/threading.h"

class QueryBuilder {
public:
  void select_columns(const std::string &columns) {
    _columns = columns;
  };
  void select_from_table(const std::string &table, const std::string &schema = "") {
    _table = table;
    _schema = schema;
  };
  void add_limit(const std::string &limit) {
    _limit = limit;
  };
  void add_orderby(const std::string &orderby) {
    _orderby = orderby;
  };
  void add_where(const std::string &where) {
    _where.push_back(where);
  };
  std::string build_query();

private:
  std::string _orderby;
  std::string _limit;
  std::string _schema;
  std::string _table;
  std::string _columns;
  std::vector<std::string> _where;
};

class ConnectionError : public std::runtime_error {
  static std::string process(SQLRETURN retcode, SQLSMALLINT htype, SQLHANDLE handle);

public:
  ConnectionError(const std::string &what, SQLRETURN ret, SQLSMALLINT htype, SQLHANDLE handle)
    : std::runtime_error(what + ": " + process(ret, htype, handle)) {
  }

  ConnectionError(const std::string &what, MYSQL *m) : std::runtime_error(what + ": " + mysql_error(m)) {
  }

  ConnectionError(const std::string &what, MYSQL_STMT *m) : std::runtime_error(what + ": " + mysql_stmt_error(m)) {
  }

  ConnectionError(const std::string &what, const std::string &error) : std::runtime_error(what + ": " + error) {
  }
};

enum SourceType { ST_MYSQL, ST_ODBC, ST_PYTHON };

struct ColumnInfo {
  std::string source_name;
  std::string source_type;
  enum enum_field_types mapped_source_type;
  unsigned long long source_length;

  std::string target_name;
  enum enum_field_types target_type;
  bool is_unsigned;
  bool is_long_data;
};

class RowBuffer : public std::vector<MYSQL_BIND> {
  int _current_field;
  std::function<void(int, const char *, size_t)> _send_blob_data;

  RowBuffer(const RowBuffer &o) : std::vector<MYSQL_BIND>(), _current_field(0) {
  }

public:
  RowBuffer(std::shared_ptr<std::vector<ColumnInfo> > columns,
            std::function<void(int, const char *, size_t)> send_blob_data, size_t max_packet_size);
  ~RowBuffer();

  void clear();

  void prepare_add_string(char *&buffer, size_t &buffer_len, unsigned long *&length);
  void prepare_add_float(char *&buffer, size_t &buffer_len);
  void prepare_add_double(char *&buffer, size_t &buffer_len);
  void prepare_add_bigint(char *&buffer, size_t &buffer_len);
  void prepare_add_long(char *&buffer, size_t &buffer_len);
  void prepare_add_short(char *&buffer, size_t &buffer_len);
  void prepare_add_tiny(char *&buffer, size_t &buffer_len);
  void prepare_add_time(char *&buffer, size_t &buffer_len);
  void prepare_add_geometry(char *&buffer, size_t &buffer_len, unsigned long *&length);
  void finish_field(bool was_null);

  enum enum_field_types target_type(bool &unsig);

  bool check_if_blob();
  void send_blob_data(const char *data, size_t length);
};

enum CopyType { CopyAll, CopyRange, CopyCount, CopyWhere };

struct CopySpec {
  CopyType type;

  std::string range_key;
  std::string where_expression;
  long long range_start;
  long long range_end;
  long long row_count;
  long long max_count;
  bool resume;
};

struct TableParam {
  std::string source_schema;
  std::string source_table;
  std::string target_schema;
  std::string target_table;
  std::string select_expression;
  std::vector<std::string> source_pk_columns;
  std::vector<std::string> target_pk_columns;
  CopySpec copy_spec;
};

class CopyDataSource {
protected:
  std::string _schema_name;
  std::string _table_name;
  int _block_size;
  size_t _max_blob_chunk_size;
  std::vector<char> _blob_buffer;
  long long _max_parameter_size;
  bool _abort_on_oversized_blobs;
  bool _use_bulk_inserts;
  bool _get_field_lengths_from_target;
  unsigned int _connection_timeout;

public:
  CopyDataSource();
  virtual ~CopyDataSource(){};

  void set_block_size(int bsize);
  void set_max_blob_chunk_size(size_t size);
  void set_max_parameter_size(unsigned long size) {
    _max_parameter_size = size;
  }
  void set_abort_on_oversized_blobs(bool value) {
    _abort_on_oversized_blobs = value;
  }
  void set_get_field_lengths_from_target(bool value) {
    _get_field_lengths_from_target = value;
  }
  bool get_get_field_lengths_from_target() {
    return _get_field_lengths_from_target;
  }
  void set_bulk_inserts(bool value) {
    _use_bulk_inserts = value;
  }
  std::string get_where_condition(const std::vector<std::string> &pk_columns,
                                  const std::vector<std::string> &last_pkeys);

  virtual size_t count_rows(const std::string &schema, const std::string &table,
                            const std::vector<std::string> &pk_columns, const CopySpec &spec,
                            const std::vector<std::string> &last_pkeys) = 0;
  virtual std::shared_ptr<std::vector<ColumnInfo> > begin_select_table(
    const std::string &schema, const std::string &table, const std::vector<std::string> &pk_columns,
    const std::string &select_expression, const CopySpec &spec, const std::vector<std::string> &last_pkeys) = 0;
  virtual void end_select_table() = 0;
  virtual bool fetch_row(RowBuffer &rowbuffer) = 0;
};

class ODBCCopyDataSource : public CopyDataSource {
  SQLHDBC _dbc;
  std::string _connstring;

  SQLHSTMT _stmt;
  std::shared_ptr<std::vector<ColumnInfo> > _columns;
  std::vector<SQLSMALLINT> _column_types;

  bool _stmt_ok;
  int _column_count;
  bool _force_utf8_input;

  std::string _source_rdbms_type;

  SQLSMALLINT odbc_type_to_c_type(SQLSMALLINT type, bool is_unsigned);

  void ucs2_to_utf8(char *inbuf, size_t inbuf_len, char *&utf8buf, size_t &utf8buf_len);

public:
  ODBCCopyDataSource(SQLHENV env, const std::string &connstring, const std::string &password, bool force_utf8_input,
                     const std::string &source_rdbms_type);
  virtual ~ODBCCopyDataSource();

  SQLRETURN get_wchar_buffer_data(RowBuffer &rowbuffer, int column);
  SQLRETURN get_char_buffer_data(RowBuffer &rowbuffer, int column);
  SQLRETURN get_date_time_data(RowBuffer &rowbuffer, int column, int type);
  SQLRETURN get_geometry_buffer_data(RowBuffer &rowbuffer, int column);

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

class MySQLCopyDataSource : public CopyDataSource {
  MYSQL _mysql;
  MYSQL_STMT *_select_stmt;
  bool _has_long_data;

public:
  MySQLCopyDataSource(const std::string &hostname, int port, const std::string &username, const std::string &password,
                      const std::string &socket, bool use_cleartext_plugin, const unsigned int connection_timeout);
  virtual ~MySQLCopyDataSource();

  virtual size_t count_rows(const std::string &schema, const std::string &table,
                            const std::vector<std::string> &pk_columns, const CopySpec &spec,
                            const std::vector<std::string> &last_pkeys);
  virtual std::shared_ptr<std::vector<ColumnInfo> > begin_select_table(
    const std::string &schema, const std::string &table, const std::vector<std::string> &pk_columns,
    const std::string &select_expression, const CopySpec &spec, const std::vector<std::string> &last_pkeys);
  virtual void end_select_table();
  virtual bool fetch_row(RowBuffer &rowbuffer);
};

class MySQLCopyDataTarget {
  struct InsertBuffer {
    MYSQL *_mysql;
    MySQLCopyDataTarget *_target;
    char *buffer;
    size_t length;
    size_t size;
    size_t last_insert_length;

    InsertBuffer(MySQLCopyDataTarget *target)
      : _target(target), buffer(NULL), length(0), size(0), last_insert_length(0) {
    }
    ~InsertBuffer() {
      if (buffer)
        free(buffer);
    }
    void reset(size_t size);
    void end_insert();

    bool append(const char *data, size_t length);
    bool append(const char *data);
    bool append_escaped(const char *data, size_t length);
    void set_connection(MYSQL *mysql) {
      _mysql = mysql;
    }
    size_t space_left();
  };

  MYSQL _mysql;
  MYSQL_STMT *_insert_stmt;
  std::string _incoming_data_charset;
  unsigned long _max_allowed_packet;
  unsigned long _max_long_data_size;
  std::string _schema;
  std::string _table;
  std::shared_ptr<std::vector<ColumnInfo> > _columns;
  RowBuffer *_row_buffer;
  bool _truncate;
  int _major_version;
  int _minor_version;
  int _build_version;

  // Variables used for bulk inserts
  bool _use_bulk_inserts;
  bool _init_bulk_insert;
  bool _get_field_lengths_from_target;
  std::string _bulk_insert_query;
  InsertBuffer _bulk_insert_buffer;
  InsertBuffer _bulk_insert_record;
  int _bulk_record_count;
  int _bulk_insert_batch;
  std::string _source_rdbms_type;
  unsigned int _connection_timeout;

  MYSQL_RES *get_server_value(const std::string &variable);
  void get_server_value(const std::string &variable, std::string &value);
  void get_server_value(const std::string &variable, unsigned long &value);
  bool format_bulk_record();
  bool append_bulk_column(size_t col_index);

  void get_server_version();
  bool is_mysql_version_at_least(const int _major, const int _minor, const int _build);
  void send_long_data(int column, const char *data, size_t length);

  void init();
  std::string ps_query();
  enum enum_field_types field_type_to_ps_param_type(enum enum_field_types ftype);

  void get_generated_columns(const std::string &schema, const std::string &table, std::vector<std::string> &gc);

public:
  MySQLCopyDataTarget(const std::string &hostname, int port, const std::string &username, const std::string &password,
                      const std::string &socket, bool use_cleartext_plugin, const std::string &app_name,
                      const std::string &incoming_charset, const std::string &source_rdbms_type,
                      const unsigned int connection_timeout);

  ~MySQLCopyDataTarget();

  size_t get_max_allowed_packet() {
    return _max_allowed_packet;
  }
  size_t get_max_long_data_size() {
    return _max_long_data_size;
  }

  void set_truncate(bool flag);

  void set_target_table(const std::string &schema, const std::string &table,
                        std::shared_ptr<std::vector<ColumnInfo> > columns);
  long long get_max_value(const std::string &key);

  bool bulk_inserts() {
    return _use_bulk_inserts;
  }
  void set_bulk_insert_batch_size(int value) {
    _bulk_insert_batch = value;
  }

  bool get_get_field_lengths_from_target() {
    return _get_field_lengths_from_target;
  }
  void set_get_field_lengths_from_target(bool value) {
    _get_field_lengths_from_target = value;
  }

  void begin_inserts();
  int end_inserts(bool flush = true);
  int do_insert(bool final = false);

  void restore_triggers(std::set<std::string> &schemas);
  void backup_triggers(std::set<std::string> &schemas);
  void backup_triggers_for_schema(const std::string &schema);
  void get_triggers_for_schema(const std::string &schema, std::map<std::string, std::string> &triggers);
  bool get_trigger_definitions_for_schema(const std::string &schema, std::map<std::string, std::string> &triggers);
  void drop_trigger_backups(const std::string &schema);
  std::vector<std::string> get_last_pkeys(const std::vector<std::string> &pk_columns, const std::string &schema,
                                          const std::string &table);

  RowBuffer &row_buffer();
};

class TaskQueue {
private:
  std::vector<TableParam> _tasks;
  base::Mutex _task_mutex;

public:
  TaskQueue();
  void add_task(const TableParam &task);
  bool get_task(TableParam &task);

  size_t size() {
    return _tasks.size();
  }
  bool empty() {
    return _tasks.empty();
  }
};

class CopyDataTask {
private:
  std::string _name;
  std::unique_ptr<CopyDataSource> _source;
  std::unique_ptr<MySQLCopyDataTarget> _target;
  TaskQueue *_tasks;
  bool _show_progress;

  GThread *_thread;

  static gpointer thread_func(gpointer data);

  void copy_table(const TableParam &task);

  void report_progress(const std::string &schema, const std::string &table, long long current, long long total);

public:
  CopyDataTask(const std::string name, CopyDataSource *psource, MySQLCopyDataTarget *ptarget, TaskQueue *ptasks,
               bool show_progress);
  ~CopyDataTask();
  void wait() {
    g_thread_join(_thread);
  }
};
