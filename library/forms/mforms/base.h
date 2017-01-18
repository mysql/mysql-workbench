/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <list>

#include "base/threading.h"

#ifdef _WIN32
#ifdef DECL_MFORMS_EXPORT
#define MFORMS_EXPORT __declspec(dllexport)
#else
#define MFORMS_EXPORT __declspec(dllimport)
#endif
#else
#define MFORMS_EXPORT
#endif

#if defined(__APPLE__)
#ifdef nil
#undef nil
#endif
#define nil __DARWIN_NULL
#include <objc/objc-runtime.h>
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace mforms {

  // The root of of mforms hierarchy.
  class MFORMS_EXPORT Object {
  public:
    Object* retain();
    void release();
    virtual void set_managed();
    void set_release_on_add(bool flag = true);
    bool is_managed();
    bool release_on_add();

    // Below code is used for debugging
    inline base::refcount_t retain_count() const {
      return _refcount;
    }

    void set_destroying();
    bool is_destroying();

#ifndef SWIG

// Note: set_data and get_data should be used exclusively by the implementation code
// for each platform. Platform dependent code must stay in the header. Otherwise we cannot
// make the mforms stub to work for unit and integration tests.

#if defined(__APPLE__)
  public:
    Object();
    virtual ~Object();

    void set_data(id data);
    id get_data() const;

  private:
    id _data;
#else // !__APPLE__
  public:
    Object();
    virtual ~Object();

    typedef void (*FreeDataFn)(void*);
    void set_data(void* data, FreeDataFn free_fn = 0);

    template <class C>
    C* get_data() const {
      return reinterpret_cast<C*>(_data);
    }

    void* get_data_ptr() const;

  private:
    void* _data;
    FreeDataFn _data_free_fn;

#endif // !__APPLE__

    volatile mutable base::refcount_t _refcount;

    // We use only ptr's in mforms.
    Object(Object const& o) {
      throw std::logic_error("Copy c-tor unsupported in mforms::Object");
    }
    Object& operator=(Object const& o) {
      throw std::logic_error("Assignment operator not supported in mforms::Object");
      return *this;
    }

  protected:
    bool _managed;
    bool _release_on_add;

#endif // !SWIG

  private:
    bool _destroying;
  };

  //! Makes the passed object be released when retain count reaches 0
  // Must be called for objects allocated with new that you won't delete manually
  template <class C>
  C* manage(C* obj, bool release_on_add = true) {
    obj->set_managed();
    if (release_on_add)
      obj->set_release_on_add();
    return obj;
  }
};

#endif // !DOXYGEN_SHOULD_SKIP_THIS
