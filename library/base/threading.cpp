/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

using namespace base;

#if GLIB_CHECK_VERSION(2,32,0)
void base::threading_init()
{
  // noop
}
#else
static bool g_thread_is_ready = false;
void base::threading_init()
{
  if (!g_thread_is_ready && !g_thread_supported())
    g_thread_init(NULL);
  g_thread_is_ready = true;
}
#endif

//--------------------------------------------------------------------------------------------------

Mutex::Mutex()
{
#if GLIB_CHECK_VERSION(2,32,0)
  g_mutex_init(&mutex);
#else
  threading_init();

  mutex = g_mutex_new();
#endif
}

//--------------------------------------------------------------------------------------------------

Mutex::~Mutex()
{
#if GLIB_CHECK_VERSION(2,32,0)
  g_mutex_clear(&mutex);
#else
  g_mutex_free(mutex);
#endif
}

//--------------------------------------------------------------------------------------------------

void Mutex::swap(Mutex &o)
{
#if GLIB_CHECK_VERSION(2,32,0)
  GMutex tmp = o.mutex;
  o.mutex = mutex;
  mutex = tmp;
#else
  GMutex *tmp = o.mutex;
  o.mutex = mutex;
  mutex = tmp;
#endif
}

//--------------------------------------------------------------------------------------------------

MutexLock::MutexLock(Mutex &mutex) : ptr(&mutex)
{
  if (!ptr)
    throw std::logic_error("NULL ptr given");
  ptr->lock();
}

//--------------------------------------------------------------------------------------------------

// take ownership of an existing lock (the other lock will be reset)
MutexLock::MutexLock(const MutexLock &mlock)
: ptr(mlock.ptr)
{
  const_cast<MutexLock*>(&mlock)->ptr = NULL;
}

//--------------------------------------------------------------------------------------------------

MutexLock &MutexLock::operator= (MutexLock &mlock)
{
  ptr = mlock.ptr;
  mlock.ptr = NULL;
  return *this;
}

//--------------------------------------------------------------------------------------------------

MutexLock::~MutexLock()
{
  if (ptr)
    ptr->unlock();
}

//----------------- Cond ---------------------------------------------------------------------------

Cond::Cond()
{
#if GLIB_CHECK_VERSION(2,32,0)
  g_cond_init(&cond);
#else
  threading_init();

  cond = g_cond_new();
#endif
}

//--------------------------------------------------------------------------------------------------

Cond::~Cond()
{
#if GLIB_CHECK_VERSION(2,32,0)
  g_cond_clear(&cond);
#else
  g_cond_free(cond);
#endif
}

//----------------- Semaphore ----------------------------------------------------------------------

static int semaphore_data = 1; // Dummy data to identify non-NULL return values.

Semaphore::Semaphore(int initial_count)
{
  _queue = g_async_queue_new();

  // Push as many "messages" to the queue as is specified by the initial count.
  // This amount is what is available before we lock in wait() or try_wait().
  while (initial_count-- > 0)
    g_async_queue_push(_queue, &semaphore_data);
}

//--------------------------------------------------------------------------------------------------

Semaphore::~Semaphore()
{
  g_async_queue_unref(_queue);
}

//--------------------------------------------------------------------------------------------------

void Semaphore::post()
{
  g_async_queue_push(_queue, &semaphore_data);
}

//--------------------------------------------------------------------------------------------------

void Semaphore::wait()
{
  g_async_queue_pop(_queue); // Waits if there is no data in the queue.
}

//--------------------------------------------------------------------------------------------------

bool Semaphore::try_wait()
{
  return g_async_queue_try_pop(_queue) != NULL;
}

//--------------------------------------------------------------------------------------------------
