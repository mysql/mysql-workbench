/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include <type_traits>
#include <utility>
#include <typeinfo>

namespace base {

  template <class T>
  using StorageType = typename std::decay<T>::type;

  struct any {
    bool isNull() const {
      return !ptr;
    }

    template <typename U>
    any(U&& value, typename std::enable_if<!std::is_same<typename std::decay<U>::type, any>::value>::type* = nullptr)
      : ptr(new Derived<StorageType<U>>(std::forward<U>(value))) {
    }

    template <class U>
    bool is() const {
      typedef StorageType<U> T;

      auto derived = dynamic_cast<Derived<T>*>(ptr);
      return derived != nullptr;
    }

    template <class U>
    StorageType<U>& as() {
      typedef StorageType<U> T;

      auto derived = dynamic_cast<Derived<T>*>(ptr);

      if (!derived)
        throw std::bad_cast();

      return derived->value;
    }

    template <class U>
    StorageType<U>& as() const {
      typedef StorageType<U> T;

      auto derived = dynamic_cast<Derived<T>*>(ptr);

      if (!derived)
        throw std::bad_cast();

      return derived->value;
    }

    template <class U>
    operator U() {
      return as<StorageType<U>>();
    }

    template <class U>
    operator U() const {
      return as<StorageType<U>>();
    }

    any() : ptr(nullptr) {
    }

    any(any&& that) : ptr(that.ptr) {
      that.ptr = nullptr;
    }

    any(const any& that) : ptr(that.clone()) {
    }

    any& operator=(const any& a) {
      if (ptr == a.ptr)
        return *this;

      auto old_ptr = ptr;

      ptr = a.clone();

      delete old_ptr;

      return *this;
    }

    any& operator=(any&& a) {
      if (ptr == a.ptr)
        return *this;

      std::swap(ptr, a.ptr);

      return *this;
    }

    ~any() {
      delete ptr;
    }

  private:
    struct Base {
      virtual ~Base() {
      }

      virtual Base* clone() const = 0;
    };

    template <typename T>
    struct Derived : Base {
      template <typename U>
      Derived(U&& value) : value(std::forward<U>(value)) {
      }

      T value;

      Base* clone() const {
        return new Derived<T>(value);
      }
    };

    Base* clone() const {
      if (ptr)
        return ptr->clone();
      else
        return nullptr;
    }

    Base* ptr;
  };

} /* namespace base */
