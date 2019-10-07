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
#include "helpers.h"

namespace casmine {

void processResult(const char *file, size_t line, bool invertComparison, bool result, const std::string &message);

template<typename T>
class MatcherBase {
public:
  MatcherBase(const char *file, size_t line,  const T value, bool inverse) :
    file(file), line(line), actualValue(value), invertComparison(inverse) {}

protected:
  const char *file;
  size_t line;
  const T actualValue;
  bool invertComparison;

  void processResult(bool result, std::string const& message) const {
    casmine::processResult(file, line, invertComparison, result, message);
  }
};

template<typename T>
class ImplementsEqualOperator {
  template<typename U>
  struct ValueHolder {
    static U& value;
  };

  template<typename U>
  static std::true_type test(U *, decltype(ValueHolder<U>::value == ValueHolder<U>::value) * = nullptr);
  static std::false_type test(void *);

public:
  typedef decltype(test(static_cast<T *>(nullptr))) type;
  static const bool value = type::value;
};

template<typename T, typename = std::enable_if<std::is_scalar_v<T>>>
class MatcherScalar : public virtual MatcherBase<T> {
public:
  MatcherScalar(const char *file, size_t line, const T actual, bool inverse)
    : MatcherBase<T>(file, line, actual, inverse) {}

  using MatcherBase<T>::actualValue;
  using MatcherBase<T>::invertComparison;
  using MatcherBase<T>::processResult;

  template<typename T2>
  void reportResult(std::string message, bool result, std::string const& relation, T2 const& expected) const {
    if (message.empty()) {
      if (IsClass<T>()) {
        message = "Expected " + typeToString<T>() + " (" + toString(actualValue) + ")";
        if (invertComparison)
          message += " not";
        message += " to " + relation + " " + typeToString<T2>() + " (" + toString(expected) + ")";
      } else {
        message = "Expected " + toString(actualValue);
        if (invertComparison)
          message += " not";
        message += " to " + relation;
        auto expectString = toString(expected);
        if (expectString != "\"\"")
          message += " " + expectString;

      }
    }
    processResult(result, message);
  }

  template<typename T2>
  void toBe(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue == expected, "equal", expected);
  }

  // For object pointers we implement a strict identity handling.
  template<typename T2>
  void toEqual(T2 const& expected, std::string message = "") {
    if constexpr (IsPointer<T>() && IsClass<T>()
      && ImplementsEqualOperator<std::remove_pointer_t<std::decay_t<T2>>>::value)
      reportResult(message, *actualValue == *expected, "equal", expected);
    else
      toBe<T2>(expected, message);
  }

  template<typename T2>
  void toBeLessThan(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue < expected, "be less than", expected);
  }

  template<typename T2>
  void toBeLessThanOrEqual(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue <= expected, "be less than or equal to", expected);
  }

  template<typename T2>
  void toBeGreaterThan(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue > expected, "be greater than", expected);
  }

  template<typename T2>
  void toBeGreaterThanOrEqual(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue >= expected, "be greater than or equal", expected);
  }

  void toBeTrue(std::string message = "") {
    reportResult(message, static_cast<bool>(actualValue), "be true", "");
  }

  void toBeFalse(std::string message = "") {
    reportResult(message, !static_cast<bool>(actualValue), "be false", "");
  }

  template<typename T2, typename = std::enable_if<std::is_arithmetic_v<T>>>
  void toBeCloseTo(T2 expected, T2 precision, std::string message = "") {
    reportResult(message, std::abs(actualValue - expected) <= precision, "be close to", expected);
  }

  template<typename = std::enable_if<std::is_integral<T>::value>>
  void toBeOddNumber(std::string message = "") {
    if constexpr (std::is_signed<T>())
      reportResult(message, std::bitset<1>(std::abs(actualValue))[0] != 0, "be an odd number", "");
    else
      reportResult(message, std::bitset<1>(actualValue)[0] != 0, "be an odd number", "");
  }

  template<typename = std::enable_if<std::is_integral<T>::value>>
  void toBeEvenNumber(std::string message = "") {
    if constexpr (std::is_signed<T>())
      reportResult(message, std::bitset<1>(std::abs(actualValue))[0] == 0, "be an even number", "");
    else
      reportResult(message, std::bitset<1>(actualValue)[0] == 0, "be an even number", "");
  }

  template<typename = std::enable_if<std::is_arithmetic<T>::value>>
  void toBeWholeNumber(std::string message = "") {
    bool result = true;
    if (std::is_floating_point_v<T>) {
      float integralPart;
      float floatPart = modff(static_cast<float>(actualValue), &integralPart);
      result = floatPart == 0;
    }

    reportResult(message, result, "be a whole number", "");
  }

  template<typename = std::enable_if<std::is_arithmetic<T>::value>>
  void toBeWithinRange(T floor, T ceiling, std::string message = "") {
    reportResult(message, floor <= actualValue && actualValue <= ceiling,
                 "be within range", "[" + toString(floor) + ", " + toString(ceiling) + "]");
  }

  template<typename = std::enable_if<std::is_arithmetic<T>::value>>
  void toBeNaN(std::string message = "") {
    reportResult(message, std::isnan(static_cast<double>(actualValue)), "be", "NaN");
  }

  template<typename = std::enable_if<std::is_arithmetic<T>::value>>
  void toBeInf(std::string message = "") {
    reportResult(message, std::isinf(static_cast<double>(actualValue)), "be", "INF");
  }
};

