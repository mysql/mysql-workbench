/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

namespace mga {

  class ScriptingContext;
  class InternalTests {
  protected:

    struct failure {
      std::string _filename;
      unsigned int _line;
      std::string _message;
    };
    
    struct TestCounters {
      unsigned int _total;
      unsigned int _succeeded;
      std::vector<failure> _failures;
      TestCounters() : _total(0), _succeeded(0) {}
      bool allOk() { return _total == _succeeded; }
      void addFailure(const failure &failure) {
        _failures.push_back(failure);
      }
    };
    
    struct TestStatistics {
      const std::string _filename;
      unsigned int _line;
      std::map<std::string, std::map<std::string, TestCounters>> _stats;
      std::string _currentDescribe;
      std::string _currentIt;
      
      void expectSuccess() {
        TestCounters &counter = _stats[_currentDescribe][_currentIt];
        ++counter._total;
        ++counter._succeeded;
      }
      
      void expectFailed(const std::string &message) {
        TestCounters &counter = _stats[_currentDescribe][_currentIt];
        ++counter._total;
        counter.addFailure({ _filename, _line, message });
      }
      
      void setDescribe(const std::string &describe) {
        _currentDescribe =  describe;
        _currentIt = "";
      }
      
      void setIt(const std::string &it) {
        _currentIt = it;
      }
      
      TestCounters getItTotals(const std::string &it) {
        return _stats[_currentDescribe][it];
      }
      
      TestCounters getDescribeTotals(const std::string &describe) {
        TestCounters result;
        for (auto pair : _stats[describe]) {
          result._succeeded += pair.second._succeeded;
          result._total += pair.second._total;
        }
        return result;
      }
      
      TestCounters getTotals() {
        TestCounters result;
        for(auto descPair : _stats) {
          for (auto itPair : descPair.second) {
            result._succeeded += itPair.second._succeeded;
            result._total += itPair.second._total;
          }
        }
        return result;
      }
      
      TestStatistics(const std::string &filename) : _filename(filename), _line(0) {
      }
    };
    
    static TestStatistics _context;
    
    template <typename T>
    class Expect {
      const T &_value;
      
      template<typename Type>
      using InvokeType = typename Type::type;
      
      template<typename Condition>
      using EnableIf = InvokeType<std::enable_if<Condition::value>>;
      template<typename Condition>
      using EnableIfNot = InvokeType<std::enable_if<!Condition::value>>;
      
      template<typename Type>
      struct IsBase {
        static constexpr bool value = Type::value;
      };
      
      template<typename Type>
      struct IsVoidFunction : public IsBase<std::is_convertible<Type, std::function<void()>>> {};
      template<typename Type>
      struct IsString : public IsBase<std::is_same<Type, std::string>> {};

      void processResult(bool result, const std::string &failureMessage) {
        result ? _context.expectSuccess() : _context.expectFailed(failureMessage);
      }

      template<typename Type>
      const std::string toString(const Type& value) { return std::to_string(value); }
      const std::string toString(const std::string& value) { return (std::string("'") + value + "'"); }
      const std::string toString(const char * value) { return (std::string("'") + value + "'"); }
      const std::string toString(const std::nullptr_t &) { return "null"; }
      const std::string toString(void *value) { return Utilities::format("0x%08x", value); }
      const std::string toString(bool value) { return value ? "'true'" : "'false'"; }
      
    public:
      Expect(const T &value) : _value(value) {}
      ~Expect(){}

      template <typename T2, typename T1 = T, typename = EnableIf<IsVoidFunction<T1>>>
      void toBe(T2 value) {
        bool result = _value() == value;
        std::string error = Utilities::format("Expected %s to be %s", toString(result).c_str(), toString(value).c_str());
        processResult(result, error);
      }

      template <typename T2, typename T1 = T, typename = EnableIfNot<IsVoidFunction<T1>>, typename = void>
      void toBe(const T2 &value) {
        bool result = _value == T(value);
        std::string error = Utilities::format("Expected %s to be %s", toString(_value).c_str(), toString(value).c_str());
        processResult(result, error);
      }

      template <typename T2, typename T1 = T, typename = EnableIfNot<IsVoidFunction<T1>>, typename = void>
      void toBe(const char *value) {
        bool result = _value == T(value);
        std::string error = Utilities::format("Expected %s to be %s", toString(_value).c_str(), toString(value).c_str());
        processResult(result, error);
      }
      
      template <typename T2, typename T1 = T, typename = EnableIf<IsString<T1>>>
      void toContain(T2 value) {
        bool result = _value.find(value) != std::string::npos;
        std::string error = Utilities::format("Expected %s to contain %s", toString(_value).c_str(), toString(value).c_str());
        processResult(result, error);
      }

      template <typename T1 = T, typename = EnableIf<IsVoidFunction<T1>>>
      void toThrowError(const std::string &error) {
        try {
            _value();
        } catch(std::exception &e) {
          bool result = std::string(e.what()).substr(0, error.length()) == error;
          processResult(result, Utilities::format("Expected to throw '%s' but it throw '%s'", error.c_str(), e.what()));
          return;
        }
        processResult(false, Utilities::format("Expected to throw but it didn't"));
      }
      
      auto toContain(...) -> decltype("You need a container or a string to use this functionality") = delete;
      auto toThrowError(...) -> decltype("To be able to catch an exception, encapsulate it in a lambda") = delete;
    };
    
    static void it(const std::string &name, const std::function<void()> &func) {
      try {
        _context.setIt(name);
        func();
      } catch (std::exception e) { 
        _context.expectFailed(e.what());
      } catch (...) {
        _context.expectFailed("Unhandled exception...");
      }
    }

    static void describe(const std::string &name, std::function<void()> func) {
      _context.setDescribe(name);
      func();
    }

    template<typename T>
    static Expect<T> makeExpect(const T &value) { return Expect<T>(value); }
    
  public:
    static void activate(ScriptingContext &context, JSObject &exports);

  private:
    static bool _registered;
  };

} // namespace mga

