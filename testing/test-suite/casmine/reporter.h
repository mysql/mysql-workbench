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

#include <string>
#include <chrono>

#pragma once

namespace casmine {
  struct TestResult;
  
  class Reporter {
  public:
    Reporter();
    virtual ~Reporter() {};

    virtual void casmineStarted(size_t totalSpecsDefined);
    virtual void moduleStarted(TestResult const& result, size_t specsDefined);
    virtual void specStarted(TestResult const& result);
    virtual void specDone(TestResult const& result);
    virtual void moduleDone(TestResult const& result);
    virtual void casmineDone(std::chrono::microseconds duration);

  protected:
    size_t totalModuleCount = 0;
    size_t modulesSucceeded = 0;
    size_t modulesFailed = 0;
    size_t modulesPending = 0;
    size_t modulesDisabled = 0;
    size_t modulesExcluded = 0;

    size_t totalSpecCount = 0;
    size_t totalSucceededSpecCount = 0;
    size_t totalFailedSpecCount = 0;
    size_t totalPendingSpecCount = 0;
    size_t totalDisabledSpecCount = 0;
    size_t totalExcludedSpecCount = 0;

    size_t totalCheckCount = 0;
    size_t totalSucceededCheckCount = 0;

    size_t currentSpecCount = 0;
    size_t currentSpecsDefined;
    size_t currentSucceededSpecCount;
    size_t currentPendingSpecCount;
    size_t currentDisabledSpecCount;
    size_t currentExcludedSpecCount;

    std::string baseDir;

    std::string currentModuleFile;
    std::string currentSpecFile;
  };

  class ConsoleReporter: public Reporter {
  public:
    virtual void casmineStarted(size_t totalSpecsDefined) override;
    virtual void moduleStarted(TestResult const& result, size_t specsDefined) override;
    virtual void specStarted(TestResult const& result) override;
    virtual void specDone(TestResult const& result) override;
    virtual void moduleDone(TestResult const& result) override;
    virtual void casmineDone(std::chrono::microseconds duration) override;

  private:
    bool _printDetailTimes = false;
    std::string _moduleIntro;
  };

  class JsonReporter: public Reporter {
  public:
    virtual void casmineStarted(size_t totalSpecsDefined) override;
    virtual void moduleStarted(TestResult const& result, size_t specsDefined) override;
    virtual void specStarted(TestResult const& result) override;
    virtual void specDone(TestResult const& result) override;
    virtual void moduleDone(TestResult const& result) override;
    virtual void casmineDone(std::chrono::microseconds duration) override;

  private:
    std::string _json;
    std::string _moduleJson;
    std::string _specJson;
  };
}
