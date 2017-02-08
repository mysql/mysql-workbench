/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "common.h"

#ifndef HAVE_PRECOMPILED_HEADERS

#include <stdexcept>
#include <glib.h>
#include <vector>
#include <string.h>

#endif

namespace base {

  // Launches one of the tools of WB, which are command line applications which resided side by side
  // to the WB executable or even as part of the application bundle (on OSX).
  // The caller is responsible for providing the full path, no searching is done here.
  BASELIBRARY_PUBLIC_FUNC void launchTool(const std::string &path, const std::vector<std::string> &params);

  // Launches a separate application which can sit anywhere, especially outside of the application bundle (on OSX).
  // The function uses a platform specific search strategy if no path is given (usually the current application
  // or bundle folder, then the typical location for apps on that platform).
  BASELIBRARY_PUBLIC_FUNC void launchApplication(const std::string &name, const std::vector<std::string> &params);

  typedef gint refcount_t;

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4275) // Exporting a class that is derived from a non-exportable class.
#endif

  class BASELIBRARY_PUBLIC_FUNC mutex_busy_error : public std::runtime_error {
  public:
    mutex_busy_error(const std::string &exc = "Mutex is busy") : std::runtime_error(exc) {
    }
  };

#ifdef _WIN32
#pragma warning(pop)
#endif

  inline GThread *create_thread(GThreadFunc func, gpointer data, GError **error = NULL, std::string name = "") {
#if GLIB_CHECK_VERSION(2, 32, 0)
    return g_thread_try_new(name.c_str(), func, data, error);
#else
    return g_thread_create(func, data, TRUE, error);
#endif
  }

  BASELIBRARY_PUBLIC_FUNC void threading_init();

  struct BASELIBRARY_PUBLIC_FUNC Mutex {
  private:
#if GLIB_CHECK_VERSION(2, 32, 0)
    GMutex mutex;
#else
    GMutex *mutex;
#endif

    // these 2 would not work well because of d-tor semantics
    inline Mutex &operator=(const Mutex &o) {
      return *this;
    }
    Mutex(const Mutex &o) {
    }

  public:
    Mutex();
    ~Mutex();

    void swap(Mutex &o);

    inline GMutex *gobj() {
#if GLIB_CHECK_VERSION(2, 32, 0)
      return &mutex;
#else
      return mutex;
#endif
    }

    void unlock() {
      g_mutex_unlock(gobj());
    }

    void lock() {
      g_mutex_lock(gobj());
    }

    bool try_lock() {
      return g_mutex_trylock(gobj()) != 0;
    }
  };

  struct BASELIBRARY_PUBLIC_FUNC MutexLock {
  protected:
    Mutex *ptr;

    MutexLock() : ptr(NULL) {
    }

  public:
    MutexLock(Mutex &mutex);
    // take ownership of an existing lock (the other lock will be reset)
    MutexLock(const MutexLock &mlock);
    MutexLock &operator=(MutexLock &mlock);

    ~MutexLock();
  };

  class BASELIBRARY_PUBLIC_FUNC MutexTryLock : public MutexLock {
  public:
    MutexTryLock(Mutex &mtx) : MutexLock() {
      if (!mtx.try_lock())
        ptr = NULL;
      else
        ptr = &mtx;
    }

    void retry_lock(Mutex &mtx) {
      if (ptr != NULL)
        throw std::logic_error("Already holding another lock");

      if (!mtx.try_lock())
        ptr = NULL;
      else
        ptr = &mtx;
    }

    bool locked() const {
      return ptr != NULL;
    }
  };

  struct BASELIBRARY_PUBLIC_FUNC Cond {
  private:
#if GLIB_CHECK_VERSION(2, 32, 0)
    GCond cond;
#else
    GCond *cond;
#endif
  public:
    Cond();
    ~Cond();

    inline GCond *gobj() {
#if GLIB_CHECK_VERSION(2, 32, 0)
      return &cond;
#else
      return cond;
#endif
    }

    void wait(Mutex &mutex) {
      g_cond_wait(gobj(), mutex.gobj());
    }

    void signal() {
      g_cond_signal(gobj());
    }

    void broadcast() {
      g_cond_broadcast(gobj());
    }
  };

  struct BASELIBRARY_PUBLIC_FUNC RecMutex {
  private:
#if GLIB_CHECK_VERSION(2, 32, 0)
    GRecMutex mutex;
#else
    GStaticRecMutex mutex;
#endif

    // These 2 would not work well because of d-tor semantics.
    inline RecMutex &operator=(const RecMutex &o) {
      return *this;
    }
    RecMutex(const RecMutex &o) {
    }

  public:
    RecMutex() {
#if GLIB_CHECK_VERSION(2, 32, 0)
      g_rec_mutex_init(&mutex);
#else
      g_static_rec_mutex_init(&mutex);
#endif
    }

    ~RecMutex() {
#if GLIB_CHECK_VERSION(2, 32, 0)
      g_rec_mutex_clear(&mutex);
#else
      g_static_rec_mutex_free(&mutex);
#endif
    }

#if GLIB_CHECK_VERSION(2, 32, 0)
    inline GRecMutex *gobj() {
      return &mutex;
    }
#else
    inline GStaticRecMutex *gobj() {
      return &mutex;
    }
#endif

    void unlock() {
#if GLIB_CHECK_VERSION(2, 32, 0)
      g_rec_mutex_unlock(gobj());
#else
      g_static_rec_mutex_unlock(gobj());
#endif
    }

    void lock() {
#if GLIB_CHECK_VERSION(2, 32, 0)
      g_rec_mutex_lock(gobj());
#else
      g_static_rec_mutex_lock(gobj());
#endif
    }

    bool try_lock() {
#if GLIB_CHECK_VERSION(2, 32, 0)
      return g_rec_mutex_trylock(gobj()) != 0;
#else
      return g_static_rec_mutex_trylock(gobj()) != 0;
#endif
    }
  };

  struct BASELIBRARY_PUBLIC_FUNC RecMutexLock {
  protected:
    RecMutex *ptr;

    RecMutexLock() : ptr(NULL) {
    }

  public:
    RecMutexLock(RecMutex &mutex, bool throw_on_block = false) : ptr(&mutex) {
      if (throw_on_block) {
        if (!ptr->try_lock())
          throw mutex_busy_error();
      } else
        ptr->lock();
    }

    RecMutexLock(const RecMutexLock &mlock) : ptr(mlock.ptr) {
      const_cast<RecMutexLock *>(&mlock)->ptr = NULL;
    }

    RecMutexLock &operator=(RecMutexLock &mlock) {
      ptr = mlock.ptr;
      mlock.ptr = NULL;
      return *this;
    }

    ~RecMutexLock() {
      if (ptr)
        ptr->unlock();
    }
  };

  class BASELIBRARY_PUBLIC_FUNC RecMutexTryLock : public RecMutexLock {
  public:
    RecMutexTryLock(RecMutex &mtx) : RecMutexLock() {
      if (!mtx.try_lock())
        ptr = NULL;
      else
        ptr = &mtx;
    }

    void retry_lock(RecMutex &mtx) {
      if (ptr != NULL)
        throw std::logic_error("Already holding another lock");

      if (!mtx.try_lock())
        ptr = NULL;
      else
        ptr = &mtx;
    }

    bool locked() const {
      return ptr != NULL;
    }
  };

  // A semaphore limits access to a bunch of resources to different threads. A count value determines
  // how many resources are available (and hence how many threads can use them at the same time).
  struct BASELIBRARY_PUBLIC_FUNC Semaphore {
  private:
    GAsyncQueue *_queue;

  public:
    Semaphore(int initial_count);
    ~Semaphore();

    void post();
    void wait();
    bool try_wait();
  };
}
