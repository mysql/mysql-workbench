/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/threading.h"

#include "grt.h"
#include "grtpp_util.h"
#include "grtpp_shell.h"

#include "common.h"

#include "wbpublic_public_interface.h"

namespace bec {

  class GRTManager;
  class WBPUBLICBACKEND_PUBLIC_FUNC GRTDispatcher;

  // Mechanism for allowing queuing of callbacks to be executed
  // in the main thread by the GRT worked thread.
  // The target object, method and arguments are all encapsulated
  // in the callback object.

  class WBPUBLICBACKEND_PUBLIC_FUNC DispatcherCallbackBase {
  private:
    base::Semaphore _semaphore;

  protected:
    DispatcherCallbackBase();

  public:
    typedef std::shared_ptr<DispatcherCallbackBase> Ref;

    virtual ~DispatcherCallbackBase();
    virtual void execute() = 0;
    void wait();
    void signal();
  };

  //------------------------------------------------------------------------------------------------

  template <class R>
  class DispatcherCallback : public DispatcherCallbackBase {
  public:
    typedef std::function<R()> slot_type;
    typedef std::shared_ptr<DispatcherCallback<R> > Ref;

    static Ref create_callback(const slot_type &slot) {
      return Ref(new DispatcherCallback<R>(slot));
    }

    void execute() {
      if (_slot)
        _return_value = _slot();
    }

    R get_result() {
      return _return_value;
    }

  private:
    slot_type _slot;
    R _return_value;

    DispatcherCallback(const slot_type &slot) : DispatcherCallbackBase(), _slot(slot){};
  };

  template <>
  class DispatcherCallback<void> : public DispatcherCallbackBase {
  public:
    typedef std::function<void()> slot_type;
    typedef std::shared_ptr<DispatcherCallback<void> > Ref;

    static Ref create_callback(const slot_type &slot = slot_type()) {
      return Ref(new DispatcherCallback<void>(slot));
    }

    void execute() {
      if (_slot)
        _slot();
    }

  private:
    slot_type _slot;

    DispatcherCallback(const slot_type &slot) : DispatcherCallbackBase(), _slot(slot){};
  };

  //------------------------------------------------------------------------------------------------

  class WBPUBLICBACKEND_PUBLIC_FUNC GRTTaskBase {
  public:
    typedef std::shared_ptr<GRTTaskBase> Ref;

    virtual ~GRTTaskBase();

    inline bool is_finished() {
      return _finished;
    }

    virtual grt::ValueRef execute() = 0;

    void cancel();
    inline bool is_cancelled() {
      return _cancelled;
    }

    std::string name() {
      return _name;
    }
    grt::ValueRef result() {
      return _result;
    };

    void set_handle_messages_from_thread() {
      _messages_to_main_thread = false;
    }

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

    grt::grt_runtime_error *get_error() {
      return _exception;
    };

    // Signals.
    typedef boost::signals2::signal<void()> StartingTaskSignal;
    StartingTaskSignal signal_starting_task;

    typedef boost::signals2::signal<void()> FinishingTaskSignal;
    FinishingTaskSignal signal_finishing_task;

    typedef boost::signals2::signal<void()> FailingTaskSignal;
    FailingTaskSignal signal_failing_task;

  protected:
    std::shared_ptr<GRTDispatcher> _dispatcher;
    grt::grt_runtime_error *_exception;
    grt::ValueRef _result;

    GRTTaskBase(const std::string &name, const std::shared_ptr<GRTDispatcher> dispatcher)
      : _dispatcher(dispatcher),
        _exception(0),
        _name(name),
        _cancelled(false),
        _finished(false),
        _messages_to_main_thread(true) {
    }

    void set_finished();

  private:
    std::string _name;
    bool _cancelled;
    bool _finished;
    bool _messages_to_main_thread;

    // Should never be defined and called.
    GRTTaskBase(GRTTaskBase &);
    GRTTaskBase &operator=(GRTTaskBase &);
  };

  //------------------------------------------------------------------------------------------------

  class WBPUBLICBACKEND_PUBLIC_FUNC GRTTask : public GRTTaskBase {
    typedef boost::signals2::signal<void()> StartedSignal;
    typedef boost::signals2::signal<void(grt::ValueRef)> FinishedSignal;
    typedef boost::signals2::signal<void(const std::exception &)> FailedSignal;
    typedef boost::signals2::signal<void(const grt::Message &)> ProcessMessageSignal;

  public:
    typedef std::shared_ptr<GRTTask> Ref;

    static Ref create_task(const std::string &name, const std::shared_ptr<GRTDispatcher> dispatcher,
                           const std::function<grt::ValueRef()> &function);

    // XXX replace with direct slots?
    StartedSignal *signal_started() {
      return &_sigStarted;
    }
    FinishedSignal *signal_finished() {
      return &_sigFinished;
    }
    FailedSignal *signal_failed() {
      return &_sigFailed;
    }
    ProcessMessageSignal *signal_message() {
      return &_message;
    }

