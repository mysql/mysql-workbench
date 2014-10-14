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

#include "base/threading.h"
#include "base/log.h"

#include "grt_dispatcher.h"
#include "grt_manager.h"
#include "grtpp_util.h"
#include "grtpp_shell.h"
#include "driver_manager.h"

#include "mforms/utilities.h"

using namespace bec;

DEFAULT_LOG_DOMAIN("GRTDispatcher");

// Extra debug print flag for additional information (not dynamically switchable like log calls).
static bool debug_dispatcher = false;

#ifdef __GNUC__
#define DPRINT(fmt, ...) if (debug_dispatcher) log_debug3(fmt,##__VA_ARGS__)
#else
#define DPRINT(...) if (debug_dispatcher) log_debug3(__VA_ARGS__)
#endif

// Helper structures to store shared pointers into an async queue.
struct CallbackHelper
{
  DispatcherCallbackBase::Ref callback;
  CallbackHelper(const DispatcherCallbackBase::Ref callback_)
    : callback(callback_)
  {
  }
};

struct GRTTaskHelper
{
  GRTTaskBase::Ref task;
  GRTTaskHelper(const GRTTaskBase::Ref task_)
    : task(task_)
  {
  }
};

struct GrtDispatcherHelper
{
  GRTDispatcher::Ref dispatcher;
  GrtDispatcherHelper(const GRTDispatcher::Ref dispatcher_)
    : dispatcher(dispatcher_)
  {
  }
};

//----------------- DispatcherCallback -------------------------------------------------------------

