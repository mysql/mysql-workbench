/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/password_cache.h"

#include "base/log.h"
#include "base/threading.h"
#include <errno.h>
#include <cstdlib>
#include <cstring>

DEFAULT_LOG_DOMAIN("pwdcache");

#ifndef _MSC_VER
#include <sys/mman.h>
#define HAVE_MLOCK 1
#endif

using namespace mforms;

PasswordCache::PasswordCache() {
  storage_len = 0;
  storage_size = 4 * 1024;
  storage = (char *)malloc(storage_size);
  if (!storage)
    logError("Unable to allocate memory for password cache, caching will be disabled (errno %i)\n", errno);
#ifdef HAVE_MLOCK
  else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    if (mlock(storage, storage_size) < 0) {
      logError("mlock password cache (errno %i)\n", errno);
      free(storage);
      storage = NULL;
    }
#pragma GCC diagnostic pop
  }
#endif
}

PasswordCache PasswordCache::instance;
static base::Mutex cache_mutex;

PasswordCache *PasswordCache::get() {
  return &instance;
}

PasswordCache::~PasswordCache() {
  if (storage) {
    memset(storage, 0, storage_size);
#ifdef HAVE_MLOCK
    if (munlock(storage, storage_size) < 0)
      logError("munlock password cache failed (errno %i)\n", errno);
#endif
    free(storage);
  }
}

void PasswordCache::add_password(const std::string &service, const std::string &account, const char *password) {
  if (storage) {
    if (!password)
      password = "";

    bool flag = false;
    {
      base::MutexLock lock(cache_mutex);

      const char *opassword = find_password(service, account);
      if (opassword) {
        if (strcmp(password, opassword) == 0)
          return;
        flag = true;
      }
    }
    if (flag)
      remove_password(service, account);

    base::MutexLock lock(cache_mutex);

    size_t reclen = sizeof(size_t) + service.size() + 1 + account.size() + 1 + strlen(password) + 1;

    // increase buffer size if new record doesnt fit
    while (storage_len + reclen > storage_size) {
      char *new_block;
      size_t new_size = storage_size + 4 * 1024;

      new_block = (char *)malloc(new_size);
      if (new_block) {
#ifdef HAVE_MLOCK
        if (mlock(new_block, new_size) < 0) {
          logError("mlock password cache (errno %i)\n", errno);
          free(new_block);
          throw std::runtime_error("Could not increase password cache size");
        }
#endif
        memcpy(new_block, storage, storage_len);
        memset(storage, 0, storage_size);
#ifdef HAVE_MLOCK
        if (munlock(storage, storage_size) < 0)
          logError("munlock password cache (errno %i)\n", errno);
#endif
        free(storage);
        storage = new_block;
        storage_size = new_size;
      } else
        throw std::runtime_error("Could not increase password cache size");
    }

    // store length of this record
    *(size_t *)(storage + storage_len) = reclen;
    storage_len += sizeof(reclen);
    // store contents
    memcpy(storage + storage_len, service.data(), service.size() + 1);
    storage_len += service.size() + 1;
    memcpy(storage + storage_len, account.data(), account.size() + 1);
    storage_len += account.size() + 1;
    memcpy(storage + storage_len, password, strlen(password) + 1);
    storage_len += strlen(password) + 1;
  } else
    throw std::runtime_error("Password storage is not available");
}

size_t PasswordCache::find_block(const std::string &service, const std::string &account) {
  size_t offset = 0;
  while (offset < storage_len) {
    size_t recsize = *(size_t *)(storage + offset);
    const char *recservice = storage + offset + sizeof(recsize);
    const char *recaccount = storage + offset + sizeof(recsize) + strlen(recservice) + 1;

    if (strcmp(recservice, service.c_str()) == 0 && strcmp(recaccount, account.c_str()) == 0)
      return offset;

    offset += recsize;
  }
  return (size_t)-1;
}

void PasswordCache::remove_password(const std::string &service, const std::string &account) {
  if (storage) {
    base::MutexLock lock(cache_mutex);

    size_t offset = find_block(service, account);
    if (offset != (size_t)-1) {
      size_t recsize = *(size_t *)(storage + offset);

      memmove(storage + offset, storage + offset + recsize, storage_len - recsize);
      storage_len -= recsize;
    }
  }
}

const char *PasswordCache::find_password(const std::string &service, const std::string &account) {
  if (storage) {
    size_t offset = find_block(service, account);
    if (offset != (size_t)-1) {
      const char *pwd = storage + offset + sizeof(size_t) + service.size() + 1 + account.size() + 1;
      return pwd;
    }
  }
  return 0;
}

bool PasswordCache::get_password(const std::string &service, const std::string &account, std::string &ret_password) {
  base::MutexLock lock(cache_mutex);
  const char *tmp = find_password(service, account);
  if (tmp)
    ret_password = tmp;
  return tmp != NULL;
}
