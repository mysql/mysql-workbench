/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/threading.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

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

gpointer thread_function2(gpointer data) {
  base::Semaphore *semaphore = static_cast<base::Semaphore *>(data);
  semaphore->wait();
  g_atomic_int_inc(&counter);

  // Don't release the semaphore.
  return NULL;
}

$describe("threading") {
  $it("Semaphore test with cooperative semaphore (init count = 0)", [&]() {
    base::Semaphore semaphore(0);
    counter = 0;

    GError *error = NULL;
    GThread *thread = create_thread(thread_function1, &semaphore, &error);

    $expect(thread).Not.toBe(nullptr);

    // Thread runs. Now wait for a moment. The thread does so too (via the semaphore).
    g_usleep(100 * BASE_TIME);
    $expect(counter).toEqual(0);

    // Awake the thread and go to sleep.
    semaphore.post();
    g_usleep(50 * BASE_TIME); // Wait here. The thread starts working but needs longer than this time.
    semaphore.wait();         // The thread awakes us here.

    $expect(counter).toEqual(10);
    semaphore.post();          // Give the semaphore back so the thread can continue.
    g_usleep(100 * BASE_TIME); // Wait a moment so that the thread actually gets CPU time.

    // Wait for the thread to finish (will also release the semaphore).
    g_thread_join(thread);

    $expect(counter).toBe(15);

    semaphore.post();
  });

  $it("Concurrent semaphore test. 7 independent threads try to access 5 counters.", [&]() {
    base::Semaphore semaphore(5);
    counter = 0;

    GThread *threads[7];
    for (int i = 0; i < 7; ++i) {
      GError *error = NULL;
      threads[i] = create_thread(thread_function2, &semaphore, &error);

      $expect(threads[i]).Not.toBe(nullptr);
    }

    try {
      g_usleep(10 * BASE_TIME);

      // At this point only 5 threads can have done their job. 2 are still waiting.
      $expect(counter).toEqual(5);

      // The threads did not release their allocation. We do this here to see if now the other
      // 2 threads get their share.
      semaphore.post();
      g_usleep(50 * BASE_TIME);
      $expect(counter).toEqual(6);

      g_usleep(100 * BASE_TIME); // Nothing must happen to the counter during that wait time.
      $expect(counter).toEqual(6);

      semaphore.post();
      g_usleep(50 * BASE_TIME);
      $expect(counter).toEqual(7);

      for (int i = 0; i < 7; ++i)
        g_thread_join(threads[i]);
    } catch (...) {
      // Always wait for the threads to finish or they access invalid memory which results
      // in a serious error that would require user interaction on Win.
      for (int i = 0; i < 7; ++i)
        g_thread_join(threads[i]);

      throw;
    }
  });
}

}