template<typename T>
class MatcherString : public virtual MatcherBase<T> {
public:
  MatcherString(const char *file, size_t line, T const& actual, bool inverse) :
    MatcherBase<T>(file, line, actual, inverse) {}

  using MatcherBase<T>::actualValue;
  using MatcherBase<T>::invertComparison;
  using MatcherBase<T>::processResult;

  void reportResult(std::string message, bool result, std::string const& relation, std::string const& expected) const {
    if (message.empty()) {
      message = "Expected " + toString(actualValue);
      if (invertComparison)
        message += " not";
      message += " to " + relation;
      if (!expected.empty())
        message += " " + expected;
    }
    processResult(result, message);
  }

  template<typename T2>
  void toBe(T2 const& expected, std::string message = "") {
    this->template toEqual<T2>(expected, message);
  }

  template<typename T2>
  void toEqual(T2 const& expected, std::string message = "") {
    auto result = actualValue.compare(expected);
    reportResult(message, result == 0, "equal", toString(expected));
  }

  template<typename T2>
  void toEqual(std::initializer_list<T2> const& expected, std::string message = "") {
    T temp { expected.begin(), expected.end() };
    auto result = actualValue.compare(temp);
    reportResult(message, result == 0, "equal", toString(temp));
  }

  template<typename T2>
  void toBeLessThan(T2 const& expected, std::string message = "") {
    auto result = actualValue.compare(expected);
    reportResult(message, result < 0, "be less than", toString(expected));
  }

  template<typename T2>
  void toBeLessThan(std::initializer_list<T> const& expected, std::string message = "") {
    T temp { expected.begin(), expected.end() };
    auto result = actualValue.compare(temp);
    reportResult(message, result < 0, "be less than", toString(temp));
  }

  template<typename T2>
  void toBeLessThanOrEqual(T2 const& expected, std::string message = "") {
    auto result = actualValue.compare(T(expected));
    reportResult(message, result <= 0, "be less than or equal", toString(expected));
  }

  template<typename T2>
  void toBeLessThanOrEqual(std::initializer_list<T2> const& expected, std::string message = "") {
    T temp { expected.begin(), expected.end() };
    auto result = actualValue.compare(temp);
    reportResult(message, result <= 0, "be less than or equal", toString(temp));
  }

  template<typename T2>
  void toBeGreaterThan(T2 const& expected, std::string message = "") {
    auto result = actualValue.compare(T(expected));
    reportResult(message, result > 0, "be greater than", toString(expected));
  }

