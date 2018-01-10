/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __GRT_TEMPLATES_H__
#define __GRT_TEMPLATES_H__

#include "ConvUtils.h"

// This file should not contain any compilable .net public code
// because it is included by many projects, and any .net public code
// will cause conflicts
// XXX: ml: this file should actually be merged with ConvUtils.h. There is no GRT stuff involved here.

using namespace System;
using namespace System::Collections::Generic;

namespace MySQL {
  namespace Grt {

    template <typename S, typename T>
      static List<T ^> ^
      CppVectorToObjectList(const std::vector<S>& input) {
        typedef const std::vector<S> SourceContainerType;
        typedef List<T ^> TargetContainerType;

        TargetContainerType ^ result = gcnew TargetContainerType(static_cast<int>(input.size()));
        SourceContainerType::const_iterator e = input.end();

        for (SourceContainerType::const_iterator i = input.begin(); i != e; i++)
          result->Add(gcnew T(*i));

        return result;
      }

      // this conversion function is for simple types that map managed objects to
      // a native c++ object used by value, such as e.g. ::bec::NodeId

      template <typename S, typename T>
      static std::vector<T> ObjectListToCppVector(List<S ^> ^ input) {
      typedef const List<S ^> ^ SourceContainerType;
      typedef std::vector<T> TargetContainerType;

      TargetContainerType result;
      result.reserve(static_cast<int>(input->Count));

  for each(S ^ listitem in input) result.push_back(*listitem->get_unmanaged_object());

  return result;
    }

    template <typename S, typename T>
      static List<T ^> ^
      CppListToObjectList(const std::list<S>& input) {
        typedef const std::list<S> SourceContainerType;
        typedef List<T ^> TargetContainerType;

        TargetContainerType ^ result = gcnew TargetContainerType(static_cast<int>(input.size()));
        SourceContainerType::const_iterator e = input.end();

        for (SourceContainerType::const_iterator i = input.begin(); i != e; i++)
          result->Add(gcnew T(*i));

        return result;
      }

      // this conversion function is for simple types that map managed objects to
      // a native c++ object used by value, such as e.g. ::bec::NodeId

      template <typename S, typename T>
      static std::list<T> ObjectListToCppList(List<S ^> ^ input) {
      typedef const List<S ^> ^ SourceContainerType;
      typedef std::list<T> TargetContainerType;

      TargetContainerType result;
      result.reserve(static_cast<int>(input->Count));

  for each(S ^ listitem in input) result.push_back(*listitem->get_unmanaged_object());

  return result;
    }

    template <typename S, typename T>
      static List<T> ^
      CppListToNativeList(const std::list<S>& input) {
        typedef const std::list<S> SourceContainerType;
        typedef List<T> TargetContainerType;

        TargetContainerType ^ result = gcnew TargetContainerType(static_cast<int>(input.size()));
        SourceContainerType::const_iterator e = input.end();

        for (SourceContainerType::const_iterator i = input.begin(); i != e; i++)
          result->Add(T(*i));

        return result;
      }

      // this conversion function is for simple types that map managed objects to
      // a native c++ object used by value, such as e.g. ::bec::NodeId

      template <typename S, typename T>
      static std::list<T> NativeListToCppList(List<S> ^ input) {
      typedef const List<S> ^ SourceContainerType;
      typedef std::list<T> TargetContainerType;

      TargetContainerType result;

  for each(S listitem in input) result.push_back(listitem);

  return result;
    }

    template <typename S, typename T>
      static List<T> ^
      CppVectorToNativeList(const std::vector<S>& input) {
        typedef const std::vector<S> SourceContainerType;
        typedef List<T> TargetContainerType;

        TargetContainerType ^ result = gcnew TargetContainerType(static_cast<int>(input.size()));
        SourceContainerType::const_iterator e = input.end();

        for (SourceContainerType::const_iterator i = input.begin(); i != e; i++)
          result->Add(T(*i));

        return result;
      }

      template <typename S, typename T>
      static std::vector<T> NativeListToCppVector(List<S> ^ input) {
      typedef const List<S> ^ SourceContainerType;
      typedef std::vector<T> TargetContainerType;

      TargetContainerType result;
      result.reserve(static_cast<int>(input->Count));

  for each(S listitem in input) result.push_back(listitem);

  return result;
    }

    template <class T>
    struct EditorCentry {
      T* editor;
      EditorCentry(T* editor) : editor(editor) {
        editor->DisableAutoRefresh();
      }
      ~EditorCentry() {
        editor->EnableAutoRefresh();
      }
      T& operator*() {
        return *editor;
      }
      T& operator->() {
        return *editor;
      }
      operator T*() {
        return editor;
      }
      operator T&() {
        return *editor;
      }
    };

    template <typename T>
    public ref class ManagedRef {
    public:
      typedef typename T Type;
      typedef typename T::Ref RefType;
      ManagedRef(RefType* ref) : _inner(ref) {
      }
      ManagedRef(IntPtr nref) : _inner(new RefType(*((RefType*)(void*)nref))) {
      }
      Type* operator->() {
        return _inner->get();
      }
      RefType operator&() {
        return *_inner;
      }
      IntPtr operator~() {
        return (IntPtr)_inner;
      }

    private:
      ~ManagedRef() {
        reset();
      }
      !ManagedRef() {
        reset();
      }
      void reset() {
        delete _inner;
        _inner = NULL;
      }

    private:
      RefType* _inner;
    };

    template <typename N, typename M>
      M ^ Ref2Ptr(typename N::Ref ref) { return gcnew M(Ref_N2M<N>(ref)); }

      template <typename N>
      ManagedRef<N> ^ Ref_N2M(typename N::Ref nref) { return gcnew ManagedRef<N>(new N::Ref(nref)); }

      /*
        this conversion is to be used when need a pointer to managed class residing in other assembly.
        native classes are invisible in this case, hence we can't use helper functions relying on strict native types.
        managed class must define appropriate constructor, accepting int parameter.
      */
      template <typename N, typename M>
      M ^ Ref2Ptr_(typename N::Ref ref) { return gcnew M((IntPtr)&ref); }

  } // namespace Grt
} // namespace MySQL

#endif // __GRT_TEMPLATES_H__
