#ifndef _SQL_BATCH_EXEC_H_
#define _SQL_BATCH_EXEC_H_

#include "cppdbc_public_interface.h"
#include <cppconn/statement.h>
#include <cppconn/connection.h>
#include <list>
#include <string>
#include <functional>

namespace sql {

  class CPPDBC_PUBLIC_FUNC SqlBatchExec {
  public:
    SqlBatchExec();

  public:
    long operator()(sql::Statement *stmt, std::list<std::string> &statements);

  private:
    void exec_sql_script(sql::Statement *stmt, std::list<std::string> &statements, long &batch_exec_err_count);

  public:
    typedef std::function<int(long long, const std::string &, const std::string &)> Error_cb;
    typedef std::function<int(float)> Batch_exec_progress_cb;
    typedef std::function<int(long, long)> Batch_exec_stat_cb;

    Error_cb _error_cb;
    Batch_exec_progress_cb _batch_exec_progress_cb;
    Batch_exec_stat_cb _batch_exec_stat_cb;

    void error_cb(const Error_cb &cb) {
      _error_cb = cb;
    };
    void batch_exec_progress_cb(const Batch_exec_progress_cb &cb) {
      _batch_exec_progress_cb = cb;
    };
    void batch_exec_stat_cb(const Batch_exec_stat_cb &cb) {
      _batch_exec_stat_cb = cb;
    };

  private:
    long _batch_exec_success_count;
    long _batch_exec_err_count;
    float _batch_exec_progress_state;
    float _batch_exec_progress_inc;

  public:
    void stop_on_error(bool value) {
      _stop_on_error = value;
    }
    bool stop_on_error() {
      return _stop_on_error;
    }

  private:
    bool _stop_on_error;

  public:
    void failback_statements(const std::list<std::string> &value) {
      _failback_statements = value;
    }
    const std::list<std::string> &failback_statements() const {
      return _failback_statements;
    }

  private:
    std::list<std::string> _failback_statements;

  public:
    const std::list<std::string> &sql_log() const {
      return _sql_log;
    }

  private:
    std::list<std::string> _sql_log;
  };

} // namespace sql

#endif // _SQL_BATCH_EXEC_H_
