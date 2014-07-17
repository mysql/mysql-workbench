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

#include "grt_dispatcher.h"
#include "grt_manager.h"
#include <grtpp_util.h>
#include <grtpp_shell.h>
#include "base/log.h"
#include "base/threading.h"
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

class NULLTask : public GRTTaskBase
{
public:
  NULLTask(GRTDispatcher *disp) : GRTTaskBase("Terminate Worker Thread", disp) {}
  virtual void finished(const grt::ValueRef &result) {} // dummy override to disable retain()
  virtual grt::ValueRef execute(grt::GRT *) { return grt::ValueRef(); }
};


//-----------------------------------------------------------------------------

class GRTSimpleTask : public GRTTaskBase {
public:
  GRTSimpleTask(const std::string &name, GRTDispatcher *disp, const boost::function<grt::ValueRef (grt::GRT*)> &function)
    : GRTTaskBase(name, disp), _function(function)
  {
  }

  grt::ValueRef execute(grt::GRT *grt)
  {
    try
    {
      return _function(grt);
    }
    catch (const std::exception &e)
    {
      failed(e);
    }
    return grt::ValueRef();
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

protected:
  GRTDispatcher *_dispatcher;

private:
  boost::function<grt::ValueRef (grt::GRT*)> _function;
};


//---------------------------------------------------------------------------

GRTTaskBase::GRTTaskBase(const std::string &name, GRTDispatcher *disp)
  : _dispatcher(disp), _exception(0), _name(name), _refcount(1), _cancelled(false), _finished(false),
_messages_to_main_thread(true)
{
}


GRTTaskBase::~GRTTaskBase()
{
  delete _exception;
}


void GRTTaskBase::set_finished()
{
  _finished= true;
}


void GRTTaskBase::cancel()
{
  _cancelled= true;
}


void GRTTaskBase::retain()
{
  base::atomic_int_inc(&_refcount);
}


void GRTTaskBase::release()
{
  bool do_delete = false;

  if (base::atomic_int_dec_and_test_if_zero(&_refcount))
    do_delete = true; // delete can cause a side-effect with recursion, which would deadlock

  if (do_delete)
    delete this;
}


void GRTTaskBase::started()
{
  signal_starting_task();
  _dispatcher->call_from_main_thread<void>(boost::bind(&GRTTaskBase::started_m, this), false, false);
}


void GRTTaskBase::started_m()
{
}


void GRTTaskBase::finished(const grt::ValueRef &result)
{
  retain();
  signal_finishing_task(); 
  _dispatcher->call_from_main_thread<void>(boost::bind(&GRTTaskBase::finished_m, this, result),false, false);
}


void GRTTaskBase::finished_m(const grt::ValueRef &result)
{
  set_finished();
  release();
}


void GRTTaskBase::failed(const std::exception &exc)
{
  const grt::grt_runtime_error *rterr= dynamic_cast<const grt::grt_runtime_error*>(&exc);

  if (rterr)
    _exception= new grt::grt_runtime_error(*rterr);
  else
    _exception= new grt::grt_runtime_error(exc.what(), "");

  retain();

  signal_failing_task();
  _dispatcher->call_from_main_thread<void>(boost::bind(&GRTTaskBase::failed_m, this, exc), false, false);
}


void GRTTaskBase::failed_m(const std::exception &exc)
{
  set_finished();
  release();
}

bool GRTTaskBase::process_message(const grt::Message &msg)
{
  retain();
  if (_messages_to_main_thread)
    _dispatcher->call_from_main_thread<void>(boost::bind(&GRTTaskBase::process_message_m, this, msg), false/*was true*/, false);
  else
    process_message_m(msg);
  
  return true;
}


void GRTTaskBase::process_message_m(const grt::Message &msg)
{
  // if there's no message handler, the default one will be called
//  // fallback to the original handler
//  if (_default_callbacks.message_cb)
//    _default_callbacks.message_cb(msg, this);

  release();
}


//---------------------------------------------------------------------------


GRTTask::GRTTask(const std::string &name, GRTDispatcher *owner, const boost::function<grt::ValueRef (grt::GRT*)> &function)
  : GRTTaskBase(name, owner), _function(function)
{
}


grt::ValueRef GRTTask::execute(grt::GRT *grt)
{
  return _function(grt);
}


void GRTTask::started_m()
{
  _started();
}


void GRTTask::finished_m(const grt::ValueRef &result)
{
  _finished(result);

  GRTTaskBase::finished_m(result);
}


void GRTTask::failed_m(const std::exception &error)
{
  //_failed.emit(error);
  _failed(*_exception);
  
  //GRTTaskBase::failed_m(error);
  GRTTaskBase::failed_m(*_exception);
}


bool GRTTask::process_message(const grt::Message &msg)
{
  if (_message.empty())
    return false;
  
  return GRTTaskBase::process_message(msg);
}


void GRTTask::process_message_m(const grt::Message &msgs)
{
  _message(msgs);

  release();
}

//-----------------------------------------------------------------------------

GRTShellTask::GRTShellTask(const std::string &name, GRTDispatcher *owner, const std::string &command)
  : GRTTaskBase(name, owner)
{
  _command= command;
}


grt::ValueRef GRTShellTask::execute(grt::GRT *grt)
{
  _result= grt->get_shell()->execute(_command);

  _prompt= grt->get_shell()->get_prompt();

  return grt::ValueRef();
}


void GRTShellTask::finished_m(const grt::ValueRef &result)
{
  _finished_signal(_result, _prompt);
  
  GRTTaskBase::finished_m(result);
}


bool GRTShellTask::process_message(const grt::Message &msg)
{
  if (_message.empty())
    return false;
  
  return GRTTaskBase::process_message(msg);
}


void GRTShellTask::process_message_m(const grt::Message &msg)
{
  _message(msg);

  release();
}

//-----------------------------------------------------------------------------

static void sleep_2ms()
{
  g_usleep(2000);
}

static GThread *_main_thread= 0;

GRTDispatcher::GRTDispatcher(grt::GRT *grt, bool threaded, bool is_main_dispatcher)
  : _busy(0), _threading_disabled(!threaded), _w_runing(0), _is_main_dispatcher(is_main_dispatcher), _shut_down(false), _grt(grt), _current_task(NULL)
{
  _shutdown_callback= false;

  if (threaded)
  {
    _task_queue= g_async_queue_new();

    _callback_queue= g_async_queue_new();
  }
  else
  {
    _task_queue= NULL;
    _callback_queue= NULL;
  }
  _thread= 0;
  if (_is_main_dispatcher) // assuming main dispatcher is created from main thread
    _main_thread= g_thread_self();
  
  // default implementation just sleeps for 2ms
  _flush_main_thread_and_wait= sleep_2ms;

  if (getenv("WB_DEBUG_DISPATCHER"))
    debug_dispatcher= true;
}


GRTDispatcher::~GRTDispatcher()
{
  shutdown();

  if (_task_queue)
    g_async_queue_unref(_task_queue);
  if (_callback_queue)
    g_async_queue_unref(_callback_queue);
}


void GRTDispatcher::start(boost::shared_ptr<GRTDispatcher> self)
{
  _shut_down = false;
  if (!_threading_disabled)
  {

    log_debug("starting worker thread\n");
    _thread= base::create_thread(worker_thread, this);
    if (_thread == 0)
    {
      log_error("base::create_thread failed to create the GRT worker thread. Falling back into non-threaded mode.\n");
      _threading_disabled = true;
    }
  }

  bec::GRTManager *grtm= bec::GRTManager::get_instance_for(_grt);
  if (grtm) // in tests, grtm may not exist
    grtm->add_dispatcher(self);

  if (_is_main_dispatcher)
  {
    _grt->push_message_handler(boost::bind(&GRTDispatcher::message_callback, this, _1, _2));
//    _grt->push_status_query_handler(boost::bind(&GRTDispatcher::status_query_callback, this));
  }
}


void GRTDispatcher::shutdown()
{
  if (_shut_down)
      return;
  _shut_down = true;
  if (_is_main_dispatcher)
  {
    _grt->pop_message_handler();
//    _grt->pop_status_query_handler();
  }

  _shutdown_callback= true;
  if (!_threading_disabled && _thread != 0) // _thread == 0, means that init was not called, but threading_disabled was set to false.
  {
    NULLTask *task= new NULLTask(this);
    add_task(task);
    log_debug2("GRTDispatcher:Main thread waiting for worker to finish\n");
    _w_runing.wait();
    log_debug2("GRTDispatcher:Main thread worker finished\n");
  }

  bec::GRTManager *grtm= bec::GRTManager::get_instance_for(_grt);
  if (grtm)
    grtm->remove_dispatcher(this);
}


gpointer GRTDispatcher::worker_thread(gpointer data)
{
  GRTDispatcher *self= static_cast<GRTDispatcher*>(data);
  GAsyncQueue *task_queue= self->_task_queue;
  GAsyncQueue *callback_queue= self->_callback_queue;

  mforms::Utilities::set_thread_name("GRTDispatcher");

  log_debug("worker thread running\n");
  
  g_async_queue_ref(task_queue);
  g_async_queue_ref(callback_queue);

  self->worker_thread_init();
  
//  self->_worker_running= true;
  while (true)
  {
    GRTTaskBase *task;

    self->worker_thread_iteration();

    // pop next task pushed to queue by the main thread

#if GLIB_CHECK_VERSION(2,32,0)
    task= static_cast<GRTTaskBase*>(g_async_queue_timeout_pop(task_queue, 1000000));
#else
    GTimeVal timeout;
    g_get_current_time(&timeout);
    timeout.tv_sec+= 1;
    task= static_cast<GRTTaskBase*>(g_async_queue_timed_pop(task_queue, &timeout));
#endif
    if (!task)
      continue;

    base::atomic_int_inc(&self->_busy);
    log_debug3("GRT dispatcher, running task %s", task->name().c_str());

    if (dynamic_cast<NULLTask*>(task) != 0) // a NULL task terminates the thread
    {
      DPRINT("worker: termination task received, closing...");
      task->finished(grt::ValueRef());
      task->release();
      base::atomic_int_dec_and_test_if_zero(&self->_busy);
      break;
    }

    if (task->is_cancelled())
    {
      DPRINT("%s", std::string("worker: task '"+task->name()+"' was cancelled.").c_str());
      task->release();
      base::atomic_int_dec_and_test_if_zero(&self->_busy);
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
      task->release();
      base::atomic_int_dec_and_test_if_zero(&self->_busy);
      continue;
    }

    if (count != self->grt()->message_handler_count())
      log_error("INTERNAL ERROR: Message handler count mismatch after executing task '%s' (%i vs %i)", 
                task->name().c_str(), count, self->grt()->message_handler_count());

    // cleanup
    task->release();

    base::atomic_int_dec_and_test_if_zero(&self->_busy);
    DPRINT("worker: task finished.");
  }

  self->worker_thread_release();

  g_async_queue_unref(task_queue);
  g_async_queue_unref(callback_queue);

//  self->_worker_running= false;
  self->_w_runing.post();

  log_debug("worker thread exiting...\n");

  return NULL;
}


void GRTDispatcher::execute_now(GRTTaskBase *task)
{
  base::atomic_int_inc(&_busy);
  prepare_task(task);
  
  execute_task(task);
  
  task->release();
  base::atomic_int_dec_and_test_if_zero(&_busy);
}


void GRTDispatcher::add_task(GRTTaskBase *task)
{
  // if threading is disabled or the worker thread is calling another 
  // task, then we have to execute it immediately otherwise we'd just deadlock
  if (_threading_disabled || _thread == g_thread_self())
  {
    execute_now(task);
  }
  else
  {
    g_async_queue_push(_task_queue, task);
  }
}


void GRTDispatcher::cancel_task(GRTTaskBase *task)
{
  task->cancel();
}


bool GRTDispatcher::get_busy()
{
  return (_task_queue && g_async_queue_length(_task_queue) > 0) || base::atomic_int_get(&_busy);
}


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


void GRTDispatcher::flush_pending_callbacks()
{
  DispatcherCallbackBase *callback;
  
  if (_callback_queue)
  {
    while ((callback= static_cast<DispatcherCallbackBase*>(g_async_queue_try_pop(_callback_queue))))
    {
      // Don't run any task, but clear the queue if we are shutting down.
      // This way the NullTask can surface up in the worker thread.
      if (!_shutdown_callback)
        callback->execute();
      callback->signal();
      callback->release();  // this release <--------------------------------------------------+
    }                                                                                     //   |
  }                                                                                       //   |
}                                                                                         //   |
                                                                                          //   |
                                                                                          //   |
void GRTDispatcher::call_from_main_thread(DispatcherCallbackBase *callback,               //   |
  bool wait,                                                                              //   |
  bool force_queue)                                                                       //   |
{                                                                                         //   |
  // retain once for ourselves                                                            //   |
  callback->retain();                                                                     //   |
                                                                                          //   |
  // and another time for the main-thread (will be released from main thread)             //   |
  callback->retain();    // this retain matched by... -----------------------------------------+

  bool is_main_thread= (g_thread_self() == _main_thread);
  if (force_queue && is_main_thread)
    wait= false;

  if (!force_queue && (_threading_disabled || is_main_thread))
  {
    callback->execute();
    callback->signal();
    callback->release();
    wait= false;
  }
  else
    g_async_queue_push(_callback_queue, callback);

  if (wait)
    callback->wait();

  callback->release();
}




void GRTDispatcher::worker_thread_init()
{
//QQQ  _grt->enable_thread_notifications();
}

void GRTDispatcher::worker_thread_release()
{
  mforms::Utilities::driver_shutdown();
}

void GRTDispatcher::worker_thread_iteration()
{
//QQQ  _grt->flush_notifications();
}


bool GRTDispatcher::message_callback(const grt::Message &msgs, void *sender)
{
  if (GRTTaskBase *task= (sender) ? static_cast<GRTTaskBase*>(sender) : _current_task)
    return task->process_message(msgs);
  return false; // let it bubble up by default
}

/*
bool GRTDispatcher::status_query_callback()
{
  if (_current_task)
    return _current_task->perform_status_query() != 0;
  else
    return _grt->query_status();
}
*/

static bool call_process_message(const grt::Message &msgs, void *sender, GRTTaskBase *task)
{
  if (sender)
    task= static_cast<GRTTaskBase*>(sender);
  return task->process_message(msgs);
}
/*
static bool call_status_query(GRTTaskBase *task)
{
  return task->perform_status_query() != 0;
}*/


void GRTDispatcher::prepare_task(GRTTaskBase *gtask)
{
  gtask->retain();
  _current_task= gtask;
//  _grt->flush_messages();
  
  // directly set the task callbacks
  if (_is_main_dispatcher)
  {
    _grt->push_message_handler(boost::bind(call_process_message, _1, _2, gtask));
//    _grt->push_status_query_handler(boost::bind(call_status_query, gtask));
  }
}


void GRTDispatcher::restore_callbacks(GRTTaskBase *task)
{
  // restore originally set msg callbacks
  if (_is_main_dispatcher)
  {
    _grt->pop_message_handler();
//    _grt->pop_status_query_handler();
  }
 
//  _grt->flush_messages();
  _current_task= NULL;
  task->release();
}


void GRTDispatcher::execute_task(GRTTaskBase *gtask)
{
  try
  {
    gtask->started();
    grt::ValueRef result= gtask->execute(_grt);
    gtask->__result= result; // for passing result to add_task_and_wait

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


void GRTDispatcher::wait_task(GRTTaskBase *task)
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


grt::ValueRef GRTDispatcher::add_task_and_wait(GRTTaskBase *task) THROW (grt::grt_runtime_error)
{
  grt::ValueRef result;
  
#if 0
  if (is_busy())
  {
    g_error("The GRT dispatcher is currently executing another task.\n"
            "Calling add_task_and_wait() while another task is running can lead\n"
            "to deadlocks and is not allowed.");
    return grt::ValueRef();
  }
#endif

  task->retain();
  
  add_task(task);
  
  // wait for the task to finish executing
  wait_task(task);
  
  // if there was an exception during execution in the worker thread,
  // we throw an exception signaling its failure
  if (task->get_error())
  {
    grt::grt_runtime_error error= *task->get_error();
    task->release();

    throw error;
  }

  result= task->__result;

  task->release();

  return result;
}


grt::ValueRef GRTDispatcher::execute_simple_function(const std::string &name, 
                                                         const boost::function<grt::ValueRef (grt::GRT*)> &function) THROW (grt::grt_runtime_error)
{
  GRTSimpleTask *task= new GRTSimpleTask(name, this, function);

  task->retain();

  add_task_and_wait(task);
  
  grt::ValueRef tmp= task->__result;
  
  task->release();
  
  return tmp;
}


void GRTDispatcher::execute_async_function(const std::string &name,
                                           const boost::function<grt::ValueRef (grt::GRT*)> &function) THROW (grt::grt_runtime_error)
{
  GRTSimpleTask *task= new GRTSimpleTask(name, this, function);

  task->retain();

  add_task(task);
}

