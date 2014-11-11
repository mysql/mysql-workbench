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

#include "base/wb_memory.h"
#include "grt/grt_threaded_task.h"

//--------------------------------------------------------------------------------------------------

GrtThreadedTask::GrtThreadedTask()
:
_grtm(NULL),
_send_task_res_msg(true),
_onetime_finish_cb(false),
_onetime_fail_cb(false)
{
}

//--------------------------------------------------------------------------------------------------

GrtThreadedTask::GrtThreadedTask(bec::GRTManager *grtm)
:
_grtm(grtm),
_send_task_res_msg(true),
_onetime_finish_cb(false),
_onetime_fail_cb(false)
{
}

//--------------------------------------------------------------------------------------------------

GrtThreadedTask::GrtThreadedTask(const GrtThreadedTask::Ref parent_task)
:
_grtm(parent_task->grtm()),
_send_task_res_msg(true),
_onetime_finish_cb(false),
_onetime_fail_cb(false)
{
  this->parent_task(parent_task);
}

//--------------------------------------------------------------------------------------------------

GrtThreadedTask::~GrtThreadedTask()
{
  parent_task(GrtThreadedTask::Ref());
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::disconnect_callbacks()
{
  _proc_cb= Proc_cb();
  _msg_cb= Msg_cb();
  _progress_cb= Progress_cb();
  _finish_cb= Finish_cb();
  _fail_cb= Fail_cb();

  _send_task_res_msg= false;
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::grtm(bec::GRTManager *grtm)
{
  if (_grtm == grtm)
    return;
  _grtm= grtm;
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::parent_task(const GrtThreadedTask::Ref val)
{
  if (_dispatcher)
  {
    if (!_parent_task || (_parent_task->dispatcher() != _dispatcher))
      _dispatcher->shutdown();
    _dispatcher.reset();
  }
  _parent_task= val;
  disconnect_callbacks();
  if (_parent_task)
  {
    grtm(_parent_task->grtm());
    _dispatcher= _parent_task->dispatcher();
    _msg_cb = _parent_task->_msg_cb;
    _progress_cb = _parent_task->_progress_cb;
    _finish_cb = _parent_task->_finish_cb;
    _onetime_finish_cb = _parent_task->_onetime_finish_cb;
    _fail_cb = _parent_task->_fail_cb;
    _onetime_fail_cb = _parent_task->_onetime_fail_cb;
    _proc_cb = _parent_task->_proc_cb;
  }
}

//--------------------------------------------------------------------------------------------------

const bec::GRTDispatcher::Ref & GrtThreadedTask::dispatcher()
{
  if (!_dispatcher)
  {
    _dispatcher = bec::GRTDispatcher::create_dispatcher(_grtm->get_grt(), _grtm->is_threaded(), false);
    _dispatcher->set_main_thread_flush_and_wait(_grtm->get_dispatcher()->get_main_thread_flush_and_wait());
    _dispatcher->start();
  }
  return _dispatcher;
}

//--------------------------------------------------------------------------------------------------

const bec::GRTTask::Ref GrtThreadedTask::task()
{
  return (_task) ? _task : ((_parent_task) ? _parent_task->task() : bec::GRTTask::Ref());
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::exec(bool sync, Proc_cb proc_cb)
{
  if (!_grtm)
    return;
  if (proc_cb.empty())
    proc_cb= _proc_cb;
  if (proc_cb.empty())
    return;

  bec::GRTDispatcher::Ref dispatcher = this->dispatcher();

  _task = bec::GRTTask::create_task(desc(), dispatcher, proc_cb);

  scoped_connect(_task->signal_message(), boost::bind(&GrtThreadedTask::process_msg, this, _1));
  scoped_connect(_task->signal_failed(), boost::bind(&GrtThreadedTask::process_fail, this, _1));
  scoped_connect(_task->signal_finished(), boost::bind(&GrtThreadedTask::process_finish, this, _1));
  if (sync)
    dispatcher->add_task_and_wait(_task);
  else
    dispatcher->add_task(_task);
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::process_msg(const grt::Message &msg)
{
  switch (msg.type)
  {
  case grt::WarningMsg:
  case grt::ErrorMsg:
  case grt::InfoMsg:
    if(_msg_cb)
      _msg_cb(msg.type, msg.text, msg.detail);
    break;
  case grt::ProgressMsg:
    if(_progress_cb)
      _progress_cb(msg.progress, msg.text);
    break;
   default:
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::process_fail(const std::exception &error)
{
  if(_fail_cb)
  {
    _fail_cb(error.what());
    if (_onetime_fail_cb)
      _fail_cb = Fail_cb();
  }
  disconnect_scoped_connects();
  _task.reset();
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::process_finish(grt::ValueRef res)
{
  if (_send_task_res_msg)
  {
    grt::StringRef res_str= grt::StringRef::cast_from(res);
    if (!res_str.empty())
      _grtm->get_grt()->send_info(grt::StringRef::cast_from(res), "", NULL);
  }
  if (_finish_cb)
  {
    _finish_cb();
    if (_onetime_finish_cb)
      _finish_cb = Finish_cb();
  }

  disconnect_scoped_connects();
  _task.reset();
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::send_msg(int msg_type, const std::string &msg, const std::string &detail)
{
  if (!_grtm)
    return;

  if (_grtm->in_main_thread())
  {
     if(_msg_cb)
        _msg_cb(msg_type, msg, detail);
  }
  else
  {
    if (!task())
      return;
    grt::GRT *grt= _grtm->get_grt();
    switch (msg_type)
    {
    case grt::WarningMsg:
      grt->send_warning(msg, detail, task().get());
      break;
    case grt::ErrorMsg:
      grt->send_error(msg, detail, task().get());
      break;
    case grt::InfoMsg:
      grt->send_info(msg, detail, task().get());
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::send_progress(float percentage, const std::string &msg, const std::string &detail)
{
  if (!_grtm || _grtm->terminated())
    return;

  if (_grtm->in_main_thread())
  {
    if (_progress_cb)
      _progress_cb(percentage, msg);
  }
  else
  {
    if (!task())
      return;
    _grtm->get_grt()->send_progress(percentage, msg, detail, task().get());
  }
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::execute_in_main_thread(const boost::function<void()> &function, bool wait, bool force_queue)
{
  dispatcher()->call_from_main_thread<void>(function, wait, force_queue);
}

//--------------------------------------------------------------------------------------------------
