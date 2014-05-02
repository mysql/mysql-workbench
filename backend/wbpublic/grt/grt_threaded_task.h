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


#ifndef _GRT_THREADED_TASK_H_
#define _GRT_THREADED_TASK_H_

#include "base/trackable.h"
#include "wbpublic_public_interface.h"
#include "grt/grt_manager.h"
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


class WBPUBLICBACKEND_PUBLIC_FUNC GrtThreadedTask : public base::trackable
{
public:
  typedef boost::shared_ptr<GrtThreadedTask> Ref;
public:
  static Ref create() { return Ref(new GrtThreadedTask()); }
  static Ref create(bec::GRTManager *grtm) { return Ref(new GrtThreadedTask(grtm)); }
  static Ref create(const GrtThreadedTask::Ref &parent_task) { return Ref(new GrtThreadedTask(parent_task)); }
public:
  virtual ~GrtThreadedTask();
  void disconnect_callbacks();
protected:
  GrtThreadedTask();
  GrtThreadedTask(bec::GRTManager *grtm);
  GrtThreadedTask(const GrtThreadedTask::Ref &parent_task);

public:
  bec::GRTManager * grtm() const { return _grtm; }
  void grtm(bec::GRTManager *grtm);

  bool is_busy() { return _dispatcher && _dispatcher->get_busy(); }
private:
  bec::GRTManager *_grtm;

private:
  const bec::GRTDispatcher::Ref & dispatcher();
private:
  bec::GRTDispatcher::Ref _dispatcher;

private:
  bec::GRTTask *_task;
  GrtThreadedTask::Ref _parent_task;

public:
  const GrtThreadedTask::Ref & parent_task() const { return _parent_task; }
  void parent_task(const GrtThreadedTask::Ref &val);

  bec::GRTTask *task(); // Returns the underlying grt task.

private:
  void on_starting(bec::GRTTaskBase *task);
  void on_finishing(bec::GRTTaskBase *task);
  void on_failing(bec::GRTTaskBase *task);

public:
  std::string desc() { return _desc; }
  void desc(const std::string &desc) { _desc= desc; }
private:
  std::string _desc;

public:
  void send_task_res_msg(bool value) { _send_task_res_msg= value; }
private:
  bool _send_task_res_msg;

public:
  typedef boost::function<grt::StringRef (grt::GRT *)> Proc_cb;
  typedef boost::function<int (int, const std::string&, const std::string&)> Msg_cb;
  typedef boost::function<int (float, const std::string&)> Progress_cb;
  typedef boost::function<int ()> Finish_cb;
  typedef boost::function<int (const std::string&)> Fail_cb;

public:
  void exec(bool sync= false, Proc_cb proc_cb= Proc_cb());
  void send_msg(int msg_type, const std::string &msg, const std::string &detail= "");
  void send_progress(float percentage, const std::string &msg, const std::string &detail= "");

public:
  void msg_cb(Msg_cb cb) { _msg_cb= cb; }
  const Msg_cb &msg_cb() { return _msg_cb; }
  
  void progress_cb(Progress_cb cb) { _progress_cb= cb; }
  void finish_cb(Finish_cb cb) { _finish_cb= cb; }
  void fail_cb(Fail_cb cb) { _fail_cb= cb; }
  void proc_cb(Proc_cb cb) { _proc_cb= cb; }

private:
  void process_msg(const grt::Message &msgs, bec::GRTTask *task);
  void process_finish(grt::ValueRef res, bec::GRTTask *task);
  void process_fail(const std::exception &error, bec::GRTTask *task);

private:
  Proc_cb _proc_cb;
  Msg_cb _msg_cb;
  Progress_cb _progress_cb;
  Finish_cb _finish_cb;
  Fail_cb _fail_cb;

public:
  void execute_in_main_thread(const boost::function<void ()> &function, bool wait, bool force_queue);
};


#ifdef _WIN32
#pragma make_public(GrtThreadedTask)
#endif


#endif /* _GRT_THREADED_TASK_H_ */
