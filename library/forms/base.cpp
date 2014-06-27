/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/base.h"
#include "mforms/mforms.h"

using namespace mforms;

//--------------------------------------------------------------------------------------------------

Object* Object::retain()
{
  base::atomic_int_inc(&_refcount);
  return this;
}

//--------------------------------------------------------------------------------------------------

void Object::release()
{
    if (base::atomic_int_dec_and_test_if_zero(&_refcount) && _managed)
      delete this;
}

//--------------------------------------------------------------------------------------------------

void Object::set_managed()
{
  _managed= true;
}

//--------------------------------------------------------------------------------------------------

void Object::set_release_on_add()
{
  _release_on_add = true;
}

//--------------------------------------------------------------------------------------------------

bool Object::is_managed()
{
  return _managed;
}

//--------------------------------------------------------------------------------------------------

bool Object::release_on_add()
{
  return _release_on_add;
}

//--------------------------------------------------------------------------------------------------

void Object::set_destroying()
{
  _destroying = true;
};

//--------------------------------------------------------------------------------------------------

bool Object::is_destroying()
{
  return _destroying;
};

//--------------------------------------------------------------------------------------------------

#ifndef SWIG
#if defined(__APPLE__) && !defined(MFORMS_STUB)
    
Object::Object()
  : _data(nil), _refcount(1), _managed(false), _release_on_add(false), _destroying(false)
{
  ControlFactory::get_instance()->instance_created();
}

//--------------------------------------------------------------------------------------------------

void Object::set_data(id data)
{
  objc_msgSend(_data, sel_getUid("release")); // [_data release]
  _data = data;
  objc_msgSend(data, sel_getUid("retain")); // [_data retain]
}

//--------------------------------------------------------------------------------------------------

id Object::get_data() const
{
  return _data;
}

//--------------------------------------------------------------------------------------------------

Object::~Object()
{
  /*objc_msgSend(_data, sel_getUid("release"));*/ /* calls [_data release] */
  ControlFactory::get_instance()->instance_destroyed();
}

#else // !__APPLE__

//--------------------------------------------------------------------------------------------------

Object::Object()
  : _data(0), _data_free_fn(0), _refcount(1), _managed(false), _release_on_add(false), _destroying(false)
{
  ControlFactory::get_instance()->instance_created();
}

//--------------------------------------------------------------------------------------------------

Object::~Object()
{
  if (_data_free_fn && _data)
    (*_data_free_fn)(_data);
  ControlFactory::get_instance()->instance_destroyed();
}

//--------------------------------------------------------------------------------------------------

void Object::set_data(void *data, FreeDataFn free_fn)
{
  _data = data;
  _data_free_fn = free_fn;
}

//--------------------------------------------------------------------------------------------------

void* Object::get_data_ptr() const
{
  return _data;
}

//--------------------------------------------------------------------------------------------------

#endif // !__APPLE__
#endif // ifndef SWIG
