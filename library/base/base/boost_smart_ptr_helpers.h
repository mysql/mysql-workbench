/* 
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
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


#ifndef _BOOST_SMART_PTR_HELPERS_H_
#define _BOOST_SMART_PTR_HELPERS_H_


#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/weak_ptr.hpp>


template <class T>
boost::shared_ptr<T> shared_ptr_from(T *raw_ptr)
{
  boost::shared_ptr<T> res;
  if (raw_ptr)
  {
    try
    {
      dynamic_cast_shared_ptr(res, raw_ptr->shared_from_this());
    }
    catch (const boost::bad_weak_ptr&)
    {
    }
  }
  return res;
}

template <class DestType, class SrcType>
inline void dynamic_cast_shared_ptr(boost::shared_ptr<DestType> &dest, const boost::shared_ptr<SrcType> &src)
{
  dest= boost::dynamic_pointer_cast<DestType, SrcType>(src);
}

template <class T>
inline boost::weak_ptr<T> weak_ptr_from(T *raw_ptr)
{
  return shared_ptr_from(raw_ptr);
}


#define RAW_PTR_LOCK(ptr_type, raw_ptr) \
boost::shared_ptr<ptr_type> raw_ptr ## _ref= shared_ptr_from(raw_ptr); \

#define RETURN_IF_FAIL_TO_RETAIN_RAW_PTR(ptr_type, raw_ptr) \
RAW_PTR_LOCK (ptr_type, raw_ptr) \
{ \
  if (!raw_ptr) \
    return; \
}

#define RETVAL_IF_FAIL_TO_RETAIN_RAW_PTR(ptr_type, raw_ptr, return_value) \
RAW_PTR_LOCK (ptr_type, raw_ptr) \
{ \
  if (!raw_ptr) \
    return return_value; \
}


#define RETAIN_WEAK_PTR(ptr_type, weak_ptr, raw_ptr) \
boost::shared_ptr<ptr_type> raw_ptr ## _ref= (weak_ptr).lock(); \
ptr_type *raw_ptr= (raw_ptr ## _ref).get();

#define RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(ptr_type, weak_ptr, raw_ptr) \
RETAIN_WEAK_PTR (ptr_type, weak_ptr, raw_ptr) \
{ \
  if (!raw_ptr) \
    return; \
}

#define RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(ptr_type, weak_ptr, raw_ptr, return_value) \
RETAIN_WEAK_PTR (ptr_type, weak_ptr, raw_ptr) \
{ \
  if (!raw_ptr) \
    return return_value; \
}

// Helper functions for using boost::intrusive_ptr. Sample usage:
// class Cls : public base::IntrusiveRef
// {
//   public:
//     typedef boost::intrusive_ptr<Cls>    Ref;
//   ...
// };
struct IntrusiveRef
{
  IntrusiveRef() : _count(0) {}
  virtual ~IntrusiveRef() {};
  int _count;
};

inline void intrusive_ptr_add_ref(IntrusiveRef* r)
{
  ++r->_count;
}

inline void intrusive_ptr_release(IntrusiveRef* r)
{
  if (--r->_count <= 0)
    delete r;
}


#endif /* _BOOST_SMART_PTR_HELPERS_H_ */
