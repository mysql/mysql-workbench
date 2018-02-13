/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_plugin_be.h"
//--------------------------------------------------------------------------------------------------

void Wb_plugin::exec_task(bool sync) {
  set_task_proc();

  bec::GRTTask::Ref task =
    bec::GRTTask::create_task(task_desc(), bec::GRTManager::get()->get_dispatcher(), _task_proc_cb);

  scoped_connect(task->signal_message(), std::bind(&Wb_plugin::process_task_msg, this, std::placeholders::_1));
  scoped_connect(task->signal_failed(), std::bind(&Wb_plugin::process_task_fail, this, std::placeholders::_1));
  scoped_connect(task->signal_finished(), std::bind(&Wb_plugin::process_task_finish, this, std::placeholders::_1));

  if (sync)
    bec::GRTManager::get()->get_dispatcher()->add_task_and_wait(task);
  else
    bec::GRTManager::get()->get_dispatcher()->add_task(task);
}

//----------------------------------------------------------------------------------------------------------------------

void Wb_plugin::process_task_msg(const grt::Message &msg) {
  switch (msg.type) {
    case grt::WarningMsg:
    case grt::ErrorMsg:
    case grt::InfoMsg:
      if (_task_msg_cb)
        _task_msg_cb(msg.type, msg.text);
      break;
    case grt::ProgressMsg:
      if (_task_progress_cb)
        _task_progress_cb(msg.progress, msg.text);
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Wb_plugin::process_task_fail(const std::exception &error) {
  if (_task_fail_cb)
    _task_fail_cb(error.what());
}

//----------------------------------------------------------------------------------------------------------------------

void Wb_plugin::process_task_finish(grt::ValueRef res) {
  grt::GRT::get()->send_info(grt::StringRef::cast_from(res));
  bec::GRTManager::get()->perform_idle_tasks();
  if (_task_fail_cb)
    _task_finish_cb();
}

//----------------------------------------------------------------------------------------------------------------------

template <typename T1, typename T2>
T2 get_option(const grt::DictRef &options, const std::string &name) {
  T2 value = 0;
  if (options.is_valid() && options.has_key(name))
    value = (T2)T1::cast_from(options.get(name));
  return value;
}

int Wb_plugin::get_int_option(const std::string &name) {
  return get_option<grt::IntegerRef, int>(_options, name);
}

double Wb_plugin::get_double_option(const std::string &name) {
  return get_option<grt::DoubleRef, double>(_options, name);
}

std::string Wb_plugin::get_string_option(const std::string &name) {
  return get_option<grt::StringRef, std::string>(_options, name);
}

void Wb_plugin::set_option(const std::string &name, int val) {
  _options.set(name, grt::IntegerRef(val));
}

void Wb_plugin::set_option(const std::string &name, const double &val) {
  _options.set(name, grt::DoubleRef(val));
}

void Wb_plugin::set_option(const std::string &name, const std::string &val) {
  _options.set(name, grt::StringRef(val));
}
