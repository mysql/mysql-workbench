/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "wb_helpers.h"

// Would be good if we could determine the order of how test modules are run.
// For instance the threading tests should run before higher level tests (e.g. the auto completion cache tests).
TEST_MODULE(threading_test, "Base library threading support tests");

// The minimum time span on which we base all other timing. Can be changed to counter
// unpredictable thread switching timing.
#define BASE_TIME 1000

static base::refcount_t counter;

gpointer thread_function1(gpointer data) {
  base::Semaphore *semaphore = static_cast<base::Semaphore *>(data);
  semaphore->wait();

  // We now can do some work. The main thread is waiting meanwhile.
  // Total time 200ms.
  while (++counter < 10)
    g_usleep(20 * BASE_TIME);

  // Done with work. Awake the main thread.
  semaphore->post();

  // We need here at least some wait time. If release and alloc follow too close to each other
  // the other thread might not awake and the logic here doesn't work.
  // Must be less than the wait time before the main thread calls try_wait().
  g_usleep(BASE_TIME);

  // Immediately allocate the semaphore again and wait a moment.
  semaphore->wait();
  g_usleep(1000 * BASE_TIME);

  // Do some final work.
  while (++counter < 15)
    g_usleep(20 * BASE_TIME);
  semaphore->post();

  return NULL;
}

/**
 *	Semaphore test with cooperative semaphore (init count = 0).
 *	Intentional synchronization of two threads.
 */
TEST_FUNCTION(10) {
  base::Semaphore semaphore(0);
  counter = 0;

  GError *error = NULL;
  GThread *thread = create_thread(thread_function1, &semaphore, &error);
  if (thread == NULL) {
    const gchar *tmp = (error != NULL) ? error->message : "out of mem?";
    fail(std::string("Thread creation failed: ") + tmp);
  }

  // Thread runs. Now wait for a moment. The thread does so too (via the semaphore).
  g_usleep(100 * BASE_TIME);
  ensure("Test counter was changed", counter == 0);

  // Awake the thread and go to sleep.
  semaphore.post();
  g_usleep(50 * BASE_TIME); // Wait here. The thread starts working but needs longer than this time.
  semaphore.wait();         // The thread awakes us here.

  ensure_equals("Test counter value", counter, 10);
  semaphore.post();          // Give the semaphore back so the thread can continue.
  g_usleep(100 * BASE_TIME); // Wait a moment so that the thread actually gets CPU time.

  // Try to allocated the semaphore (which must fail as the thread has it again).
  ensure("Unexpectedly got semaphore access", !semaphore.try_wait());
  ensure_equals("Test counter value", counter, 10);

  // Wait for the thread to finish (will also release the semaphore).
  g_thread_join(thread);

  ensure_equals("Test counter value", counter, 15);

  // Try a last time to allocate the semaphore. Should now succeed.
  ensure("Couldn't get semaphore access", semaphore.try_wait());
  semaphore.post();
}

gpointer thread_function2(gpointer data) {
  base::Semaphore *semaphore = static_cast<base::Semaphore *>(data);
  semaphore->wait();
  g_atomic_int_inc(&counter);

  // Don't release the semaphore.
  return NULL;
}

/**
 *	Concurrent semaphore test.
 *	7 independent threads try to access 5 "counters".
 */
TEST_FUNCTION(20) {
  base::Semaphore semaphore(5);
  counter = 0;

  GThread *threads[7];
  for (int i = 0; i < 7; ++i) {
    GError *error = NULL;
    threads[i] = create_thread(thread_function2, &semaphore, &error);
    if (threads[i] == NULL) {
      const gchar *tmp = (error != NULL) ? error->message : "out of mem?";
      fail(std::string("Thread creation failed: ") + tmp);
    }
  }

  try {
    g_usleep(10 * BASE_TIME);

    // At this point only 5 threads can have done their job. 2 are still waiting.
    ensure_equals("Wrong counter value", counter, 5);

    // The threads did not release their allocation. We do this here to see if now the other
    // 2 threads get their share.
    semaphore.post();
    g_usleep(50 * BASE_TIME);
    ensure_equals("Wrong counter value", counter, 6);

    g_usleep(100 * BASE_TIME); // Nothing must happen to the counter during that wait time.
    ensure_equals("Wrong counter value", counter, 6);

    semaphore.post();
    g_usleep(50 * BASE_TIME);
    ensure_equals("Wrong counter value", counter, 7);

    for (int i = 0; i < 7; ++i)
      g_thread_join(threads[i]);
  } catch (...) {
    // Always wait for the threads to finish or they access invalid memory which results
    // in a serious error that would require user interaction on Win.
    for (int i = 0; i < 7; ++i)
      g_thread_join(threads[i]);

    throw;
  }
}

END_TESTS;

//----------------------------------------------------------------------------------------------------------------------
