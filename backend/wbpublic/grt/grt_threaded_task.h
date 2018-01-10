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

#pragma once

#include "base/trackable.h"
#include "wbpublic_public_interface.h"
#include "grt/grt_manager.h"

class WBPUBLICBACKEND_PUBLIC_FUNC GrtThreadedTask : public base::trackable {
public:
  typedef std::shared_ptr<GrtThreadedTask> Ref;

public:
  static Ref create() {
    return Ref(new GrtThreadedTask());
  }
  static Ref create(const GrtThreadedTask::Ref parent_task) {
    return Ref(new GrtThreadedTask(parent_task));
  }

public:
  virtual ~GrtThreadedTask();
  void disconnect_callbacks();

protected:
  GrtThreadedTask();
  GrtThreadedTask(const GrtThreadedTask::Ref parent_task);

public:
  bool is_busy() {
    return _dispatcher && _dispatcher->get_busy();
  }

private:
  const bec::GRTDispatcher::Ref &dispatcher();

private:
  bec::GRTDispatcher::Ref _dispatcher;

private:
  bec::GRTTask::Ref _task;
  GrtThreadedTask::Ref _parent_task;

public:
  const GrtThreadedTask::Ref parent_task() const {
    return _parent_task;
  }
  void parent_task(const GrtThreadedTask::Ref val);

  const bec::GRTTask::Ref task(); // Returns the underlying grt task.

private:
  void on_starting(const bec::GRTTaskBase::Ref task);

public:
  std::string desc() {
    return _desc;
  }
  void desc(const std::string &desc) {
    _desc = desc;
  }

private:
  std::string _desc;

public:
  void send_task_res_msg(bool value) {
    _send_task_res_msg = value;
  }

private:
  bool _send_task_res_msg;

public:
  typedef std::function<grt::StringRef()> Proc_cb;
  typedef std::function<int(int, const std::string &, const std::string &)> Msg_cb;
  typedef std::function<int(float, const std::string &)> Progress_cb;
  typedef std::function<void()> Finish_cb;
  typedef std::function<void(const std::string &)> Fail_cb;

public:
  void exec(bool sync = false, Proc_cb proc_cb = Proc_cb());
  void send_msg(int msg_type, const std::string &msg, const std::string &detail = "");
  void send_progress(float percentage, const std::string &msg, const std::string &detail = "");

public:
  void msg_cb(Msg_cb cb) {
    _msg_cb = cb;
  }
  const Msg_cb &msg_cb() {
    return _msg_cb;
  }

  void progress_cb(Progress_cb cb) {
    _progress_cb = cb;
  }
  void finish_cb(Finish_cb cb, bool onetime = false) {
    _finish_cb = cb;
    _onetime_finish_cb = onetime;
  }
  void fail_cb(Fail_cb cb, bool onetime = false) {
    _fail_cb = cb;
    _onetime_fail_cb = onetime;
  }
  void proc_cb(Proc_cb cb) {
    _proc_cb = cb;
  }

private:
  void process_msg(const grt::Message &msgs);
  void process_finish(grt::ValueRef res);
  void process_fail(const std::exception &error);

private:
  Proc_cb _proc_cb;
  Msg_cb _msg_cb;
  Progress_cb _progress_cb;
  Finish_cb _finish_cb;
  bool _onetime_finish_cb;
  Fail_cb _fail_cb;
  bool _onetime_fail_cb;

public:
  void execute_in_main_thread(const std::function<void()> &function, bool wait, bool force_queue);
};
