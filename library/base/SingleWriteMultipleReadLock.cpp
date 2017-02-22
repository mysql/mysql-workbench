
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

#include "base/SingleWriteMultipleReadLock.h"


void SingleWriteMultipleReadLock::readLock() {
  std::unique_lock<std::mutex> lock(_mutex);
  while (_waitingWriters != 0)
    _readerGate.wait(lock);
  ++_activeReaders;
  lock.unlock();
}

void SingleWriteMultipleReadLock::readUnlock() {
  std::unique_lock<std::mutex> lock(_mutex);
  --_activeReaders;
  lock.unlock();
  _writerGate.notify_one();
}

void SingleWriteMultipleReadLock::writeLock() {
  std::unique_lock<std::mutex> lock(_mutex);
  ++_waitingWriters;
  while (_activeReaders != 0 || _activeWriters != 0)
    _writerGate.wait(lock);
  ++_activeWriters;
  lock.unlock();
}

void SingleWriteMultipleReadLock::writeUnlock() {
  std::unique_lock<std::mutex> lock(_mutex);
  --_waitingWriters;
  --_activeWriters;
  if (_waitingWriters > 0)
    _writerGate.notify_one();
  else
    _readerGate.notify_all();
  lock.unlock();
}
