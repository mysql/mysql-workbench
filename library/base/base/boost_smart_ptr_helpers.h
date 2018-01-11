/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _BOOST_SMART_PTR_HELPERS_H_
#define _BOOST_SMART_PTR_HELPERS_H_

#include <boost/shared_ptr.hpp>

#include "base/log.h"

namespace BoostHelper {
  template <class shrPtr>
  class Container {
  public:
    shrPtr ptr;
    Container(){};
    Container(const Container &incoming) : ptr(incoming.ptr){};
    Container(const shrPtr &incoming) : ptr(incoming){};
    Container(Container &&incoming) : ptr(std::move(incoming.ptr)){};
    void operator()(...) {
      ptr.reset();
    }
  };
  template <class C>
  static std::shared_ptr<C> convertPointer(const boost::shared_ptr<C> &ptr) {
    typedef Container<std::shared_ptr<C>> ContainerType;
    ContainerType *c = boost::get_deleter<ContainerType, C>(ptr);
    if (c == NULL)
      return std::shared_ptr<C>(ptr.get(), Container<boost::shared_ptr<C>>(ptr));
    else
      return c->ptr;
  }
}

template <class T>
std::shared_ptr<T> shared_ptr_from(T *raw_ptr) {
  std::shared_ptr<T> res;
  if (raw_ptr) {
    try {
      dynamic_cast_shared_ptr(res, raw_ptr->shared_from_this());
    } catch (const std::bad_weak_ptr &exc) {
      DEFAULT_LOG_DOMAIN("smart_ptr_helpers")
      logError("Unable to dynamic_cast raw_ptr: %s", exc.what());
    }
  }
  return res;
}

template <class DestType, class SrcType>
inline void dynamic_cast_shared_ptr(std::shared_ptr<DestType> &dest, const std::shared_ptr<SrcType> &src) {
  dest = std::dynamic_pointer_cast<DestType, SrcType>(src);
}

template <class T>
inline std::weak_ptr<T> weak_ptr_from(T *raw_ptr) {
  return shared_ptr_from(raw_ptr);
}

#define RAW_PTR_LOCK(ptr_type, raw_ptr) std::shared_ptr<ptr_type> raw_ptr##_ref = shared_ptr_from(raw_ptr);

#define RETVAL_IF_FAIL_TO_RETAIN_RAW_PTR(ptr_type, raw_ptr, return_value) \
  RAW_PTR_LOCK(ptr_type, raw_ptr) {                                       \
    if (!raw_ptr)                                                         \
      return return_value;                                                \
  }

#define RETAIN_WEAK_PTR(ptr_type, weak_ptr, raw_ptr)           \
  std::shared_ptr<ptr_type> raw_ptr##_ref = (weak_ptr).lock(); \
  ptr_type *raw_ptr = (raw_ptr##_ref).get();

#define RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(ptr_type, weak_ptr, raw_ptr) \
  RETAIN_WEAK_PTR(ptr_type, weak_ptr, raw_ptr) {                       \
    if (!raw_ptr)                                                      \
      return;                                                          \
  }

#define RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(ptr_type, weak_ptr, raw_ptr, return_value) \
  RETAIN_WEAK_PTR(ptr_type, weak_ptr, raw_ptr) {                                     \
    if (!raw_ptr)                                                                    \
      return return_value;                                                           \
  }

// Helper functions for using boost::intrusive_ptr. Sample usage:
// class Cls : public base::IntrusiveRef
// {
//   public:
//     typedef boost::intrusive_ptr<Cls>    Ref;
//   ...
// };
struct IntrusiveRef {
  IntrusiveRef() : _count(0) {
  }
  virtual ~IntrusiveRef(){};
  int _count;
};

inline void intrusive_ptr_add_ref(IntrusiveRef *r) {
  ++r->_count;
}

inline void intrusive_ptr_release(IntrusiveRef *r) {
  if (--r->_count <= 0)
    delete r;
}

#endif /* _BOOST_SMART_PTR_HELPERS_H_ */
