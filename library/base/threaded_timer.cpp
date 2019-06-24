/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <stdio.h>
#include <stdexcept>

#include "base/threaded_timer.h"
#include "base/log.h"
#include "base/threading.h"

// 30 fps should ensure smooth animations. Higher values are better, but put higher load on a system.
#define BASE_FREQUENCY 30

// Define the maximum number of worker threads. If they are used up tasks have to wait.
#define WORKER_THREAD_COUNT 2

DEFAULT_LOG_DOMAIN(DOMAIN_BASE)

//--------------------------------------------------------------------------------------------------

static ThreadedTimer *_timer = NULL;
G_LOCK_DEFINE(_timer);

/**
 * Returns the singleton instance of the timer.
 */
ThreadedTimer *ThreadedTimer::get() {
  G_LOCK(_timer);
  if (_timer == NULL) {
    _timer = new ThreadedTimer(BASE_FREQUENCY);
  }
  G_UNLOCK(_timer);
  return _timer;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called from the main framework when the application goes down. So we can stop all threads
 * gracefully.
 */
void ThreadedTimer::stop() {
  delete _timer;
  _timer = NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to add a new task (either a re-occuring or a one-shot task) to the timer's task list.
 *
 * @param unit Specifies in which unit the given value is. One can specify time spans and other units.
 * @param value A value which must be interpreted in the given unit. It can be:
 *              - A frequency (given in Hz).
 *              - A time span (given in seconds).
 * @param single_shot True, if this event must be triggered only once.
 * @param callback_ What to call when a timer event fires.
 * @result The id of the new task (can be used in the callback) or -1 if the task could not be added.
 */
int ThreadedTimer::add_task(TimerUnit unit, double value, bool single_shot, TimerFunction callback) {
  TimerTask task = {0, 0.0, 0.0, callback, false, single_shot, false};

  if (value <= 0)
    throw std::logic_error("The given timer value is invalid.");

  switch (unit) {
    case TimerFrequency:
      // The given value is a frequency. It must not be higher than our base frequency.
      // Note: giving a one-shot timer with a frequency doesn't make much sense, but we
      //       support this nonetheless.
      if (value > BASE_FREQUENCY)
        throw std::logic_error("The given task frequency is higher than the base frequency.");
      task.wait_time = 1 / value;
      break;
    case TimerTimeSpan:
      // The given value is a time span given in seconds.
      // It must not be lower than the minimal time span we support.
      if (value < 1.0 / BASE_FREQUENCY)
        throw std::logic_error("The given task time span is smaller than the smallest supported value.");
      task.wait_time = value;
      break;
  }
  if (task.wait_time > 0) {
    ThreadedTimer *timer = ThreadedTimer::get();
    base::MutexLock lock(timer->_timer_lock);

    // in theory, it is possible to wrap around to 0 again.  Not a very likely scenario, but better safe than sorry
    if (timer->_next_id == 0) // 0 is special, skip it over
      timer->_next_id++;

    // We have the lock acquired so it is save to increment the id counter.
    task.task_id = timer->_next_id++;
    timer->_tasks.push_back(task);

    return task.task_id;
  }
  return -1;
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes the given task from the task list by setting its stop flag. If the task is running
 * currently it can finish as usual. It is then removed on the next run of the scheduler.
 *
 * @param task_id The id of the task to remove. If it does not exist nothing happens.
 */
bool ThreadedTimer::remove_task(int task_id) {
  ThreadedTimer *timer = ThreadedTimer::get();
  return timer->remove(task_id);
}

//--------------------------------------------------------------------------------------------------

ThreadedTimer::ThreadedTimer(int base_frequency) : _terminate(false), _next_id(1) {
  // Wait time in microseconds.
  _wait_time = 1000 * 1000 / base_frequency;
  _thread = base::create_thread(start, this);
  _pool = g_thread_pool_new((GFunc)pool_function, this, WORKER_THREAD_COUNT, FALSE, NULL);
}

//--------------------------------------------------------------------------------------------------

/**
 * Shuts down the timer and does not return until currently running threads have terminated.
 */
ThreadedTimer::~ThreadedTimer() {
  // Free the thread pool but wait until tasks, which are currently executing have finished.
  // Pending tasks are discarded.
  logDebug2("Threaded timer shutdown...\n");

  // Don't lock the mutex or we might deadlock here if the mutex is currently held by the work loop.
  _terminate = true;

  // Wait for the timer thread to terminate.
  g_thread_join(_thread);

  g_thread_pool_free(_pool, TRUE, TRUE);

  logDebug2("Threaded timer shutdown done\n");
}

//--------------------------------------------------------------------------------------------------

/**
 * Main entry point for the timer thread.
 */
gpointer ThreadedTimer::start(gpointer data) {
  ThreadedTimer *thread = static_cast<ThreadedTimer *>(data);
  thread->main_loop();
  return NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Entry point for all pool (worker) threads.
 */
void ThreadedTimer::pool_function(gpointer data, gpointer user_data) {
  ThreadedTimer *timer = static_cast<ThreadedTimer *>(user_data);
  TimerTask *task = static_cast<TimerTask *>(data);

  try {
    bool do_stop = task->callback(task->task_id);
    base::MutexLock lock(timer->_timer_lock);
    task->stop = do_stop || task->single_shot;
    task->scheduled = false;
  } catch (std::exception &e) {
    // In the case of an exception we remove the task silently.
    base::MutexLock lock(timer->_timer_lock);
    task->stop = true;
    task->scheduled = false;
    logWarning("Threaded timer: exception in pool function: %s\n", e.what());
  } catch (...) {
    // Most exceptions should be caught by the part above. Just to be on the safe side
    // do this extra branch.
    base::MutexLock(timer->_timer_lock);
    task->stop = true;
    task->scheduled = false;
    logWarning("Threaded timer: unknown exception in pool function\n");
  }

}

//--------------------------------------------------------------------------------------------------

// Helper predicate for removing finished tasks.
class IsStopped : public std::function<TimerTask (bool)> {
public:
  bool operator()(TimerTask &task) {
    return task.stop;
  }
};

//--------------------------------------------------------------------------------------------------

void ThreadedTimer::main_loop() {
  // Provides a high-quality clock which is used to compute execution times of tasks.
  GTimer *clock = g_timer_new();
  g_timer_start(clock);
  while (!_terminate) {
    // This sleep call forms our base frequency.
    g_usleep(_wait_time);

    if (_terminate)
      break;

    // 1. Compute next execution time for new tasks.
    base::MutexLock lock(_timer_lock);
    for (std::list<TimerTask>::iterator iterator = _tasks.begin(); iterator != _tasks.end(); iterator++) {
      if (iterator->next_time == 0)
        iterator->next_time = g_timer_elapsed(clock, NULL) + iterator->wait_time;
    }

    // 2. Execute all tasks which are due now.
    // Processing of the task entries should be very fast here. No need to make a copy of them.
    gdouble current_time = g_timer_elapsed(clock, NULL);
    for (std::list<TimerTask>::iterator iterator = _tasks.begin(); iterator != _tasks.end(); ++iterator) {
      if (_terminate)
        break;

      if (!iterator->scheduled && iterator->next_time <= current_time && !iterator->stop) {
        // When the task is due push it to our thread pool. It will then get one of the
        // free threads assigned to run in and pool_function is called in this thread's context.
        // Do it only if it isn't already scheduled.
        TimerTask &task = *iterator;
        task.scheduled = true;
        task.next_time += task.wait_time;
        g_thread_pool_push(_pool, &task, NULL);
      }
    }

    // 3. Remove stopped task.
    _tasks.remove_if(IsStopped());
  }
  g_timer_destroy(clock);
}

//--------------------------------------------------------------------------------------------------

/**
 * This function is actually doing the work for the static remove_task function.
 * @returns true, if the task could be removed, otherwise false.
 * If the task is already scheduled for execution it cannot be removed anymore.
 */
bool ThreadedTimer::remove(int task_id) {
  base::MutexLock lock(_timer_lock);
  for (std::list<TimerTask>::iterator iterator = _tasks.begin(); iterator != _tasks.end(); iterator++) {
    if (iterator->task_id == task_id) {
      iterator->stop = true;
      TimerTask &task = *iterator;
      return !static_cast<bool>(g_thread_pool_move_to_front(_pool, &task));
    }
  }
  return true;
}

//--------------------------------------------------------------------------------------------------
