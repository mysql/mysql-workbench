/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "db_mysql_public_interface.h"
#include "grt/grt_manager.h"
#include "base/trackable.h"

class Wb_plugin;
#ifdef _WIN32
#pragma make_public(Wb_plugin)
#endif

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC Wb_plugin : public base::trackable {
public:
  Wb_plugin() {
    _options = grt::DictRef(true);
  }
  virtual ~Wb_plugin() {
  }

public:
  virtual std::string task_desc() = 0;
  void exec_task(bool sync = false);

protected:
  typedef std::function<grt::StringRef()> Task_proc_cb;
  virtual void set_task_proc() = 0;
  Task_proc_cb _task_proc_cb;

public:
  typedef std::function<int(int, const std::string &)> Task_msg_cb;
  typedef std::function<int(float, const std::string &)> Task_progress_cb;
  typedef std::function<int()> Task_finish_cb;
  typedef std::function<int(const std::string &)> Task_fail_cb;

  void task_msg_cb(Task_msg_cb cb) {
    _task_msg_cb = cb;
  }
  void task_progress_cb(Task_progress_cb cb) {
    _task_progress_cb = cb;
  }
  void task_finish_cb(Task_finish_cb cb) {
    _task_finish_cb = cb;
  }
  void task_fail_cb(Task_fail_cb cb) {
    _task_fail_cb = cb;
  }

  void process_task_msg(const grt::Message &msgs);
  void process_task_finish(grt::ValueRef res);
  void process_task_fail(const std::exception &error);

private:
  Task_msg_cb _task_msg_cb;
  Task_progress_cb _task_progress_cb;
  Task_finish_cb _task_finish_cb;
  Task_fail_cb _task_fail_cb;

public:
  void set_option(const std::string &name, int val);
  void set_option(const std::string &name, const double &val);
  void set_option(const std::string &name, const std::string &val);
  int get_int_option(const std::string &name);
  double get_double_option(const std::string &name);
  std::string get_string_option(const std::string &name);

protected:
  grt::DictRef _options;
};
