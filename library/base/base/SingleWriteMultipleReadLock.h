
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <condition_variable>

#endif

class BASELIBRARY_PUBLIC_FUNC SingleWriteMultipleReadLock {
public:
  void readLock();
  void readUnlock();
  void writeLock();
  void writeUnlock();

private:
  std::condition_variable _readerGate;
  std::condition_variable _writerGate;

  std::mutex _mutex;
  size_t _activeReaders = 0;
  size_t _waitingWriters = 0;
  size_t _activeWriters = 0;
};