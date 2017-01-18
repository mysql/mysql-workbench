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

#ifndef _THREADED_TIMER_H_
#define _THREADED_TIMER_H_

#include "common.h"

#ifndef HAVE_PRECOMPILED_HEADERS
#include "glib.h"
#include <list>
#include <functional>
#endif

#include "base/threading.h"

// The callback type used for timer events. It gets the id of the task returned from add_task
// and must return a boolean value which tells us if the task should continue to run or
// immediately be stopped. For one-shot tasks the return value has no meaning.
typedef std::function<bool(int)> TimerFunction;

#ifdef _WIN32
#pragma warning(disable : 4251) // We don't want to DLL export TimerTask, and we don't need a warning for that.
#endif

struct TimerTask {
  int task_id;
  gdouble next_time;      // Precomputed target time when this task must be triggered again.
  gdouble wait_time;      // The time in seconds to wait until this task is executed again.
  TimerFunction callback; // The callback to trigger when the timer fires.
  bool stop;              // Tells the scheduler to remove this task.
  bool single_shot;       // If true then this task will only run once.
  bool scheduled;         // True if the task has been scheduled currently (it is waiting in the pool to get executed).
};

typedef std::list<TimerTask> TaskList;

// The unit type of the timer value given to ThreadedTimer::add_task.
enum TimerUnit { TimerFrequency, TimerTimeSpan };

/**
 * The threaded timer is supposed to run as a singleton and provide scheduled timer events. It takes orders when to
 * trigger
 * a timer event, depending on the given frequency (if it is a repeating timer) or delay (for one-shot timers).
 * It forms the base for timed services like animations, server pings in the background etc.
 */
class BASELIBRARY_PUBLIC_FUNC ThreadedTimer {
public:
  static ThreadedTimer* get();
  static void stop();

  static int add_task(TimerUnit unit, double value, bool single_shot, TimerFunction callback);
  static void remove_task(int task_id);

private:
  base::Mutex _timer_lock; // Synchronize access to the timer class.
  GThreadPool* _pool;      // A number of threads which trigger the callbacks (to make them independant of each other).
  int _wait_time;          // The time the timer thread has to wait until looking for new tasks to execute.
  bool _terminate;         // Set to true when shutting down the timer.
  int _next_id;            // A counter for task ids.

  GThread* _thread; // This thread loops endlessly executing tasks as they come in.
  TaskList _tasks;

  ThreadedTimer(int base_frequency);
  ~ThreadedTimer();

  static gpointer start(gpointer data);
  static gpointer pool_function(gpointer data, gpointer user_data);
  void main_loop();
  void remove(int task_id);
};

#endif // _THREADED_TIMER_H_