  template<typename T2>
  void toBeGreaterThan(std::initializer_list<T2> const& expected, std::string message = "") {
    T temp { expected.begin(), expected.end() };
    auto result = actualValue.compare(temp);
    reportResult(message, result > 0, "be greater than", toString(temp));
  }

  template<typename T2>
  void toBeGreaterThanOrEqual(T2 const& expected, std::string message = "") {
    auto result = actualValue.compare(T(expected));
    reportResult(message, result >= 0, "be greater than or equal", toString(expected));
  }

  template<typename T2>
  void toBeGreaterThanOrEqual(std::initializer_list<T2> const& expected, std::string message = "") {
    T temp { expected.begin(), expected.end() };
    auto result = actualValue.compare(temp);
    reportResult(message, result >= 0, "be greater than or equal", toString(temp));
  }

  template<typename T2>
  void toContain(T2 const& part, std::string message = "") {
    reportResult(message, actualValue.find(T(part)) != T::npos, "contain", toString(part));
  }

  template<typename T2>
  void toContain(std::initializer_list<T2> const& part, std::string message = "") {
    T temp { part.begin(), part.end() };
    reportResult(message, actualValue.find(temp) != T::npos, "contain", toString(temp));
  }

  template<typename T2>
  void toStartWith(T2 const& part, std::string message = "") {
    reportResult(message, actualValue.compare(0, T(part).size(), part) == 0, "start with", toString(part));
  }

  template<typename T2>
  void toStartWith(std::initializer_list<T2> const& part, std::string message = "") {
    T temp { part.begin(), part.end() };
    reportResult(message, actualValue.compare(0, temp.size(), temp) == 0, "start with", toString(temp));
  }

  template<typename T2>
  void toEndWith(T2 const& part, std::string message = "") {
    bool result = false;
    auto start = static_cast<long>(actualValue.size()) - static_cast<long>(T(part).size());
    if (start >= 0 && start < static_cast<long>(actualValue.size()))
      result = actualValue.compare(start, T::npos, part) == 0;

    reportResult(message, result, "end with", toString(part));
  }

  template<typename T2>
  void toEndWith(std::initializer_list<T2> const& part, std::string message = "") {
    T temp { part.begin(), part.end() };
    bool result = false;
    auto start = static_cast<long>(actualValue.size()) - static_cast<long>(temp.size());
    if (start >= 0 && start < static_cast<long>(actualValue.size()))
      result = actualValue.compare(start, T::npos, temp) == 0;

    reportResult(message, result, "end with", toString(temp));
  }

  void toContainOnlyWhiteSpaces(std::string message = "") {
    reportResult(message, actualValue.find_first_not_of(" \t\n\v\f\r") == T::npos, "be all whitespaces", "");
  }

  void toContainNoWhiteSpaces(std::string message = "") {
    reportResult(message, actualValue.find_first_of(" \t\n\v\f\r") == T::npos, "contain no whitespaces", "");
  }

  void toBeEmpty(std::string message = "") {
    reportResult(message, actualValue.empty(), "be empty", "");
  }

