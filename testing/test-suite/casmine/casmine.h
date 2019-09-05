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
#include "describe.h"
#include "expect.h"
#include "reporter.h"

namespace casmine {

#define $LOCAL_ENV(...) \
template <class... Args> \
using INNER = casmine::LocalEnvironmentAggregator<casmine::DefaultExpects, _ENV_EXT_, Args...>; \
typedef INNER<__VA_ARGS__> LocalEnvironment;

#define $ModuleEnvironment(...) \
struct _ENV_EXT_; \
\
static char _N_[] = __FILE__; \
$LOCAL_ENV(__VA_ARGS__) \
struct _ENV_EXT_ : casmine::EnvironmentBase

#define $TestData struct TestData

// OptionalData allows to let the compiler deal with existing and non-existing test data.
#define $DEFINE_IMPL \
template<typename, typename = void> class OptionalData { protected: void prepareData() {} }; \
template<typename T> class OptionalData<T, std::void_t<decltype(sizeof(T))>> { \
  protected: std::unique_ptr<T> data; \
  void prepareData() { data = std::make_unique<T>(); } \
}; \
\
class DescribeImpl : public OptionalData<struct TestData>, public casmine::Describe { \
public: \
  DescribeImpl(std::string const& name, char const* file, size_t line, casmine::ItemRuntimeSpec type) \
  : Describe(name, file, line, type) { \
    runWithExceptionFrame(result, std::bind(&DescribeImpl::runDescribe, this)); \
  } \
  virtual ~DescribeImpl() override {};\
  virtual void prepareRun() override { prepareData(); } \
 \
  void runDescribe(); \
};

#define $DESCRIBE_INIT(describeName, describeType) \
template<char const* file_, size_t line_, casmine::ItemRuntimeSpec type_> \
class DescribeInitImpl : public casmine::DescribeInit { \
public: \
  DescribeInitImpl(std::string const& name): DescribeInit(name, file_, line_, type_) {}; \
  virtual ~DescribeInitImpl() {}; \
  virtual casmine::Describe* initialize() override { return new DescribeImpl(name, file, line, type); } \
}; \
static DescribeInitImpl<_N_, __LINE__, describeType> _init_(describeName);

#define $describe(describeName) \
static_assert(casmine::length(describeName) > 0, "describe name must not be empty"); \
$DEFINE_IMPL \
$DESCRIBE_INIT(describeName, casmine::ItemRuntimeSpec::Normal) \
void DescribeImpl::runDescribe()

#define $fdescribe(describeName) \
static_assert(casmine::length(describeName) > 0, "describe name must not be empty"); \
$DEFINE_IMPL \
$DESCRIBE_INIT(describeName, casmine::ItemRuntimeSpec::Focused) \
void DescribeImpl::runDescribe()

#define $xdescribe(describeName) \
static_assert(casmine::length(describeName) > 0, "describe name must not be empty"); \
$DEFINE_IMPL \
$DESCRIBE_INIT(describeName, casmine::ItemRuntimeSpec::Disabled) \
void DescribeImpl::runDescribe()

#define $it addIt<_N_, __LINE__, casmine::ItemRuntimeSpec::Normal>
#define $fit addIt<_N_, __LINE__, casmine::ItemRuntimeSpec::Focused>
#define $xit addIt<_N_, __LINE__, casmine::ItemRuntimeSpec::Disabled>

#define $beforeAll beforeAll<_N_, __LINE__>
#define $afterAll afterAll<_N_, __LINE__>
#define $beforeEach beforeEach<_N_, __LINE__>
#define $afterEach afterEach<_N_, __LINE__>

// $expect/$fail calls can be located in different files than their surrounding $describe().
// That's why we have to log the current file for each call.
#define $expect(value) LocalEnvironment::makeExpect(_N_, __LINE__, value)
#define $fail(reason) casmine::CasmineContext::get()->recordFailure(_N_, __LINE__, reason);
#define $success() casmine::CasmineContext::get()->recordSuccess(_N_, __LINE__);

#define $pending(reason) casmine::CasmineContext::get()->markPending(reason)

class CasmineContext {
public:
  virtual ~CasmineContext();
  std::map<std::string, ConfigMapVariableTypes> settings;
  rapidjson::Document configuration;

  static CasmineContext* get();

  void addInitializer(DescribeInit *initializer);

  void addReporter(std::unique_ptr<Reporter> reporter);
  void clearReporters();

  void sendStartEvent(std::size_t totalSpecCount);
  void sendModuleStartEvent(TestResult const& result, std::size_t specCount);
  void sendSpecStartEvent(TestResult const& result);
  void sendSpecDoneEvent(TestResult const& result);
  void sendModuleDoneEvent(TestResult const& result);
  void sendDoneEvent(std::chrono::microseconds name);

  void runTests(std::string const& baseDir);

  void recordSuccess(const char *file, size_t line);
  void recordFailure(const char *file, size_t line, std::string const& message);

  void forceSpecList(std::vector<std::string> const& list);
  std::vector<std::string> getSpecList();

  void markPending(std::string const& reason);

  std::string baseDir() { return _baseDir; };
  std::string outputDir() { return _baseDir + "/output"; }
  std::string tmpDataDir() { return _baseDir + "/tmpdata"; }

  std::string getConfigurationStringValue(std::string const& path, std::string const& defaultValue = "") const;
  int getConfigurationIntValue(std::string const& path, int defaultValue = 0) const;
  double getConfigurationDoubleValue(std::string const& path, double defaultValue = 0.0) const;
  bool getConfigurationBoolValue(std::string const& path, bool defaultValue = false) const;

protected:
  CasmineContext();

private:
  // These objects create the actual test object on demand.
  std::vector<DescribeInit *> _initializers;

  std::vector<std::unique_ptr<Reporter>> _reporters;
  Describe *_currentDescribe = nullptr;
  std::string _baseDir;

  std::mutex _resultMutex; // Synchronizes result recording across threads.

  // Ping support, to ensure regular output also for long lasting tests.
  std::atomic<bool> _stopPing; // Set to true when the ping thread has to stop.
  size_t _pingCount = 0;       // Counter for intermittant line breaks and the initial message.
  std::timed_mutex _pingMutex; // The "semaphore" to signal new output.
  std::thread _pingThread;     // The background thread to print the ping dots.

  void startPing();
  void resetPing();

  rapidjson::Value const* getConfigValueFromPath(std::string const& path) const;

  // The specs that were given on the command line.
  std::vector<std::string> _specForceList;
};

};
