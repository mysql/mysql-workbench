/*
 * Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <algorithm>
#include <mutex>
#include <condition_variable>

#include "base/threading.h"

using namespace base;

class Mutex::Private {
public:
  std::mutex mutex;
};

Mutex::Mutex() {
  _d = new Private();
}

//----------------------------------------------------------------------------------------------------------------------

Mutex::~Mutex() {
  delete _d;
}

//----------------------------------------------------------------------------------------------------------------------

void Mutex::lock() {
  _d->mutex.lock();
}

//----------------------------------------------------------------------------------------------------------------------

void Mutex::unlock() {
  _d->mutex.unlock();
}

//----------------------------------------------------------------------------------------------------------------------

bool Mutex::tryLock() {
  return _d->mutex.try_lock();
}

//----------------------------------------------------------------------------------------------------------------------

class MutexLock::Private {
public:
  std::lock_guard<std::mutex> guard;

  Private(Mutex const &mutex): guard(mutex._d->mutex) {
  }
};

MutexLock::MutexLock(Mutex const &mutex) {
  _d = new Private(mutex);
}

//----------------------------------------------------------------------------------------------------------------------

MutexLock::MutexLock(MutexLock &&o) {
  // Move ownership of the underlying guard.
  _d = o._d;
  o._d = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

MutexLock::~MutexLock() {
  delete _d;
}

//----------------- RecMutex -------------------------------------------------------------------------------------------

class RecMutex::Private {
public:
  std::recursive_mutex mutex;
};

RecMutex::RecMutex() {
  _d = new Private();
}

//----------------------------------------------------------------------------------------------------------------------

RecMutex::~RecMutex() {
  delete _d;
}

//----------------------------------------------------------------------------------------------------------------------

void RecMutex::lock() {
  _d->mutex.lock();
}

//----------------------------------------------------------------------------------------------------------------------

void RecMutex::unlock() {
  _d->mutex.unlock();
}

//----------------------------------------------------------------------------------------------------------------------

bool RecMutex::tryLock() {
  return _d->mutex.try_lock();
}

//----------------------------------------------------------------------------------------------------------------------

class RecMutexLock::Private {
public:
  std::lock_guard<std::recursive_mutex> guard;

  Private(RecMutex const &mutex) : guard(mutex._d->mutex) {
  }
};

RecMutexLock::RecMutexLock(RecMutex &mutex, bool throwOnBlock) {
  if (throwOnBlock) {
    if (!mutex.tryLock())
      throw mutex_busy_error();
  }
  _d = new Private(mutex);

  if (throwOnBlock)
    mutex.unlock(); // Undo the tryLock() call.
}

//----------------------------------------------------------------------------------------------------------------------

RecMutexLock::RecMutexLock(RecMutexLock &&o) {
  // Move ownership of the underlying guard.
  _d = o._d;
  o._d = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

RecMutexLock::~RecMutexLock() {
  delete _d;
}

//----------------- Semaphore ------------------------------------------------------------------------------------------

class Semaphore::Private {
public:
  std::mutex mutex;
  std::condition_variable condition;
  int count;
};

Semaphore::Semaphore() {
  _d = new Private();
  _d->count = 0;
}

Semaphore::Semaphore(int initialCount) {
  _d = new Private();
  _d->count = initialCount;
}

//----------------------------------------------------------------------------------------------------------------------

Semaphore::~Semaphore() {
  delete _d;
  _d = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

void Semaphore::post() {
  std::unique_lock<std::mutex> lock(_d->mutex);
  _d->count++;
  _d->condition.notify_one();
}

//----------------------------------------------------------------------------------------------------------------------

void Semaphore::wait() {
  std::unique_lock<std::mutex> lock(_d->mutex);

  _d->condition.wait(lock, [this]() { return _d->count > 0; });
  _d->count--;
}

//----------------------------------------------------------------------------------------------------------------------