  template<typename T2>
  void toBeSameLengthAs(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue.size() == T(expected).size(), "be of same size as", toString(expected));
  }

  template<typename T2>
  void toBeLongerThan(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue.size() > T(expected).size(), "be longer than", toString(expected));
  }

  template<typename T2>
  void toBeShorterThan(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue.size() < T(expected).size(), "be shorter than", toString(expected));
  }

  template<typename T2>
  void toMatch(std::basic_regex<T2> const& r, std::string message = "") {
    reportResult(message, std::regex_match(actualValue, r), "to match the given pattern", "");
  }

  template<typename T2>
  void toMatch(std::basic_string<T2> const& pattern, std::string message = "") {
    std::basic_regex<T2> r(pattern);
    reportResult(message, std::regex_match(actualValue, r), "to match the given pattern", "");
  }

  template<typename T2>
  void toMatch(T2 const* pattern, std::string message = "") {
    std::basic_regex<T2> r(pattern);
    reportResult(message, std::regex_match(actualValue, r), "to match the given pattern", "");
  }

  /**
   * Compares the actual value with the content of the given file. Only reports a single success or fail event.
   */
  void toEqualContentOfFile(std::string const& expectedFilename, std::string message = "") {
    std::stringstream input(actualValue);
    std::ifstream expected;

#ifdef _MSC_VER
    auto temp = utf8ToUtf16(expectedFilename);
    expected.open(std::wstring(temp.begin(), temp.end()));
#else
    expected.open(expectedFilename);
#endif

    if (!expected.good()) {
      reportResult(message, false, "be compared with a valid file", "(" + expectedFilename +")");
      return;
    }

    input.seekg(0, std::ios::end);
    auto inputSize = input.tellg();

    expected.seekg(0, std::ios::end);
    auto expectedSize = expected.tellg();

    if (inputSize != expectedSize) {
      reportResult(message, false, "have same length as the expected file content", "(" + expectedFilename +")");
    }

    input.seekg(0, std::ios::beg);
    expected.seekg(0, std::ios::beg);

    std::string line, refline;
    size_t i = 0;
    while (expected.good() && input.good()) {
      ++i;
      getline(expected, refline);
      getline(input, line);
      if (line != refline) {
        if (invertComparison) {
          reportResult(message, true, "", "");
          return;
        }

        if (message.empty()) {
          message = "Line " + std::to_string(i) + " differs. Expected: \n\t" + toString(line) + "\nbut got:\n\t\"" +
            refline + "\"";
        }
        processResult(false, message);

        return;
      }
    }

    if (message.empty())
      message = "Expected input and file content to be different, but they are the same (" + expectedFilename +").";
    processResult(true, message);
  }
};

template<typename T>
class MatcherTypeSupport : public virtual MatcherBase<T> {
public:
  MatcherTypeSupport(const char *file, size_t line, T const& actual, bool inverse) :
    MatcherBase<T>(file, line, actual, inverse) {}

  using MatcherBase<T>::actualValue;
  using MatcherBase<T>::invertComparison;
  using MatcherBase<T>::processResult;

  template<typename T2>
  void toBeInstanceOf(std::string message = "") {
    using ExpectedType = typename std::decay_t<std::remove_pointer_t<T2>>;
    if (message.empty()) {
      message = "Expected type \"" + typeToString<T>() + "\"";
      if (invertComparison)
        message += " not";
      message += " to be derived from \"" + typeToString<T2>() + "\"";
    }
    processResult(dynamic_cast<T2 *>(actualValue) != nullptr, message);
  }

  template<typename T2>
  void toBeSameType(std::string message = "") {
    if (message.empty()) {
      message = "Expected \"" + typeToString<T>() + "\"";
      if (invertComparison)
        message += " not";
      message += " to be of type \"" + typeToString<T2>() + "\"";
    }
    processResult(std::is_same<T, T2>::value, message);
  }

};

template<typename T = std::function<void()>>
class MatcherException : public virtual MatcherBase<T> {
public:
  using MatcherBase<T>::actualValue;
  using MatcherBase<T>::invertComparison;
  using MatcherBase<T>::processResult;

  MatcherException(const char *file, size_t line, T const& func, bool inverse)
    : MatcherBase<T>(file, line, func, inverse) {}

  void toThrow(std::string const& message = "") const {
    bool exceptionFired = false;
    try {
      actualValue();
    } catch (...) {
      exceptionFired = true;
    }

    processResult(exceptionFired, message.empty() ? (!invertComparison ? "Expected to throw but it didn't" : "Expected not to throw but it did") : message);
  }

