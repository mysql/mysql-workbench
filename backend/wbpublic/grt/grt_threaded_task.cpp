/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/wb_memory.h"
#include "base/log.h"

#include "grt/grt_threaded_task.h"

DEFAULT_LOG_DOMAIN("GRT task")

//--------------------------------------------------------------------------------------------------

GrtThreadedTask::GrtThreadedTask() : _send_task_res_msg(true), _onetime_finish_cb(false), _onetime_fail_cb(false) {
}
//--------------------------------------------------------------------------------------------------

GrtThreadedTask::GrtThreadedTask(const GrtThreadedTask::Ref parent_task)
  : _send_task_res_msg(true), _onetime_finish_cb(false), _onetime_fail_cb(false) {
  this->parent_task(parent_task);
}

//--------------------------------------------------------------------------------------------------

GrtThreadedTask::~GrtThreadedTask() {
  parent_task(GrtThreadedTask::Ref());
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::disconnect_callbacks() {
  _proc_cb = Proc_cb();
  _msg_cb = Msg_cb();
  _progress_cb = Progress_cb();
  _finish_cb = Finish_cb();
  _fail_cb = Fail_cb();

  _send_task_res_msg = false;
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::parent_task(const GrtThreadedTask::Ref val) {
  if (_dispatcher) {
    if (!_parent_task || (_parent_task->dispatcher() != _dispatcher))
      _dispatcher->shutdown();
    _dispatcher.reset();
  }
  _parent_task = val;
  disconnect_callbacks();
  if (_parent_task) {
    _dispatcher = _parent_task->dispatcher();
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

const bec::GRTDispatcher::Ref &GrtThreadedTask::dispatcher() {
  if (!_dispatcher) {
    _dispatcher = bec::GRTDispatcher::create_dispatcher(bec::GRTManager::get()->is_threaded(), false);
    _dispatcher->set_main_thread_flush_and_wait(
      bec::GRTManager::get()->get_dispatcher()->get_main_thread_flush_and_wait());
    _dispatcher->start();
  }
  return _dispatcher;
}

//--------------------------------------------------------------------------------------------------

const bec::GRTTask::Ref GrtThreadedTask::task() {
  return (_task) ? _task : ((_parent_task) ? _parent_task->task() : bec::GRTTask::Ref());
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::exec(bool sync, Proc_cb proc_cb) {
  logDebug3("Sending task \"%s\" to dispatcher (%s)...\n", _desc.c_str(), sync ? "wait" : "don't wait");

  if (!proc_cb)
    proc_cb = _proc_cb;
  if (!proc_cb)
    return;

  bec::GRTDispatcher::Ref dispatcher = this->dispatcher();

  _task = bec::GRTTask::create_task(desc(), dispatcher, proc_cb);

  scoped_connect(_task->signal_message(), std::bind(&GrtThreadedTask::process_msg, this, std::placeholders::_1));
  scoped_connect(_task->signal_failed(), std::bind(&GrtThreadedTask::process_fail, this, std::placeholders::_1));
  scoped_connect(_task->signal_finished(), std::bind(&GrtThreadedTask::process_finish, this, std::placeholders::_1));
  if (sync)
    dispatcher->add_task_and_wait(_task);
  else
    dispatcher->add_task(_task);
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::process_msg(const grt::Message &msg) {
  switch (msg.type) {
    case grt::WarningMsg:
    case grt::ErrorMsg:
    case grt::InfoMsg:
      if (_msg_cb)
        _msg_cb(msg.type, msg.text, msg.detail);
      break;
    case grt::ProgressMsg:
      if (_progress_cb)
        _progress_cb(msg.progress, msg.text);
      break;
    default:
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::process_fail(const std::exception &error) {
  if (_fail_cb) {
    _fail_cb(error.what());
    if (_onetime_fail_cb)
      _fail_cb = Fail_cb();
  }
  disconnect_scoped_connects();
  _task.reset();
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::process_finish(grt::ValueRef res) {
  if (_send_task_res_msg) {
    grt::StringRef res_str = grt::StringRef::cast_from(res);
    if (!res_str.empty())
      grt::GRT::get()->send_info(grt::StringRef::cast_from(res), "", NULL);
  }
  if (_finish_cb) {
    _finish_cb();
    if (_onetime_finish_cb)
      _finish_cb = Finish_cb();
  }

  disconnect_scoped_connects();
  _task.reset();
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::send_msg(int msg_type, const std::string &msg, const std::string &detail) {
  if (bec::GRTManager::get()->in_main_thread()) {
    if (_msg_cb)
      _msg_cb(msg_type, msg, detail);
  } else {
    if (!task())
      return;
    switch (msg_type) {
      case grt::WarningMsg:
        grt::GRT::get()->send_warning(msg, detail, task().get());
        break;
      case grt::ErrorMsg:
        grt::GRT::get()->send_error(msg, detail, task().get());
        break;
      case grt::InfoMsg:
        grt::GRT::get()->send_info(msg, detail, task().get());
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::send_progress(float percentage, const std::string &msg, const std::string &detail) {
  if (bec::GRTManager::get()->terminated())
    return;

  if (bec::GRTManager::get()->in_main_thread()) {
    if (_progress_cb)
      _progress_cb(percentage, msg);
  } else {
    if (!task())
      return;
    grt::GRT::get()->send_progress(percentage, msg, detail, task().get());
  }
}

//--------------------------------------------------------------------------------------------------

void GrtThreadedTask::execute_in_main_thread(const std::function<void()> &function, bool wait, bool force_queue) {
  dispatcher()->call_from_main_thread<void>(function, wait, force_queue);
}

//--------------------------------------------------------------------------------------------------
