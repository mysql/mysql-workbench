/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _BASE_THREADING_H_
#define _BASE_THREADING_H_

#include "common.h"

#include <glib.h>
#include <stdexcept>

#if !(defined (__LP64__) || defined (__LLP64__)) || defined (_WIN32) && !defined (_WIN64)
  #define RUN_OS_32
#else
  #define RUN_OS_64
#endif

#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#include <semaphore.h>
#include "string_utilities.h"
#endif

#define BOOST_DATE_TIME_NO_LIB
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#undef BOOST_DATE_TIME_NO_LIB

namespace base {

#if defined(_WIN32)
# ifdef RUN_OS_32
typedef LONG refcount_t;
# else
typedef LONGLONG refcount_t;
# endif
#elif defined(__APPLE__)
#ifdef RUN_OS_32
typedef int32_t refcount_t;
#else
typedef int64_t refcount_t;
#endif
#else
typedef int refcount_t;
#endif

inline GThread *create_thread(GThreadFunc func, gpointer data, GError **error = NULL, std::string name = "")
{
#if GLIB_CHECK_VERSION(2,32,0)
  return g_thread_try_new(name.c_str(), func, data, error);
#else
  return g_thread_create(func, data, TRUE, error);
#endif
}


inline void atomic_int_inc(volatile refcount_t *val)
{
#if defined(_WIN32)
#ifdef RUN_OS_32
  InterlockedIncrement(val);
#else
  InterlockedIncrement64(val);
#endif
#elif defined(__APPLE__)
  #ifdef RUN_OS_32
  OSAtomicIncrement32Barrier(val);
  #else
  OSAtomicIncrement64Barrier(val);
  #endif
#else
  g_atomic_int_inc(val);
#endif
}

inline bool atomic_int_dec_and_test_if_zero(volatile refcount_t *val)
{
#if defined(_WIN32)
  #ifdef RUN_OS_32
  return InterlockedDecrement(val) == 0;
  #else
  return InterlockedDecrement64(val) == 0;
  #endif
#elif defined(__APPLE__)
  #ifdef RUN_OS_32
  return OSAtomicDecrement32Barrier(val) == 0;
  #else
  return OSAtomicDecrement64Barrier(val) == 0;
  #endif
#else
  return g_atomic_int_dec_and_test(val);
#endif
}

inline refcount_t atomic_int_get(volatile refcount_t* val)
{
  #if !defined(_WIN32) && !defined(__APPLE__)
    return g_atomic_int_get(val);
  #else
    return *val;
  #endif
}

BASELIBRARY_PUBLIC_FUNC void threading_init();

struct BASELIBRARY_PUBLIC_FUNC Mutex
{
private:

#if GLIB_CHECK_VERSION(2,32,0)
  GMutex mutex;
#else
  GMutex *mutex;
#endif

  // these 2 would not work well because of d-tor semantics
  inline Mutex &operator = (const Mutex &o) { return *this; }
  Mutex(const Mutex &o) {}

public:
  Mutex();
  ~Mutex();

  void swap(Mutex &o);

  inline GMutex *gobj()
  {
#if GLIB_CHECK_VERSION(2,32,0)
    return &mutex;
#else
    return mutex;
#endif
  }

  void unlock()
  {
    g_mutex_unlock(gobj());
  }

  void lock()
  {
    g_mutex_lock(gobj());
  }

  bool try_lock()
  {
    return g_mutex_trylock(gobj()) != 0;
  }
};


struct BASELIBRARY_PUBLIC_FUNC MutexLock
{
protected:
  Mutex *ptr;

  MutexLock () : ptr(NULL) {}
public:
  MutexLock(Mutex &mutex);
  // take ownership of an existing lock (the other lock will be reset)
  MutexLock(const MutexLock &mlock);
  MutexLock &operator = (MutexLock &mlock);

  ~MutexLock();
};


class BASELIBRARY_PUBLIC_FUNC MutexTryLock : public MutexLock
{
public:
  MutexTryLock(Mutex &mtx) : MutexLock()
  {
    if(!mtx.try_lock())
      ptr = NULL;
    else
      ptr = &mtx;
  }

  void retry_lock(Mutex &mtx)
  {
    if (ptr != NULL)
      throw std::logic_error("Already holding another lock");

    if(!mtx.try_lock())
      ptr = NULL;
    else
      ptr = &mtx;

  }

  bool locked() const
  {
    return ptr != NULL;
  }
};

struct BASELIBRARY_PUBLIC_FUNC Cond
{
private:
#if GLIB_CHECK_VERSION(2,32,0)
  GCond cond;
#else
  GCond *cond;
#endif
public:
  Cond();
  ~Cond();

