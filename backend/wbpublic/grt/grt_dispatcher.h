/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _GRTDISPATCHER_H_
#define _GRTDISPATCHER_H_

#include <grtpp.h>
#include <grtpp_util.h>
#include <grtpp_shell.h>
#include "common.h"
#include "base/threading.h"
#include "wbpublic_public_interface.h"

#include <boost/shared_ptr.hpp>

#define BOOST_DATE_TIME_NO_LIB
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#undef BOOST_DATE_TIME_NO_LIB

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC GRTDispatcher;

  // Mechanism for allowing queueing of callbacks to be executed
  // in the main thread by the GRT worked thread
  // The target object, method and arguments are all encapsulated
  // in the callback object.

  class DispatcherCallbackBase
  {
    base::Mutex _mutex;
    base::Cond _cond;
    volatile base::refcount_t _refcount;

  public:
    DispatcherCallbackBase()
      : _refcount(1)
      {}
    
    DispatcherCallbackBase *retain()
    {
      base::atomic_int_inc(&_refcount);
      return this;
    }
    
    void release()
    {
      if (base::atomic_int_dec_and_test_if_zero(&_refcount))
        delete this;
    }
    
    virtual ~DispatcherCallbackBase()
    {
      signal();
    }
    
    virtual void execute()= 0;
    
    void wait()
    {
      base::MutexLock lock(_mutex);
      _cond.wait(_mutex);
    }
    
    void signal()
    {
      _cond.signal();
    }
  };
  
  
  template<class R>
    class DispatcherCallback : public DispatcherCallbackBase 
  {
    typedef boost::function<R ()> slot_type;
    slot_type _slot;
    
  public:
    R rvalue;
    
    DispatcherCallback(const slot_type &slot)
      : DispatcherCallbackBase(), _slot(slot)
      {
      };
    
    void execute()
    {
      if(_slot)
        rvalue= _slot();
    }
    
    R get_result() { return rvalue; }
  };
  
  template<>
  class DispatcherCallback<void> : public DispatcherCallbackBase 
  {
    typedef boost::function<void ()> slot_type;
    slot_type _slot;
    
  public:
    DispatcherCallback(const slot_type &slot= slot_type())
    : DispatcherCallbackBase(), _slot(slot)
    {
    };
    
    void execute()
    {
      if (_slot)
        _slot();
    }
  };
  
  
  //---------------------------------------------------------------------------

  class WBPUBLICBACKEND_PUBLIC_FUNC GRTTaskBase 
  {
    friend class GRTDispatcher;
    
  public:
    GRTTaskBase(const std::string &name, GRTDispatcher *disp);
    virtual ~GRTTaskBase();

    inline bool is_finished() { return _finished; }

    virtual grt::ValueRef execute(grt::GRT *grt)= 0;

    void cancel();
    inline bool is_cancelled() { return _cancelled; }

    std::string name() { return _name; }
  
    void retain();
    void release();

    void set_handle_messages_from_thread() { _messages_to_main_thread = false; }

    // _m suffix methods are called in the main thread
    // the other ones are called in the grt thread and 
    // schedule the call of their _m counterparts

    virtual void started();
    virtual void started_m();
    
    virtual void finished(const grt::ValueRef &result);
    virtual void finished_m(const grt::ValueRef &result);
    
    virtual void failed(const std::exception &exc);
    virtual void failed_m(const std::exception &exc);

    virtual bool process_message(const grt::Message &msg);
    virtual void process_message_m(const grt::Message &msg);

    grt::grt_runtime_error *get_error() { return _exception; };

  public:
    typedef boost::signals2::signal<void ()> StartingTaskSignal;
    StartingTaskSignal signal_starting_task;

    typedef boost::signals2::signal<void ()> FinishingTaskSignal;
    FinishingTaskSignal signal_finishing_task;

    typedef boost::signals2::signal<void ()> FailingTaskSignal;
    FailingTaskSignal signal_failing_task;

  protected:
    GRTDispatcher *_dispatcher;

    grt::grt_runtime_error *_exception;

    void set_finished();

  private:
    std::string _name;
    volatile base::refcount_t _refcount;
    bool _cancelled;
    bool _finished;
    bool _messages_to_main_thread;

    grt::ValueRef __result;

    // should never be defined and called
    GRTTaskBase(GRTTaskBase&);
    GRTTaskBase& operator= (GRTTaskBase&);
  };
  
  
  class WBPUBLICBACKEND_PUBLIC_FUNC GRTTask : public GRTTaskBase 
  {
    typedef boost::signals2::signal<void ()> StartedSignal;
    typedef boost::signals2::signal<void (grt::ValueRef)> FinishedSignal;
    typedef boost::signals2::signal<void (const std::exception&)> FailedSignal;
    typedef boost::signals2::signal<void (const grt::Message&)> ProcessMessageSignal; 

  public:
    GRTTask(const std::string &name, GRTDispatcher *owner, const boost::function<grt::ValueRef (grt::GRT*)> &function);

    //XXX replace with direct slots?
    StartedSignal *signal_started() { return &_started; }
    FinishedSignal *signal_finished() { return &_finished; }
    FailedSignal *signal_failed() { return &_failed; }
    ProcessMessageSignal *signal_message() { return &_message; }
    
  protected:
    boost::function<grt::ValueRef (grt::GRT*)> _function;
    
    StartedSignal _started;
    FinishedSignal _finished;
    FailedSignal _failed;
    ProcessMessageSignal _message;
    
    virtual grt::ValueRef execute(grt::GRT *grt);

    virtual void started_m();
    virtual void finished_m(const grt::ValueRef &result);
    virtual void failed_m(const std::exception &error);

    virtual bool process_message(const grt::Message &msg);
    virtual void process_message_m(const grt::Message &msg);
  };

  
  class GRTShellTask : public GRTTaskBase
  {
    typedef boost::signals2::signal<void (grt::ShellCommand,std::string)> FinishedSignal;
    typedef boost::signals2::signal<void (const grt::Message&)> ProcessMessageSignal;

  public:
    GRTShellTask(const std::string &name, GRTDispatcher *owner, const std::string &command);

    FinishedSignal &signal_finished() { return _finished_signal; }
    ProcessMessageSignal &signal_message() { return _message; }

    inline std::string get_prompt() const { return _prompt; }
    inline grt::ShellCommand get_result() const { return _result; }

  protected:
    virtual grt::ValueRef execute(grt::GRT *grt);
    virtual void finished_m(const grt::ValueRef &result);

    virtual bool process_message(const grt::Message &msg);
    virtual void process_message_m(const grt::Message &msg);

    FinishedSignal _finished_signal;
    ProcessMessageSignal _message;

    std::string _command;
    
    std::string _prompt;
    grt::ShellCommand _result;
  };


  //----------------------------------------------------------------------

  class WBPUBLICBACKEND_PUBLIC_FUNC GRTDispatcher 
  {
  public:
    typedef void (*FlushAndWaitCallback)();
  private:
    friend class GRTTaskBase;
    GAsyncQueue *_task_queue;
    FlushAndWaitCallback _flush_main_thread_and_wait;
    
    volatile base::refcount_t _busy;
    
    bool _threading_disabled;
    boost::interprocess::interprocess_semaphore _w_runing;
    volatile bool _shutdown_callback;
    bool _is_main_dispatcher;
    bool _shut_down;
    
    GAsyncQueue *_callback_queue;
    
    GThread *_thread;
    
    static gpointer worker_thread(gpointer data);

    grt::GRT *_grt;
    //std::list<GRTTaskBase*> _current_task;
    GRTTaskBase *_current_task;

    void prepare_task(GRTTaskBase *task);
    void execute_task(GRTTaskBase *task);

    void worker_thread_init();
    void worker_thread_release();
    void worker_thread_iteration();

    void restore_callbacks(GRTTaskBase *task);

    bool message_callback(const grt::Message &msg, void *sender);

  public:
    GRTDispatcher(grt::GRT *grt, bool threaded, bool is_main_dispatcher);
    virtual ~GRTDispatcher();

    typedef boost::shared_ptr<GRTDispatcher> Ref;

    grt::GRT *grt() { return _grt; };

    void execute_now(GRTTaskBase *task);
    
    void add_task(GRTTaskBase *task);
    grt::ValueRef add_task_and_wait(GRTTaskBase *task) THROW (grt::grt_runtime_error);

    grt::ValueRef execute_simple_function(const std::string &name, 
      const boost::function<grt::ValueRef (grt::GRT*)> &function) THROW (grt::grt_runtime_error);

    void execute_async_function(const std::string &name, 
      const boost::function<grt::ValueRef (grt::GRT*)> &function) THROW (grt::grt_runtime_error);

    void wait_task(GRTTaskBase *task);
    
    template<class R>
      R call_from_main_thread(const boost::function<R ()> &callback, bool wait, bool force_queue)
      {
        DispatcherCallback<R> *cb= new DispatcherCallback<R>(callback);
        R result;
        
        call_from_main_thread(cb, wait, force_queue);
        
        // result is only valid if wait = true
        result= cb->get_result();
        
        cb->release();
        
        return result;
      }
    
    
    void call_from_main_thread(DispatcherCallbackBase *callback, bool wait, bool force_queue);
    
    void set_main_thread_flush_and_wait(FlushAndWaitCallback callback);
    FlushAndWaitCallback get_main_thread_flush_and_wait() { return _flush_main_thread_and_wait; }
    
    void start(boost::shared_ptr<GRTDispatcher> self);
    void shutdown();

    bool get_busy();

    void cancel_task(GRTTaskBase *task);
    
    void flush_pending_callbacks();

    GThread *get_thread() const { return _thread; }
  };
  

  template<>
    inline void GRTDispatcher::call_from_main_thread<void>(const boost::function<void ()> &callback, bool wait, bool force_queue)
    {
      DispatcherCallback<void> *cb= new DispatcherCallback<void>(callback);
      
      call_from_main_thread(cb, wait, force_queue);
      
      cb->release();
    }

};


#endif /* _GRTDISPATCHER_H_ */
