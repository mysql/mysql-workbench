/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "common.h"
#include "matchers.h"

namespace casmine {

// The container providing access to a specific Expect instance, including its inverted variant.
template <typename Exp, typename BaseValueType = typename Exp::ValueType>
class ExpectTemplate : public Exp {
public:
  ExpectTemplate(const char *file, size_t line, BaseValueType const& value) :
    MatcherBase<BaseValueType>(file, line, value, false),
    Exp(file, line,  value, false),
    Not(file, line,  value) {}

  struct NOT : Exp {
    NOT(const char *file, size_t line, BaseValueType const& value) :
      MatcherBase<BaseValueType>(file, line, value, true),
      Exp(file, line,  value, true) {}
  } Not;

};

// The base class for all expectation classes.
template<typename T>
class Expect {
public:
  using ValueType = T;
};

//------------- Specialized Expect definitions -------------------------------------------------------------------------

template <typename T>
class ExpectScalar : public Expect<T>, public MatcherScalar<T> {
public:
  ExpectScalar(const char *file, size_t line, T const& value, bool inverse) :
    MatcherBase<T>(file, line, value, inverse),
    Expect<T>(),
    MatcherScalar<T>(file, line, value, inverse) {}
};

template <typename T>
class ExpectClass : public Expect<T>, public MatcherClass<T>, public MatcherTypeSupport<T> {
public:
  ExpectClass(const char *file, size_t line, T const& value, bool inverse) :
    MatcherBase<T>(file, line, value, inverse),
    Expect<T>(),
    MatcherClass<T>(file, line, value, inverse),
    MatcherTypeSupport<T>(file, line, value, inverse){}
};

template <typename T>
class ExpectString : public Expect<T>, public MatcherString<T> {
public:
  ExpectString(const char *file, size_t line, T const& value, bool inverse) :
    MatcherBase<T>(file, line, value, inverse),
    Expect<T>(),
    MatcherString<T>(file, line, value, inverse) {}
};

template <typename T = std::function<void()>>
class ExpectException : public Expect<T>, public MatcherException<T> {
public:
  ExpectException(const char *file, size_t line, T const& func, bool inverse) :
    MatcherBase<T>(file, line, func, inverse),
    Expect<T>(),
    MatcherException<T>(file, line, func, inverse) {}
};

template <typename T>
class ExpectContainer : public Expect<T>, public MatcherContainer<T> {
public:
  ExpectContainer(const char *file, size_t line, T const& value, bool inverse) :
    MatcherBase<T>(file, line, value, inverse),
    Expect<T>(),
    MatcherContainer<T>(file, line, value, inverse) {}
};

template <typename T>
class ExpectAssociativeContainer : public Expect<T>, public MatcherAssociativeContainer<T> {
public:
  ExpectAssociativeContainer(const char *file, size_t line, T const& value, bool inverse) :
    MatcherBase<T>(file, line, value, inverse),
    Expect<T>(),
    MatcherAssociativeContainer<T>(file, line, value, inverse) {}
};

template <typename T>
class ExpectPointer : public Expect<T>, public MatcherScalar<T>, public MatcherTypeSupport<T>, public MatcherNull<T> {
public:
  ExpectPointer(const char *file, size_t line, T const& value, bool inverse) :
    MatcherBase<T>(file, line, value, inverse),
    Expect<T>(),
    MatcherScalar<T>(file, line, value, inverse),
    MatcherTypeSupport<T>(file, line, value, inverse),
    MatcherNull<T>(file, line, value, inverse) {}
};

template <typename T>
class ExpectSmartPointer : public Expect<T>, public MatcherNull<T const&> {
public:
  ExpectSmartPointer(const char *file, size_t line, T const& value, bool inverse) :
    MatcherBase<T const&>(file, line, value, inverse),
    Expect<T>(),
    MatcherNull<T const&>(file, line, value, inverse) {}
};

struct DefaultExpects : EnvironmentBase {
  template<
    typename T,
    typename = EnableIf<std::is_scalar<T>>,
    typename = EnableIfNot<std::is_pointer<T>>,
    typename = EnableIfNot<IsString<T>>
  >
  static auto makeExpect(const char *file, size_t line, T value, int *dummy = nullptr) {
    return ExpectTemplate<ExpectScalar<T>>(file, line, value);
  }

  template<
    typename T,
    typename = EnableIf<std::is_pointer<T>>,
    typename = EnableIfNot<std::is_same<T, const char *>>,
    typename = EnableIfNot<std::is_same<T, const wchar_t *>>
  >
  static auto makeExpect(const char *file, size_t line, T value) {
    return ExpectTemplate<ExpectPointer<T>>(file, line, value);
  }

  template<
    typename T,
    typename = EnableIf<IsClass<T>>,
    typename = EnableIfNot<IsVoidFunction<T>>,
    typename = EnableIfNot<IsContainer<T>>,
    typename = EnableIfNot<IsAssociativeContainer<T>>,
    typename = EnableIfNot<IsSmartPointer<T>>,
    typename = EnableIfNot<IsPointer<T>>,
    typename = EnableIfNot<IsString<T>>
  >
  static auto makeExpect(const char *file, size_t line, T const& value, char *dummy = nullptr) {
    return ExpectTemplate<ExpectClass<T>>(file, line, value);
  }

  static auto makeExpect(const char *file, size_t line, std::function<void()> const& func) {
    return ExpectTemplate<ExpectException<>>(file, line, func);
  }

  template<typename CharT, typename Traits, typename Alloc>
  static auto makeExpect(const char *file, size_t line, std::basic_string<CharT, Traits, Alloc> const& value) {
    return  ExpectTemplate<ExpectString<std::basic_string<CharT, Traits, Alloc>>>(file, line, value);
  }

  static auto makeExpect(const char *file, size_t line, const char *value) {
    return ExpectTemplate<ExpectString<std::string>>(file, line, value);
  }

  static auto makeExpect(const char *file, size_t line, const wchar_t *value) {
    return ExpectTemplate<ExpectString<std::wstring>>(file, line, value);
  }

  // Available in C++20.
  // static auto makeExpect(const char *file, size_t line, const char8_t *value) {
  //  return Expect<ExpectString<std::u8string>>(file, line, value);
  // }

  static auto makeExpect(const char *file, size_t line, const char16_t *value) {
    return ExpectTemplate<ExpectString<std::u16string>>(file, line, value);
  }

  static auto makeExpect(const char *file, size_t line, const char32_t *value) {
    return ExpectTemplate<ExpectString<std::u32string>>(file, line, value);
  }

  template<
    typename Container,
    typename = EnableIf<IsContainer<Container>>,
    typename = EnableIfNot<IsAssociativeContainer<Container>>
  >
  static auto makeExpect(const char *file, size_t line, Container const& value, size_t *dummy = nullptr) {
    return ExpectTemplate<ExpectContainer<Container>, Container>(file, line, value);
  }

  template<
    typename Container,
    typename = EnableIf<IsAssociativeContainer<Container>>
  >
  static auto makeExpect(const char *file, size_t line, Container const& value, bool *dummy = nullptr) {
    return ExpectTemplate<ExpectAssociativeContainer<Container>, Container>(file, line, value);
  }

  template<
    typename T,
    typename = EnableIf<IsSmartPointer<T>>
  >
  static auto makeExpect(const char *file, size_t line, T const& value) {
    return ExpectTemplate<ExpectSmartPointer<T>, T const&>(file, line, value);
  }
};

}  //  namespace
