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

#include <type_traits>
#include <functional>
#include <string>
#include <variant>
#include <iostream>
#include <sstream>
#include <chrono>
#include <stdio.h>
#include <fstream>
#include <memory>
#include <regex>
#include <thread>
#include <atomic>
#include <mutex>
#include <any>
#include <optional>
#include <cmath>
#include <bitset>

#include <array>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <deque>
#include <forward_list>
#include <stack>
#include <queue>

#if !defined(_MSC_VER)
  #include <unistd.h>
  #include <wordexp.h>
#endif

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/error/en.h"

namespace casmine {

  enum class ItemRuntimeSpec {
    Normal,    // A normally executed spec.
    Focused,   // A higher prio spec, automatically excluding all normal specs.
    Disabled,  // A spec explicitly excluded from the suite.
    Forced,    // A spec that was forced on the command line.
    Excluded   // If at least one focused spec exists, all normal specs will be implicitly converted to excluded specs.
  };

  struct Failure {
    const char *file;
    size_t line;
    std::string message;

    Failure(char const *file, size_t line, std::string const& message): file(file), line(line), message(message) {}
  };

  struct TestResult {
    std::string name;

    const char *file;
    size_t total = 0;
    size_t succeeded = 0;
    ItemRuntimeSpec runType;

    bool pending = false;
    std::string pendingReason;

    std::chrono::microseconds duration = std::chrono::microseconds(0);
    std::vector<Failure> failures;
  };

  typedef std::variant<std::string, int, double, bool> ConfigMapVariableTypes;

  template<typename Type>
  using InvokeType = typename Type::type;

  // Helpers for conditional type support.
  template<typename Condition>
  using EnableIf = InvokeType<std::enable_if<Condition::value>>;
  template<typename Condition>
  using EnableIfNot = InvokeType<std::enable_if<!Condition::value>>;

  template<typename Type>
  struct IsVoidFunction : public std::is_convertible<Type, std::function<void()>>::type {};

  template<typename Type>
  struct IsString : public std::false_type {};
  template <typename charT, typename traits, typename Alloc>
  struct IsString<std::basic_string<charT, traits, Alloc> > : std::true_type {};

  template<typename Type>
  struct IsBool : public std::is_same<Type, bool>::type {};

  template<typename Type>
  struct IsPointer : public std::is_pointer<Type>::type {};

  template<typename Type>
  struct IsClass : public std::is_class<typename std::remove_pointer_t<typename std::remove_reference_t<Type>>>::type {};

  template<typename Type>
  struct IsArithmetic : public std::is_arithmetic<Type>::type {};

  template<typename T, typename T2 = void>
  struct IsContainer : std::false_type {};
  template<typename T, size_t N>
  struct IsContainer<std::array<T, N>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::list<T>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::set<T>> : std::true_type {};
  template<typename T1, typename T2>
  struct IsContainer<std::map<T1, T2>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::vector<T>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::multiset<T>> : std::true_type {};
  template<typename T1, typename T2>
  struct IsContainer<std::multimap<T1, T2>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::unordered_set<T>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::unordered_multiset<T>> : std::true_type {};
  template<typename T1, typename T2>
  struct IsContainer<std::unordered_map<T1, T2>> : std::true_type {};
  template<typename T1, typename T2>
  struct IsContainer<std::unordered_multimap<T1, T2>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::stack<T>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::queue<T>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::priority_queue<T>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::deque<T>> : std::true_type {};
  template<typename T>
  struct IsContainer<std::forward_list<T>> : std::true_type {};

  // Similar, but for associative containers only.
  template<typename T, typename T2 = void>
  struct IsAssociativeContainer : std::false_type {};
  template<typename T1, typename T2>
  struct IsAssociativeContainer<std::map<T1, T2>> : std::true_type {};
  template<typename T1, typename T2>
  struct IsAssociativeContainer<std::multimap<T1, T2>> : std::true_type {};
  template<typename T1, typename T2>
  struct IsAssociativeContainer<std::unordered_map<T1, T2>> : std::true_type {};
  template<typename T1, typename T2>
  struct IsAssociativeContainer<std::unordered_multimap<T1, T2>> : std::true_type {};

  template<typename T>
  struct IsWeakPointer : std::false_type {};
  template<typename T>
  struct IsWeakPointer<std::weak_ptr<T>> : std::true_type {};

  template<typename T>
  struct IsSmartPointer : std::false_type {};
  template<typename T>
  struct IsSmartPointer<std::weak_ptr<T>> : std::true_type {};
  template<typename T>
  struct IsSmartPointer<std::unique_ptr<T>> : std::true_type {};
  template<typename T>
  struct IsSmartPointer<std::shared_ptr<T>> : std::true_type {};

  template<typename... Interfaces>
  struct LocalEnvironmentAggregator : public Interfaces... {
    using Interfaces::makeExpect...;
  };

  struct EnvironmentBase {
    void makeExpect() {}
  };

}   //  namespace
