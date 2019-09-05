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

namespace casmine {

class PendingException : public std::runtime_error {
public:
  PendingException(std::string const& reason): std::runtime_error(reason) {}
};

class Describe;

struct It {
  std::function<void ()> function;

  // Source position of the $it() construction. The file name is stored in the result for reporters.
  size_t line;
  TestResult result;

  It(std::string const& name, std::function<void ()> function, char const* file, size_t line, ItemRuntimeSpec type);
};

class BeforeAfterInvoker {
public:
  std::function<void ()> function;

  char const* fileName;
  size_t line;

  BeforeAfterInvoker(std::function<void ()> function, char const* file, size_t line);
};

class Describe {
public:
  std::function<void ()> function;

  Describe(std::string const& name, char const* file, size_t line, ItemRuntimeSpec type);
  virtual ~Describe();

  template<char const* file, size_t line, ItemRuntimeSpec type>
  void addIt(std::string const& name, std::function<void ()> func) {
    tests.emplace_back(name, func, file, line, type);
    if (type == ItemRuntimeSpec::Focused)
      _hasFocusedTests = true;
  }

  void run(bool haveFocused);

  template<char const* file, size_t line>
  void beforeAll(std::function<void ()> func) {
    _invokers.try_emplace("beforeAll", func, file, line);
  }

  template<char const* file, size_t line>
  void afterAll(std::function<void ()> func) {
    _invokers.try_emplace("afterAll", func, file, line);
  }

  template<char const* file, size_t line>
  void beforeEach(std::function<void ()> func) {
    _invokers.try_emplace("beforeEach", func, file, line);
  }

  template<char const* file, size_t line>
  void afterEach(std::function<void ()> func) {
    _invokers.try_emplace("afterEach", func, file, line);
  }

  void markForced() {
    result.runType = ItemRuntimeSpec::Forced;
  }

  void markDisabled() {
    result.runType = ItemRuntimeSpec::Disabled;
  }

  bool isFocused() const;
  size_t specCount() { return tests.size(); };

  void recordSuccess(const char *file, size_t line);
  void recordFailure(const char *file, size_t line, std::string const& message);

  void sourceContext(char const *file, size_t line);
  void markPending(std::string const& reason);

  virtual void prepareRun() = 0;

protected:
  std::vector<It> tests;

  TestResult result; // Results from beforeAll/afterAll.

  bool runWithExceptionFrame(TestResult &result, std::function<void ()> func);
  bool runWithExceptionFrame(TestResult &result, std::string const& invoker);

private:
  std::map<std::string, BeforeAfterInvoker> _invokers;

  It *_currentIt = nullptr;

  char const* _currentFile;
  size_t _currentLine = 0;
  bool _hasFocusedTests = false;
};

// This class registers itself with the CasmineContext, on static init.
class DescribeInit {
public:
  DescribeInit(std::string const& name, char const* file, size_t line, ItemRuntimeSpec type);
  ~DescribeInit() {};
  virtual Describe* initialize() = 0;

  std::string getName();
  ItemRuntimeSpec getType();
protected:
  std::string name;
  char const* file;
  size_t line;
  ItemRuntimeSpec type;
};

}   //  namespace
