/* 
 * Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __BASE_MEMORY_H__
#define __BASE_MEMORY_H__

#include <cstdio>

namespace base
{

//==============================================================================
//
//==============================================================================
// This template is an analgue of std::auto_ptr in terms of semantic, it is only
// this template provides alternative way to free managed resource. For example
// this might get handy when working with FILE*
//
//     static void scope_fclose(FILE* fp)
//     {
//       if (fp)
//         fclose(fp);
//     }
//
//     typedef scope_ptr<FILE, &scope_fclose> FILE_auto_ptr;
//     ...
//     {
//       FILE_auto_ptr fp = g_fopen(file_name, "r");
//       while (!feof(fp))
//       {
//         fputc(fgetc(fp), fp);
//       }
//     } // After this brace fp goes out of scope and gets fclosed, also 
//       // no need to worry about exceptions

template <typename T, void FreeRoutine(T*)>
class scope_ptr
{
    mutable T   *_ptr;
  public:
    ~scope_ptr()
    {
      if (_ptr)
        FreeRoutine(_ptr);
    };

    scope_ptr() : _ptr(0) {};

    scope_ptr(T* r) : _ptr(r) {};

    scope_ptr(const scope_ptr& r)
    {
      if (_ptr)
        FreeRoutine(_ptr);
      _ptr = r._ptr;
      const_cast<scope_ptr&>(r)._ptr = 0;
    }
  
    T* get()
    {
      return _ptr;
    }

    scope_ptr& operator=(const scope_ptr& r)
    {
      if (_ptr)
        FreeRoutine(_ptr);
      _ptr = r._ptr;
      r._ptr = 0;
      return *this;
    }

    T& operator*() const throw()
    {
      return *_ptr;
    }

    T* operator->() const throw()
    {
      return _ptr;
    }

    operator T*() const
    {
      return _ptr;
    }

    operator bool() const
    {
      return _ptr != NULL;
    }
};
  
  
struct SetFlagOnLeaveScope
{
  SetFlagOnLeaveScope(bool *flag, bool value_when_leaving) : _flag(flag), _new_value(value_when_leaving) {}
  ~SetFlagOnLeaveScope() { *_flag = _new_value; }
  bool *_flag;
  bool _new_value;
};


// Here we define most usable scope ptrs
inline void FreeCharArray(char* p)
{
  delete[] p;
}
typedef scope_ptr<char, FreeCharArray>     AutoCharArray;


inline void scope_fclose(FILE* fp)
{
  if (fp)
    fclose(fp);
}
typedef scope_ptr<FILE, scope_fclose> FILE_scope_ptr;

}

#endif