GRTTaskBase::~GRTTaskBase()
{
  delete _exception;
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::set_finished()
{
  _finished = true;
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::cancel()
{
  _cancelled = true;
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::started()
{
  signal_starting_task();
  _dispatcher->call_from_main_thread<void>(boost::bind(&GRTTaskBase::started_m, this), false, false);
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::started_m()
{
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::finished(const grt::ValueRef &result)
{
  signal_finishing_task();
  _dispatcher->call_from_main_thread<void>(boost::bind(&GRTTaskBase::finished_m, this, result), true, false);
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::finished_m(const grt::ValueRef &result)
{
  set_finished();
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::failed(const std::exception &exc)
{
  const grt::grt_runtime_error *rterr = dynamic_cast<const grt::grt_runtime_error*>(&exc);

  if (rterr)
    _exception = new grt::grt_runtime_error(*rterr);
  else
    _exception = new grt::grt_runtime_error(exc.what(), "");

  signal_failing_task();
  _dispatcher->call_from_main_thread<void>(boost::bind(&GRTTaskBase::failed_m, this, exc), false, false);
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::failed_m(const std::exception &exc)
{
  set_finished();
}

//--------------------------------------------------------------------------------------------------

bool GRTTaskBase::process_message(const grt::Message &msg)
{
  if (_messages_to_main_thread)
    _dispatcher->call_from_main_thread<void>(boost::bind(&GRTTaskBase::process_message_m, this, msg), false, false);
  else
    process_message_m(msg);

  return true;
}

//--------------------------------------------------------------------------------------------------

void GRTTaskBase::process_message_m(const grt::Message &msg)
{
}

//----------------- GrtNullTask --------------------------------------------------------------------

class GrtNullTask : public GRTTaskBase
{
public:
  GrtNullTask(const GRTDispatcher::Ref dispatcher) : GRTTaskBase("Terminate Worker Thread", dispatcher) {}
  virtual void finished(const grt::ValueRef &result) {}
  virtual grt::ValueRef execute(grt::GRT *)
  {
    _result = grt::ValueRef();
    return _result;
  }
};

//--------------------------------------------------------------------------------------------------

class GRTSimpleTask : public GRTTaskBase
{
public:
  typedef std::shared_ptr<GRTSimpleTask> Ref;

  static Ref create_task(const std::string &name, const GRTDispatcher::Ref dispatcher,
    const boost::function<grt::ValueRef(grt::GRT*)> &function)
  {
    return Ref(new GRTSimpleTask(name, dispatcher, function));
  }

protected:
  GRTSimpleTask(const std::string &name, const GRTDispatcher::Ref dispatcher,
    const boost::function<grt::ValueRef (grt::GRT*)> &function)
    : GRTTaskBase(name, dispatcher), _function(function)
  {
  }

  grt::ValueRef execute(grt::GRT *grt)
  {
    try
    {
      _result = _function(grt);
    }
    catch (const std::exception &e)
    {
      _result = grt::ValueRef();
      failed(e);
    }
    return _result;
  }

  virtual void started() {}
  virtual void finished(const grt::ValueRef &result) { set_finished(); }

  virtual void failed(const std::exception &exc)
  {
    const grt::grt_runtime_error *rterr= dynamic_cast<const grt::grt_runtime_error*>(&exc);

    if (rterr)
      _exception= new grt::grt_runtime_error(*rterr);
    else
      _exception= new grt::grt_runtime_error(exc.what(), "");
  }

private:
  boost::function<grt::ValueRef (grt::GRT*)> _function;
};

//----------------- GRTTask ------------------------------------------------------------------------

GRTTask::GRTTask(const std::string &name, const GRTDispatcher::Ref dispatcher,
  const boost::function<grt::ValueRef(grt::GRT*)> &function)
  : GRTTaskBase(name, dispatcher), _function(function)
{
}

//--------------------------------------------------------------------------------------------------

GRTTask::Ref GRTTask::create_task(const std::string &name, const GRTDispatcher::Ref dispatcher,
  const boost::function<grt::ValueRef(grt::GRT*)> &function)
{
  return Ref(new GRTTask(name, dispatcher, function));
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef GRTTask::execute(grt::GRT *grt)
{
  _result = _function(grt);
  return _result;
}

//--------------------------------------------------------------------------------------------------

void GRTTask::started_m()
{
  _started();
}

//--------------------------------------------------------------------------------------------------

void GRTTask::finished_m(const grt::ValueRef &result)
{
  _finished(result);

  GRTTaskBase::finished_m(result);
}

//--------------------------------------------------------------------------------------------------

void GRTTask::failed_m(const std::exception &error)
{
  _failed(*_exception);
 
  GRTTaskBase::failed_m(*_exception);
}

//--------------------------------------------------------------------------------------------------

bool GRTTask::process_message(const grt::Message &msg)
{
  if (_message.empty())
    return false;
  
  return GRTTaskBase::process_message(msg);
}

//--------------------------------------------------------------------------------------------------

void GRTTask::process_message_m(const grt::Message &msgs)
{
  _message(msgs);
}

//----------------- GRTShellTask -------------------------------------------------------------------

GRTShellTask::GRTShellTask(const std::string &name, const GRTDispatcher::Ref dispatcher,
  const std::string &command)
  : GRTTaskBase(name, dispatcher)
{
  _command = command;
}

//--------------------------------------------------------------------------------------------------

GRTShellTask::Ref GRTShellTask::create_task(const std::string &name, const GRTDispatcher::Ref dispatcher,
  const std::string &command)
{
  return Ref(new GRTShellTask(name, dispatcher, command));
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef GRTShellTask::execute(grt::GRT *grt)
{
  _result = grt->get_shell()->execute(_command);
  _prompt = grt->get_shell()->get_prompt();

  return grt::ValueRef();
}

//--------------------------------------------------------------------------------------------------

void GRTShellTask::finished_m(const grt::ValueRef &result)
{
  _finished_signal(_result, _prompt);
  
  GRTTaskBase::finished_m(result);
}

//--------------------------------------------------------------------------------------------------

bool GRTShellTask::process_message(const grt::Message &msg)
{
  if (_message.empty())
    return false;
  
  return GRTTaskBase::process_message(msg);
}

//--------------------------------------------------------------------------------------------------

void GRTShellTask::process_message_m(const grt::Message &msg)
{
  _message(msg);
}

//----------------- GRTDispatcher ------------------------------------------------------------------

static void sleep_2ms()
{
  g_usleep(2000);
}

static GThread *_main_thread = NULL;

GRTDispatcher::GRTDispatcher(grt::GRT *grt, bool threaded, bool is_main_dispatcher)
  : _busy(0), _threading_disabled(!threaded), _w_runing(0), _is_main_dispatcher(is_main_dispatcher),
  _shut_down(false), _grt(grt)
{
  _shutdown_callback = false;

  if (threaded)
  {
    _task_queue = g_async_queue_new();
    _callback_queue = g_async_queue_new();
  }
  else
  {
    _task_queue = NULL;
    _callback_queue = NULL;
  }
  _thread= 0;
  if (_is_main_dispatcher) // Assuming main dispatcher is created from main thread.
    _main_thread = g_thread_self();
  
  // default implementation just sleeps for 2ms
  _flush_main_thread_and_wait= sleep_2ms;

  if (getenv("WB_DEBUG_DISPATCHER"))
    debug_dispatcher= true;
}

//--------------------------------------------------------------------------------------------------

GRTDispatcher::~GRTDispatcher()
{
  shutdown();

  if (_task_queue)
    g_async_queue_unref(_task_queue);
  if (_callback_queue)
    g_async_queue_unref(_callback_queue);
}

//--------------------------------------------------------------------------------------------------

GRTDispatcher::Ref GRTDispatcher::create_dispatcher(grt::GRT *grt, bool threaded, bool is_main_dispatcher)
{
  return Ref(new GRTDispatcher(grt, threaded, is_main_dispatcher));
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::start()
{
  _shut_down = false;
  if (!_threading_disabled)
  {
    log_debug("starting worker thread\n");

    GrtDispatcherHelper *helper = new GrtDispatcherHelper(shared_from_this());
    _thread= base::create_thread(worker_thread, helper);
    if (_thread == 0)
    {
      log_error("base::create_thread failed to create the GRT worker thread. Falling back into non-threaded mode.\n");
      _threading_disabled = true;
    }
  }

  bec::GRTManager *grtm = bec::GRTManager::get_instance_for(_grt);
  if (grtm) // in tests, grtm may not exist
    grtm->add_dispatcher(shared_from_this());

  if (_is_main_dispatcher)
    _grt->push_message_handler(boost::bind(&GRTDispatcher::message_callback, this, _1, _2));
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::shutdown()
{
  if (_shut_down)
      return;
  _shut_down = true;
  if (_is_main_dispatcher)
    _grt->pop_message_handler();

  _shutdown_callback= true;
  if (!_threading_disabled && _thread != 0) // _thread == 0, means that init was not called, but threading_disabled was set to false.
  {
    std::shared_ptr<GrtNullTask> task(new GrtNullTask(shared_from_this()));
    add_task(task);
    log_debug2("GRTDispatcher:Main thread waiting for worker to finish\n");
    _w_runing.wait();
    log_debug2("GRTDispatcher:Main thread worker finished\n");
  }

  bec::GRTManager *grtm = bec::GRTManager::get_instance_for(_grt);
  if (grtm)
    grtm->remove_dispatcher(shared_from_this());
}

//--------------------------------------------------------------------------------------------------

gpointer GRTDispatcher::worker_thread(gpointer data)
{
  GrtDispatcherHelper *helper = static_cast<GrtDispatcherHelper *>(data);
  GRTDispatcher::Ref self = helper->dispatcher;
  delete helper;

  GAsyncQueue *task_queue= self->_task_queue;
  GAsyncQueue *callback_queue= self->_callback_queue;

  mforms::Utilities::set_thread_name("GRTDispatcher");

  log_debug("worker thread running\n");
  
  g_async_queue_ref(task_queue);
  g_async_queue_ref(callback_queue);

  self->worker_thread_init();
  
  while (true)
  {
    GRTTaskBase::Ref task;

    self->worker_thread_iteration();

    // pop next task pushed to queue by the main thread

#if GLIB_CHECK_VERSION(2, 32, 0)
    GRTTaskHelper *helper = static_cast<GRTTaskHelper *>(g_async_queue_timeout_pop(task_queue, 1000000));
    if (helper == NULL)
      continue;
    task = helper->task;
    delete helper;
#else
    GTimeVal timeout;
    g_get_current_time(&timeout);
    timeout.tv_sec+= 1;

    GRTTaskHelper *helper = static_cast<GRTTaskHelper *>(g_async_queue_timed_pop(task_queue, &timeout));
    if (helper == NULL)
      continue;
    task = helper->task;
    delete helper;
#endif

    g_atomic_int_inc(&self->_busy);
    log_debug3("GRT dispatcher, running task %s", task->name().c_str());

    if (dynamic_cast<GrtNullTask*>(task.get()) != 0) // a NULL task terminates the thread
    {
      DPRINT("worker: termination task received, closing...");
      task->finished(grt::ValueRef());
      g_atomic_int_dec_and_test(&self->_busy);
      break;
    }

    if (task->is_cancelled())
    {
      DPRINT("%s", std::string("worker: task '"+task->name()+"' was cancelled.").c_str());
      g_atomic_int_dec_and_test(&self->_busy);
      continue;
    }
    
    int count = self->grt()->message_handler_count();

    // do pre-execution preparations
    self->prepare_task(task);

    // execute the task
    self->execute_task(task);

    if (task->get_error())
    {
      log_error("%s\n", std::string(("worker: task '"+task->name()+"' has failed with error:.")+task->get_error()->what()).c_str());
      g_atomic_int_dec_and_test(&self->_busy);
      continue;
    }

    if (count != self->grt()->message_handler_count())
    {
      log_error("INTERNAL ERROR: Message handler count mismatch after executing task '%s' (%i vs %i)",
        task->name().c_str(), count, self->grt()->message_handler_count());
    }

    g_atomic_int_dec_and_test(&self->_busy);
    DPRINT("worker: task finished.");
  }

  self->worker_thread_release();

  g_async_queue_unref(task_queue);
  g_async_queue_unref(callback_queue);

  self->_w_runing.post();

  log_debug("worker thread exiting...\n");

  return NULL;
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::execute_now(const GRTTaskBase::Ref task)
{
  g_atomic_int_inc(&_busy);
  prepare_task(task);
  
  execute_task(task);
  
  g_atomic_int_dec_and_test(&_busy);
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::add_task(const GRTTaskBase::Ref task)
{
  // If threading is disabled or the worker thread is calling another 
  // task, we have to execute it immediately otherwise we'd just deadlock.
  if (_threading_disabled || _thread == g_thread_self())
    execute_now(task);
  else
  {
    GRTTaskHelper *helper = new GRTTaskHelper(task);
    g_async_queue_push(_task_queue, helper);
  }
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::cancel_task(const GRTTaskBase::Ref task)
{
  task->cancel();
}

//--------------------------------------------------------------------------------------------------

bool GRTDispatcher::get_busy()
{
  return (_task_queue && g_async_queue_length(_task_queue) > 0) || g_atomic_int_get(&_busy);
}

//--------------------------------------------------------------------------------------------------

/** Optional callback to wait and flush stuff in main thread.
 *
 * This callback is called when the main thread is waiting on some task to finish.
 * It should at least put the main thread to sleep so that the wait loop
 * doesn't use 100% cpu. It can also perform certain tasks like responding to 
 * screen redraw requests, flushing performSelectorOnMainThread queues (in MacOSX)
 * etc. It should not handle mouse and keyboard events or anything that could
 * cause another call to the backend.
 */
void GRTDispatcher::set_main_thread_flush_and_wait(FlushAndWaitCallback callback)
{
  _flush_main_thread_and_wait= callback;
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::flush_pending_callbacks()
{
  if (_callback_queue)
  {
    while (true)
    {
      CallbackHelper *helper = static_cast<CallbackHelper *>(g_async_queue_try_pop(_callback_queue));
      if (helper == NULL)
        break;

      DispatcherCallbackBase::Ref callback = helper->callback;
      delete helper;

      // Don't run any task, but clear the queue if we are shutting down.
      // This way the NullTask can surface up in the worker thread.
      if (!_shutdown_callback)
        callback->execute();
      callback->signal();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::call_from_main_thread(const DispatcherCallbackBase::Ref callback, bool wait, bool force_queue)
{
  bool is_main_thread= (g_thread_self() == _main_thread);
  if (force_queue && is_main_thread)
    wait= false;

  if (!force_queue && (_threading_disabled || is_main_thread))
  {
    callback->execute();
    callback->signal();
    wait = false;
  }
  else
  {
    CallbackHelper *helper = new CallbackHelper(callback);
    g_async_queue_push(_callback_queue, helper);
  }

  if (wait)
    callback->wait();
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::worker_thread_init()
{
//QQQ  _grt->enable_thread_notifications();
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::worker_thread_release()
{
  mforms::Utilities::driver_shutdown();
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::worker_thread_iteration()
{
//QQQ  _grt->flush_notifications();
}

//--------------------------------------------------------------------------------------------------

bool GRTDispatcher::message_callback(const grt::Message &msgs, void *sender)
{
  if (sender == NULL)
  {
    if (_current_task)
      return _current_task->process_message(msgs);
    return false; // Let it bubble up by default.
  }

  GRTTaskHelper *helper = static_cast<GRTTaskHelper*>(sender);
  return helper->task->process_message(msgs);
}

//--------------------------------------------------------------------------------------------------

static bool call_process_message(const grt::Message &msgs, void *sender, const GRTTaskBase::Ref task)
{
  if (sender != NULL)
  {
    GRTTaskHelper *helper = static_cast<GRTTaskHelper*>(sender);
    return helper->task->process_message(msgs);
  }
  return task->process_message(msgs);
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::prepare_task(const GRTTaskBase::Ref gtask)
{
  _current_task = gtask;
  
  // Directly set the task callbacks.
  if (_is_main_dispatcher)
    _grt->push_message_handler(boost::bind(call_process_message, _1, _2, gtask));
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::restore_callbacks(const GRTTaskBase::Ref task)
{
  // Restore originally set msg callbacks.
  if (_is_main_dispatcher)
    _grt->pop_message_handler();
 
  _current_task.reset();
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::execute_task(const GRTTaskBase::Ref gtask)
{
  try
  {
    gtask->started();
    grt::ValueRef result = gtask->execute(_grt);

    restore_callbacks(gtask);
    gtask->finished(result);
  }
  catch (std::exception &error)
  {
    log_exception("exception in grt execute_task, continuing",error);
    restore_callbacks(gtask);
    gtask->failed(error);
  }
  catch (std::exception *error)
  {
    log_exception("exception in grt execute_task, continuing",*error);
    restore_callbacks(gtask);
    gtask->failed(*error);
  }
  catch (...)
  {
    log_error("Unknown exception in grt execute_task.");
    restore_callbacks(gtask);
    gtask->failed(std::runtime_error("Unknown reason"));
  }
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::wait_task(const GRTTaskBase::Ref task)
{
  bool is_main_thread = g_thread_self() == _main_thread;
  
  // wait for a task to be completed, making sure that
  // the task won't deadlock because of a call to 
  // call_from_main_thread()

  while (!task->is_finished() && !task->is_cancelled())
  {
    flush_pending_callbacks();
    
    if (_flush_main_thread_and_wait && is_main_thread)
      _flush_main_thread_and_wait();
  }
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef GRTDispatcher::add_task_and_wait(const GRTTaskBase::Ref task) THROW(grt::grt_runtime_error)
{
#if 0
  if (is_busy())
  {
    g_error("The GRT dispatcher is currently executing another task.\n"
            "Calling add_task_and_wait() while another task is running can lead\n"
            "to deadlocks and is not allowed.");
    return grt::ValueRef();
  }
#endif

  add_task(task);
  
  // wait for the task to finish executing
  wait_task(task);
  
  // if there was an exception during execution in the worker thread,
  // we throw an exception signaling its failure
  if (task->get_error())
  {
    grt::grt_runtime_error error= *task->get_error();
    throw error;
  }

  return task->result();
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef GRTDispatcher::execute_simple_function(const std::string &name,
  const boost::function<grt::ValueRef (grt::GRT*)> &function) THROW (grt::grt_runtime_error)
{
  GRTSimpleTask::Ref task(GRTSimpleTask::create_task(name, shared_from_this(), function));
  add_task_and_wait(task);

  return task->result();
}

//--------------------------------------------------------------------------------------------------

void GRTDispatcher::execute_async_function(const std::string &name,
  const boost::function<grt::ValueRef(grt::GRT*)> &function) THROW (grt::grt_runtime_error)
{
  add_task(GRTSimpleTask::create_task(name, shared_from_this(), function));
}

//--------------------------------------------------------------------------------------------------
