#include "stdafx.h"

#include "sql_batch_exec.h"
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <memory>

namespace sql
{


SqlBatchExec::SqlBatchExec()
:
_stop_on_error(true)
{
}


long SqlBatchExec::operator()(sql::Statement *stmt, std::list<std::string> &statements)
{
  _batch_exec_success_count= 0;
  _batch_exec_err_count= 0;
  _sql_log.clear();

  exec_sql_script(stmt, statements, _batch_exec_err_count);
  if (_batch_exec_err_count)
  {
    long failback_script_exec_err_count= 0;
    exec_sql_script(stmt, _failback_statements, failback_script_exec_err_count);
    _batch_exec_err_count+= failback_script_exec_err_count;
  }

  if(_batch_exec_stat_cb)
	_batch_exec_stat_cb(_batch_exec_success_count, _batch_exec_err_count);

  return _batch_exec_err_count;
}


void SqlBatchExec::exec_sql_script(sql::Statement *stmt, std::list<std::string> &statements, long &batch_exec_err_count)
{
  _batch_exec_progress_state= 0;
  _batch_exec_progress_inc= 1.f / statements.size();

  for (std::list<std::string>::const_iterator i= statements.begin(), i_end= statements.end(); i != i_end; ++i)
  {
    try
    {
      _sql_log.push_back(*i);
      if (stmt->execute(*i))
        std::auto_ptr<sql::ResultSet> rs(stmt->getResultSet());
      ++_batch_exec_success_count;
    }
    catch (SQLException &e)
    {
      ++batch_exec_err_count;
      if (_error_cb.empty())
        throw;
      else
      {
        if (&_batch_exec_err_count != &batch_exec_err_count) // applies only to failback scripts
          _error_cb(-1, "Error when running failback script. Details follow.", "");
        _error_cb(e.getErrorCode(), e.what(), *i);        
      }
    }
    _batch_exec_progress_state+= _batch_exec_progress_inc;
    if (_batch_exec_progress_cb)
      _batch_exec_progress_cb(_batch_exec_progress_state);
    
    if (batch_exec_err_count && _stop_on_error)
      break;
  }
}


} // namespace sql