  inline GCond *gobj()
  {
#if GLIB_CHECK_VERSION(2,32,0)
    return &cond;
#else
    return cond;
#endif
  }

  void wait(Mutex &mutex)
  {
    g_cond_wait(gobj(), mutex.gobj());
  }

  void signal()
  {
    g_cond_signal(gobj());
  }

  void broadcast()
  {
    g_cond_broadcast(gobj());
  }
};


struct BASELIBRARY_PUBLIC_FUNC RecMutex
{
private:
#if GLIB_CHECK_VERSION(2,32,0)
  GRecMutex mutex;
#else
  GStaticRecMutex mutex;
#endif

  // these 2 would not work well because of d-tor semantics
  inline RecMutex &operator = (const RecMutex &o) { return *this; }
  RecMutex(const RecMutex &o) {}

public:
  RecMutex()
  {
#if GLIB_CHECK_VERSION(2,32,0)
    g_rec_mutex_init(&mutex);
#else
    g_static_rec_mutex_init(&mutex);
#endif
  }

  ~RecMutex()
  {
#if GLIB_CHECK_VERSION(2,32,0)
    g_rec_mutex_clear(&mutex);
#else
    g_static_rec_mutex_free(&mutex);
#endif
  }

#if GLIB_CHECK_VERSION(2,32,0)
  inline GRecMutex *gobj()
  {
    return &mutex;
  }
#else
  inline GStaticRecMutex *gobj()
  {
    return &mutex;
  }
#endif

  void unlock()
  {
#if GLIB_CHECK_VERSION(2,32,0)
    g_rec_mutex_unlock(gobj());
#else
    g_static_rec_mutex_unlock(gobj());
#endif
  }

  void lock()
  {
#if GLIB_CHECK_VERSION(2,32,0)
    g_rec_mutex_lock(gobj());
#else
    g_static_rec_mutex_lock(gobj());
#endif
  }

  bool try_lock()
  {
#if GLIB_CHECK_VERSION(2,32,0)
    return g_rec_mutex_trylock(gobj()) != 0;
#else
    return g_static_rec_mutex_trylock(gobj()) != 0;
#endif
  }
};


struct BASELIBRARY_PUBLIC_FUNC RecMutexLock
{
protected:
  RecMutex *ptr;

  RecMutexLock () : ptr(NULL) {}
public:
  RecMutexLock(RecMutex &mutex) : ptr(&mutex)
  {
    ptr->lock();
  }

  RecMutexLock(const RecMutexLock &mlock)
  : ptr(mlock.ptr)
  {
    const_cast<RecMutexLock*>(&mlock)->ptr = NULL;
  }

  RecMutexLock &operator = (RecMutexLock &mlock)
  {
    ptr = mlock.ptr;
    mlock.ptr = NULL;
    return *this;
  }

  ~RecMutexLock()
  {
    if (ptr)
      ptr->unlock();
  }
};


class BASELIBRARY_PUBLIC_FUNC RecMutexTryLock : public RecMutexLock
{
public:
  RecMutexTryLock(RecMutex &mtx) : RecMutexLock()
  {
    if(!mtx.try_lock())
      ptr = NULL;
    else
      ptr = &mtx;
  }

  void retry_lock(RecMutex &mtx)
  {
    if (ptr != NULL)
      throw std::logic_error("Already holding another lock");

    if(!mtx.try_lock())
      ptr = NULL;
    else
      ptr = &mtx;

  }

  bool locked() const
  {
    return ptr != NULL;
  }
};

#ifdef __APPLE__
  // boost 1.55 uses sem_init(), which is not implemented in OSX, so we have our own impl here to workaround
  struct BASELIBRARY_PUBLIC_FUNC Semaphore
  {
    sem_t *sem;
    Semaphore(int initial_count)
    {
      if ((sem = sem_open(base::strfmt("/wbsemaphore%p", this).c_str(), O_CREAT, 0644, initial_count)) == SEM_FAILED)
      {
        throw std::logic_error("creation of semaphore failed");
      }
    }

    ~Semaphore()
    {
      sem_close(sem);
      sem_unlink(base::strfmt("/wbsemaphore%p", this).c_str());
    }

    void post()
    {
      sem_post(sem);
    }

    void wait()
    {
      sem_wait(sem);
    }
  };
#else
  typedef boost::interprocess::interprocess_semaphore semaphore;
#endif
};
#endif
