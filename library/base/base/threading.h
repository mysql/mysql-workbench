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

#include <stdexcept>
#include <glib.h>
#include <vector>
#include <string.h>

namespace base {

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

  // An encapsulation of the std::mutex, std::recursive_mutex and std::lock_guard classes,
  // as we cannot include them in C++/CLI code.
  struct BASELIBRARY_PUBLIC_FUNC Mutex {
    friend struct MutexLock;

  public:
    Mutex();
    Mutex(Mutex const &o) = delete;
    Mutex(Mutex &&o) = delete;
    ~Mutex();

    Mutex &operator = (Mutex &o) = delete;

    void lock();
    void unlock();
    bool tryLock();

  private:
    class Private;
    Private *_d;
  };

  struct BASELIBRARY_PUBLIC_FUNC MutexLock {
  public:
    MutexLock(Mutex const &mutex);
    MutexLock(MutexLock const &o) = delete;
    MutexLock &operator=(MutexLock &o) = delete;

    ~MutexLock();

  private:
    class Private;
    Private *_d;
  };

  struct BASELIBRARY_PUBLIC_FUNC RecMutex {
    friend struct RecMutexLock;

  public:
    RecMutex();
    RecMutex(RecMutex const &o) = delete;
    RecMutex(RecMutex &&o) = delete;
    ~RecMutex();

    RecMutex &operator = (RecMutex &o) = delete;

    void lock();
    void unlock();
    bool tryLock();

  private:
    class Private;
    Private *_d;
  };

  struct BASELIBRARY_PUBLIC_FUNC RecMutexLock {
  public:
    RecMutexLock(RecMutex &mutex, bool throwOnBlock = false);
    RecMutexLock(RecMutexLock &&o);
    RecMutexLock(RecMutexLock const &o) = delete;
    RecMutexLock &operator=(RecMutexLock &o) = delete;

    ~RecMutexLock();

  private:
    class Private;
    Private *_d;
  };

  // A semaphore limits access to a bunch of resources to different threads. A count value determines
  // how many resources are available (and hence how many threads can use them at the same time).
  struct BASELIBRARY_PUBLIC_FUNC Semaphore {
  public:
    Semaphore(int initialCount);
    ~Semaphore();

    void post();
    void wait();

  private:
    class Private;
    Private *_d;
  };

} // namespace base