  protected:
    std::function<grt::ValueRef()> _function;

    StartedSignal _sigStarted;
    FinishedSignal _sigFinished;
    FailedSignal _sigFailed;
    ProcessMessageSignal _message;

    virtual grt::ValueRef execute();

    GRTTask(const std::string &name, const std::shared_ptr<GRTDispatcher> dispatcher,
            const std::function<grt::ValueRef()> &function);
    virtual void started_m();
    virtual void finished_m(const grt::ValueRef &result);
    virtual void failed_m(const std::exception &error);

    virtual bool process_message(const grt::Message &msg);
    virtual void process_message_m(const grt::Message &msg);
  };

  //------------------------------------------------------------------------------------------------

  class GRTShellTask : public GRTTaskBase {
    typedef boost::signals2::signal<void(grt::ShellCommand, std::string)> FinishedSignal;
    typedef boost::signals2::signal<void(const grt::Message &)> ProcessMessageSignal;

  public:
    typedef std::shared_ptr<GRTShellTask> Ref;

    static Ref create_task(const std::string &name, const std::shared_ptr<GRTDispatcher> dispatcher,
                           const std::string &command);

    FinishedSignal &signal_finished() {
      return _finished_signal;
    }
    ProcessMessageSignal &signal_message() {
      return _message;
    }

    inline std::string get_prompt() const {
      return _prompt;
    }
    inline grt::ShellCommand get_result() const {
      return _result;
    }

  protected:
    GRTShellTask(const std::string &name, const std::shared_ptr<GRTDispatcher> dispatcher, const std::string &command);

    virtual grt::ValueRef execute();
    virtual void finished_m(const grt::ValueRef &result);

    virtual bool process_message(const grt::Message &msg);
    virtual void process_message_m(const grt::Message &msg);

    FinishedSignal _finished_signal;
    ProcessMessageSignal _message;

    std::string _command;

    std::string _prompt;
    grt::ShellCommand _result;
  };

  //------------------------------------------------------------------------------------------------

  class WBPUBLICBACKEND_PUBLIC_FUNC GRTDispatcher : public std::enable_shared_from_this<GRTDispatcher> {
  public:
    typedef void (*FlushAndWaitCallback)();
    typedef std::shared_ptr<GRTDispatcher> Ref;

  private:
    GAsyncQueue *_task_queue;
    FlushAndWaitCallback _flush_main_thread_and_wait;
    std::weak_ptr<bec::GRTManager> _grtm;

    volatile base::refcount_t _busy;

    bool _threading_disabled;
    base::Semaphore _w_runing;
    volatile bool _shutdown_callback;
    bool _is_main_dispatcher;
    bool _shut_down;
    bool _started;

    GAsyncQueue *_callback_queue;
    GThread *_thread;

    static gpointer worker_thread(gpointer data);

    GRTTaskBase::Ref _current_task;

    GRTDispatcher(bool threaded, bool is_main_dispatcher);

    void prepare_task(const GRTTaskBase::Ref task);
    void execute_task(const GRTTaskBase::Ref task);

    void worker_thread_init();
    void worker_thread_release();
    void worker_thread_iteration();

    void restore_callbacks(const GRTTaskBase::Ref task);

    bool message_callback(const grt::Message &msg, void *sender);

  public:
    static Ref create_dispatcher(bool threaded, bool is_main_dispatcher);

    virtual ~GRTDispatcher();

    void execute_now(const GRTTaskBase::Ref task);

    void add_task(const GRTTaskBase::Ref task);
    grt::ValueRef add_task_and_wait(const GRTTaskBase::Ref task);

    grt::ValueRef execute_sync_function(const std::string &name, const std::function<grt::ValueRef()> &function);

    void execute_async_function(const std::string &name, const std::function<grt::ValueRef()> &function);

    void wait_task(const GRTTaskBase::Ref task);

    template <class R>
    R call_from_main_thread(const std::function<R()> &callback, bool wait, bool force_queue) {
      typename DispatcherCallback<R>::Ref cb = DispatcherCallback<R>::create_callback(callback);
      call_from_main_thread(cb, wait, force_queue);
      return cb->get_result();
    }

    void call_from_main_thread(const DispatcherCallbackBase::Ref callback, bool wait, bool force_queue);

    void set_main_thread_flush_and_wait(FlushAndWaitCallback callback);
    FlushAndWaitCallback get_main_thread_flush_and_wait() {
      return _flush_main_thread_and_wait;
    }

    void start();
    void shutdown();

    bool get_busy();

    void cancel_task(const GRTTaskBase::Ref task);

    void flush_pending_callbacks();

    GThread *get_thread() const {
      return _thread;
    }
  };

  template <>
  inline void GRTDispatcher::call_from_main_thread<void>(const std::function<void()> &callback, bool wait,
                                                         bool force_queue) {
    DispatcherCallback<void>::Ref cb = DispatcherCallback<void>::create_callback(callback);
    call_from_main_thread(cb, wait, force_queue);
  }
};