  template<typename ExceptionType = std::string>
  void toThrowError(std::string const& pattern, std::string const& message = "") const {
    std::regex matcher(pattern);
    std::string actualError;

    try {
      if constexpr (std::is_base_of_v<std::exception, ExceptionType>) {
        try {
          actualValue();
        } catch(ExceptionType const& e) {
          actualError = e.what();
        }
      } else if constexpr (std::is_base_of_v<std::exception, typename std::decay_t<std::remove_pointer_t<ExceptionType>>>) {
        try {
          actualValue();
        } catch(ExceptionType const& e) {
          actualError = e->what();
          delete e;
        }
      } else { // String type exceptions.
        try {
          actualValue();
        } catch (const char *s) {
          actualError = s;
        } catch (std::string const& s) {
          actualError = s;
        } catch (std::string *s) {
          actualError = "*" + *s;
          delete s;
        }
      }

      bool result = !actualError.empty();
      if (result) {
        result = std::regex_match(actualError, matcher);
        processResult(result, message.empty() ? "The pattern\" "s + pattern + "\" did not match the error \"" +
          actualError + "\"" : message);
      } else
        processResult(result, message.empty()
          ? (!invertComparison ? "Expected an exception but didn't get any" : "Expected no exception but got one")
          : message
        );
    } catch (...) {
      // Not the exception we expected. Try to find out what was thrown.
      std::string actualType;
      try {
        throw;
      } catch (std::exception const& e) {
        actualType = typeToString<decltype(e)>();
      } catch (std::exception *e) {
        actualType = typeToString<decltype(e)>();
        delete e;
      } catch (const char *) {
        actualType = "string";
      } catch (std::string const&) {
        actualType = "string";
      } catch (std::string *s) {
        actualType = "*string";
        delete s;
      } catch (...) {
        actualType = "an unkown exception";
      }
      processResult(false, "Expected exception of type \"" + typeToString<ExceptionType>() + "\" but got \"" +
                    actualType + "\"");
    }
  }
};

template<typename T>
class MatcherContainer : public virtual MatcherBase<T> {
public:
  using MatcherBase<T>::actualValue;
  using MatcherBase<T>::invertComparison;
  using MatcherBase<T>::processResult;

  MatcherContainer(const char *file, size_t line, T const& actual, bool inverse) :
    MatcherBase<T>(file, line, actual, inverse) {}

  void toContain(typename T::value_type const& expected, std::string message = "") const {
    if (message.empty()) {
      message = "Expected " + toString(expected);
      if (invertComparison)
        message += " not";
      message += " to be an element of " + casmine::containerToString(actualValue);
    }

    if constexpr (std::is_base_of_v<std::set<typename T::value_type>, T>)
      processResult(actualValue.count(expected) > 0, message);
    else
      processResult(std::find(actualValue.begin(), actualValue.end(), expected) != actualValue.end(), message);
  }

  void toContainValues(T const& expected, std::string message = "") const {
    if (message.empty()) {
      if (invertComparison) {
        message = "Expected no value from " + containerToString(expected) + " to be in " + containerToString(actualValue);
      } else {
        message = "Expected all values from " + containerToString(expected) + " to be in " + containerToString(actualValue);
      }
    }

    bool result = true;
    for (auto &item : expected) {
      bool found;
      if constexpr (std::is_base_of_v<std::set<typename T::value_type>, T>)
        found = actualValue.count(item);
      else
        found = std::find(actualValue.begin(), actualValue.end(), item) != actualValue.end();
      if (found ^ !invertComparison) {
        result = false;
        break;
      }
    }

    processResult(result, message);
  }

  void toHaveSize(std::size_t expected, std::string message = "") {
    if (message.empty()) {
      message = "Expected size " + toString(expected);
      if (invertComparison)
        message += " not";
      message += " to equal " + toString(actualValue.size());
    }

    processResult(actualValue.size() == expected, message);
  }

  void toEqual(T const& expected, std::string message = "") {
    processResult(actualValue == expected, message);
  }
};

template<typename Map>
class MatcherAssociativeContainer : public virtual MatcherBase<Map> {
public:
  using MatcherBase<Map>::actualValue;
  using MatcherBase<Map>::invertComparison;
  using MatcherBase<Map>::processResult;

  MatcherAssociativeContainer(const char *file, size_t line, Map const& actual, bool inverse)
    : MatcherBase<Map>(file, line, actual, inverse) {}

  void toContainKey(typename Map::key_type const& expected, std::string message = "") const {
    if (message.empty()) {
      message = "Expected " + toString(expected) + " ";
      if (invertComparison)
        message += "not ";
      message += "to be an existing key in " + casmine::containerToString(actualValue);
    }

    processResult(actualValue.find(expected) != actualValue.end(), message);
  }

