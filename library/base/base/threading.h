/*
 * Copyright (c) 2013, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "common.h"

#include <stdexcept>
#include <glib.h>
#include <vector>
#include <string.h>

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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4275) // Exporting a class that is derived from a non-exportable class.
#endif

  class BASELIBRARY_PUBLIC_FUNC mutex_busy_error : public std::runtime_error {
  public:
    mutex_busy_error(const std::string &exc = "Mutex is busy") : std::runtime_error(exc) {
    }
  };

#ifdef _MSC_VER
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
    MutexLock(MutexLock &&o);
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
    Semaphore();
    Semaphore(int initialCount);
    ~Semaphore();
    Semaphore& operator=(const Semaphore& other) = delete;

    void post();
    void wait();

  private:
    class Private;
    Private *_d;
  };

} // namespace base
