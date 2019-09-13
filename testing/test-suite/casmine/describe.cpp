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

#include "describe.h"
#include "casmine.h"

using namespace casmine;
using namespace std::chrono;

using namespace std::string_literals;

//----------------- It -------------------------------------------------------------------------------------------------

It::It(std::string const& name, std::function<void()> function, char const* file, size_t line, ItemRuntimeSpec type)
  : function(function), line(line) {
  result.runType = type;
  result.file = file;
  result.name = name;
}

//----------------- BeforeAfterInvoker ---------------------------------------------------------------------------------

BeforeAfterInvoker::BeforeAfterInvoker(std::function<void()> function, char const* file, size_t line)
  : function(function), fileName(file), line(line) {
}

//----------------- Describe -------------------------------------------------------------------------------------------

Describe::Describe(std::string const& name, char const* file, size_t line,
  ItemRuntimeSpec type) : _currentFile(file), _currentLine(line) {
  result.runType = type;
  result.file = file;
  result.name = name;
}

//----------------------------------------------------------------------------------------------------------------------

Describe::~Describe() {
}

//----------------------------------------------------------------------------------------------------------------------

void Describe::run(bool haveFocused) {
  auto context = CasmineContext::get();

  bool continueOnException = std::get<bool>(context->settings["continueOnException"]);
  bool stop = false;
  _currentIt = nullptr;

  if (haveFocused && !isFocused() && result.runType == ItemRuntimeSpec::Normal)
    result.runType = ItemRuntimeSpec::Excluded;

  context->sendModuleStartEvent(result, tests.size());

  high_resolution_clock::time_point moduleStart = high_resolution_clock::now();
  if (!result.pending && (result.runType == ItemRuntimeSpec::Normal || isFocused() || result.runType == ItemRuntimeSpec::Forced)) {
    stop = runWithExceptionFrame(result, std::bind(&Describe::prepareRun, this)); // Creates test data.
    if (!stop) {
      stop = runWithExceptionFrame(result, "beforeAll") && !continueOnException;

      if (!stop) {
        stop = runWithExceptionFrame(result, [&]() {
          for (It &item : tests) {
            _currentIt = &item;

            if (item.result.runType != ItemRuntimeSpec::Disabled
              && (!_hasFocusedTests || item.result.runType == ItemRuntimeSpec::Focused)) {
              high_resolution_clock::time_point specStart = high_resolution_clock::now();
              context->sendSpecStartEvent(item.result);

              stop = runWithExceptionFrame(item.result, "beforeEach") && !continueOnException;
              if (!stop) {
                sourceContext(item.result.file, item.line); // Might be overridden by the contained $expect() calls.
                stop = runWithExceptionFrame(item.result, item.function) && !continueOnException;

                if (!stop)
                  stop = runWithExceptionFrame(item.result, "afterEach") && !continueOnException;
              }
              item.result.duration = duration_cast<microseconds>(high_resolution_clock::now() - specStart);
              context->sendSpecDoneEvent(item.result);

            }
            else {
              if (item.result.runType == ItemRuntimeSpec::Normal)
                item.result.runType = ItemRuntimeSpec::Excluded;
              context->sendSpecStartEvent(item.result);
              context->sendSpecDoneEvent(item.result);
            }

            _currentIt = nullptr;

            if (stop)
              break;
          }

          stop = runWithExceptionFrame(result, "afterAll") && !continueOnException;
        });
      }
    }
  }

  result.duration = duration_cast<microseconds>(high_resolution_clock::now() - moduleStart);
  context->sendModuleDoneEvent(result);
}

//----------------------------------------------------------------------------------------------------------------------

bool Describe::isFocused() const {
  return result.runType == ItemRuntimeSpec::Focused || _hasFocusedTests;
}

//----------------------------------------------------------------------------------------------------------------------

void Describe::recordSuccess(const char *file, size_t line) {
  // Record the current location, which allows to specify at least a nearby location for exceptions.
  sourceContext(file, line);

  if (_currentIt != nullptr) {
    ++_currentIt->result.succeeded;
    ++_currentIt->result.total;
  }
  else {
    ++result.succeeded;
    ++result.total;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Describe::recordFailure(const char *file, size_t line, std::string const& message) {
  sourceContext(file, line);

  if (_currentIt != nullptr) {
    _currentIt->result.failures.emplace_back(file, line, message);
    ++_currentIt->result.total;
  }
  else {
    result.failures.emplace_back(file, line, message);
    ++result.total;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Describe::sourceContext(char const *file, size_t line) {
  _currentFile = file;
  _currentLine = line;
}

//----------------------------------------------------------------------------------------------------------------------

void Describe::markPending(std::string const& reason) {
  throw PendingException(reason);
}

//----------------------------------------------------------------------------------------------------------------------

bool Describe::runWithExceptionFrame(TestResult &result, std::function<void()> func) {
  try {
    func();
  } catch (PendingException &e) {
    result.pending = true;
    result.pendingReason = e.what();
  } catch (std::exception &e) {
    result.failures.push_back(Failure(_currentFile, _currentLine, "[exception] "s + e.what()));
    return true;
  } catch (std::string &s) {
    result.failures.push_back(Failure(_currentFile, _currentLine, "[exception] "s + s));
    return true;
  } catch (char const* s) {
    result.failures.push_back(Failure(_currentFile, _currentLine, "[exception] "s + s));
    return true;
  } catch (...) {
    result.failures.push_back(Failure(_currentFile, _currentLine, "[exception] Unknown"));
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool Describe::runWithExceptionFrame(TestResult &result, std::string const& invoker) {
  auto iterator = _invokers.find(invoker);
  if (iterator == _invokers.end())
    return false;

  sourceContext(iterator->second.fileName, iterator->second.line);
  return runWithExceptionFrame(result, iterator->second.function);
}

//----------------- DescribeInit ---------------------------------------------------------------------------------------

DescribeInit::DescribeInit(std::string const& name, char const* file, size_t line,
  ItemRuntimeSpec type) : name(name), file(file), line(line), type(type) {
  CasmineContext::get()->addInitializer(this);
}

//----------------------------------------------------------------------------------------------------------------------

#ifndef __APPLE__
#include <filesystem>
#endif

std::string DescribeInit::getName() {
#ifdef __APPLE__
  // The filesystem header is not available yet on macOS.
  std::string f { file };
  size_t sep = f.find_last_of("/");
  if (sep != std::string::npos)
    f = f.substr(sep + 1, f.size() - sep - 1);

  size_t dot = f.find_last_of(".");
  if (dot != std::string::npos)
    return f.substr(0, dot);
#else

  std::filesystem::path p(file);
  if (p.has_stem())
    return p.stem().string();
#endif

  return "";
}

//----------------------------------------------------------------------------------------------------------------------

ItemRuntimeSpec DescribeInit::getType() {
  return type;
}

//----------------------------------------------------------------------------------------------------------------------