  void toContainValue(typename Map::mapped_type const& expected, std::string message = "") const {
    if (message.empty()) {
      message = "Expected " + toString(expected) + " ";
      if (invertComparison)
        message += "not ";
      message += "to be an existing value in " + casmine::containerToString(actualValue);
    }

    auto result = std::find_if(actualValue.cbegin(), actualValue.cend(), [&](auto const& entry) {
      return entry.second == expected;
    });
    processResult(result != actualValue.end(), message);
  }

  void toHaveSize(std::size_t expected, std::string message = "") {
    if (message.empty()) {
      message = "Expected size " + toString(expected);
      if (invertComparison)
        message += "not ";
      message += "to equal " + toString(actualValue.size());
    }
    processResult(actualValue.size() == expected, message);
  }

  void toEqual(Map const& expected, std::string message = "") {
    processResult(actualValue == expected, message);
  }
};

template<typename T>
class MatcherNull : public virtual MatcherBase<T> {
public:
  MatcherNull(const char *file, size_t line, T const& actual, bool inverse) :
    MatcherBase<T>(file, line, actual, inverse) {}

  using MatcherBase<T>::actualValue;
  using MatcherBase<T>::invertComparison;
  using MatcherBase<T>::processResult;

  using BaseType = typename std::remove_const<typename std::remove_reference<T>::type>::type;

  void toBeNull(std::string message = "") {
    if (message.empty()) {
      message = "Expected value " + toString(actualValue);
      if (invertComparison)
        message += " not";
      message += " to be null";
    }

    if constexpr (IsWeakPointer<BaseType>::value) {
      std::shared_ptr<typename BaseType::element_type> temp = actualValue.lock();
      processResult(temp == nullptr, message);
    } else {
      processResult(actualValue == nullptr, message);
    }
  }

  void toBeValid(std::string message = "") {
    if (message.empty()) {
      message = "Expected value " + toString(actualValue);
      if (invertComparison)
        message += " not";
      message += " to be valid";
    }

    if constexpr (std::is_same<std::weak_ptr<typename BaseType::element_type>, BaseType>::value) {
      std::shared_ptr<typename BaseType::element_type> temp = actualValue.lock();
      processResult(temp != nullptr, message);
    } else {
      processResult(actualValue != nullptr, message);
    }
  }
};

template<typename T, typename = EnableIf<IsClass<T>>>
class MatcherClass : public virtual MatcherBase<T> {
public:
  MatcherClass(const char *file, size_t line, T const& actual, bool inverse)
    : MatcherBase<T>(file, line, actual, inverse) {}

  using MatcherBase<T>::actualValue;
  using MatcherBase<T>::invertComparison;
  using MatcherBase<T>::processResult;

  void reportResult(std::string message, bool result, std::string const& relation) const {
    if (message.empty()) {
      message = "Expected instance of class " + toString(actualValue);
      if (invertComparison)
        message += " not";
      message += " to " + relation + " the given instance";
    }
    processResult(result, message);
  }

  // The member toBe is not defined for classes (or references thereof) as this means to compare an object to itself.
  // The other methods are about object identity and usually require overloadeded comparison operators.

  template<typename T2>
  void toEqual(T2 const& expected, std::string message = "") {
    reportResult(message, actualValue == expected, "be equal compared to");
  }

  void toBeLessThan(T const& expected, std::string message = "") {
    reportResult(message, actualValue < expected, "be less than");
  }

  void toBeLessThanOrEqual(T const& expected, std::string message = "") {
    reportResult(message, actualValue <= expected, "be less than or equal compared to");
  }

  void toBeGreaterThan(T const& expected, std::string message = "") {
    reportResult(message, actualValue > expected, "be greater than");
  }

  void toBeGreaterThanOrEqual(T const& expected, std::string message = "") {
    reportResult(message, actualValue >= expected, "be greater than or equal compared to");
  }

};

}   //  namespace
