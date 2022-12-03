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

#include "ansi-styles.h"
#include "casmine.h"

using namespace casmine;
using namespace rapidjson;

using namespace std::chrono;

#ifdef _MSC_VER
#undef GetObject
#endif


//----------------------------------------------------------------------------------------------------------------------

CasmineContext* CasmineContext::get() {
  static std::unique_ptr<CasmineContext> instance(new CasmineContext());
  return instance.get();
}

//----------------------------------------------------------------------------------------------------------------------

CasmineContext::CasmineContext() {
  _stopPing = false;
  _reporters.push_back(std::make_unique<ConsoleReporter>());
  _reporters.push_back(std::make_unique<JsonReporter>());

  settings["continueOnException"] = false;
  settings["verbose"] = false;
  settings["no-colors"] = false;
  settings["only-selected"] = false;
}

//----------------------------------------------------------------------------------------------------------------------

CasmineContext::~CasmineContext() {
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::addInitializer(DescribeInit *initializer) {
  _initializers.push_back(initializer);
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::addReporter(std::unique_ptr<Reporter> reporter) {
  _reporters.push_back(std::move(reporter));
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::clearReporters() {
  _reporters.clear();
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::sendStartEvent(std::size_t totalSpecCount) {
  for (auto &reporter : _reporters) {
    reporter->casmineStarted(totalSpecCount);
  }
  resetPing();
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::sendModuleStartEvent(TestResult const& result, std::size_t specCount) {
  for (auto &reporter : _reporters) {
    reporter->moduleStarted(result, specCount);
  }
  resetPing();
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::sendSpecStartEvent(TestResult const& result) {
  for (auto &reporter : _reporters) {
    reporter->specStarted(result);
  }
  resetPing();
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::sendSpecDoneEvent(TestResult const& result) {
  for (auto &reporter : _reporters) {
    reporter->specDone(result);
  }
  resetPing();
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::sendModuleDoneEvent(TestResult const& result) {
  for (auto &reporter : _reporters) {
    reporter->moduleDone(result);
  }
  resetPing();
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::sendDoneEvent(microseconds duration) {
  for (auto &reporter : _reporters) {
    reporter->casmineDone(duration);
  }
  resetPing();
}

//----------------------------------------------------------------------------------------------------------------------

#ifndef __APPLE__
  #include <filesystem>

  namespace fs = std::filesystem;
#endif

void CasmineContext::runTests(std::string const& baseDir) {
  _baseDir = baseDir;
  bool onlySelected = std::get<bool>(settings["only-selected"]);

  std::cout << styleGrayOn << styleItalicOn << "Preparing test data..." << stylesReset << std::endl << std::endl;

#ifdef __APPLE__
  
  // The filesystem header is not available yet on macOS.
  std::string command = "rm -Rf " + outputDir();
  system(command.c_str());
  command = "rm -Rf " + tmpDataDir();
  system(command.c_str());

  command = "mkdir -p -m770 " + outputDir();
  system(command.c_str());

  command = "cp -R " + _baseDir + "/data " + tmpDataDir();
  system(command.c_str());

#else

  std::error_code error;
  fs::remove_all(tmpDataDir(), error); // Ignore any error.
  fs::remove_all(outputDir(), error);

  fs::create_directories(outputDir());
  fs::copy(_baseDir + "/data", tmpDataDir(), fs::copy_options::recursive);

#endif

  high_resolution_clock::time_point start = high_resolution_clock::now();

  std::size_t totalSpecCount = 0;
  bool haveFocused = false;
  std::list<Describe *> describes;
  std::map<std::string, Describe*> forcedOrder;

  for (DescribeInit *entry : _initializers) {
    _currentDescribe = entry->initialize(); // Does not allocate any test data.

    totalSpecCount += _currentDescribe->specCount();

    if (!_specForceList.empty()) {
      auto iter = std::find(_specForceList.begin(), _specForceList.end(), entry->getName());
      if (iter != _specForceList.end()) {
        _currentDescribe->markForced();
        forcedOrder.insert({entry->getName(), _currentDescribe});
      } else {
        _currentDescribe->markDisabled();
        describes.push_back(_currentDescribe);
      }
    } else {
      if (!onlySelected && _currentDescribe->isFocused())
        haveFocused = true;

      describes.push_back(_currentDescribe);
    }
  }

  if (onlySelected) {
    for (auto rit = _specForceList.rbegin(); rit != _specForceList.rend(); rit++) {
      describes.push_front(forcedOrder.at(*rit));
    }
  }

  startPing();
  sendStartEvent(totalSpecCount);

  for (Describe *describe : describes) {
    _currentDescribe = describe;
    describe->run(haveFocused);
    delete describe;
  }

  auto duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
  sendDoneEvent(duration);

  if (_pingThread.joinable()) {
    _pingMutex.unlock();
    _stopPing = true;
    _pingThread.join();
  }

  std::cout << styleGrayOn << styleItalicOn << "Cleaning up test data..." << stylesReset << std::endl << std::endl; 

#ifdef __APPLE__

  command = "rm -R " + outputDir();
  system(command.c_str());
  command = "rm -R " + tmpDataDir();
  system(command.c_str());

#else

  fs::remove_all(tmpDataDir()); // Will throw in case of errors.
  fs::remove_all(outputDir());

#endif

  std::cout << styleGrayOn << styleItalicOn << "Done." << stylesReset << std::endl << std::endl; 
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::forceSpecList(std::vector<std::string> const& list) {
  _specForceList = list;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> CasmineContext::getSpecList() {
  std::vector<std::string> list;
  for (const auto &it: _initializers) {
    if (it->getType() == ItemRuntimeSpec::Normal || it->getType() == ItemRuntimeSpec::Focused) {
      list.push_back(it->getName());
    }
  }
  return list;
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::recordSuccess(const char *file, size_t line) {
  std::lock_guard<std::mutex> lock(_resultMutex);

  if (_currentDescribe != nullptr)
    _currentDescribe->recordSuccess(file, line);
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::recordFailure(const char *file, size_t line, std::string const& message)  {
  std::lock_guard<std::mutex> lock(_resultMutex);

  if (_currentDescribe != nullptr)
    _currentDescribe->recordFailure(file, line, message);
}

//----------------------------------------------------------------------------------------------------------------------

void CasmineContext::markPending(std::string const& reason) {
  std::lock_guard<std::mutex> lock(_resultMutex);
  
  if (_currentDescribe != nullptr)
    _currentDescribe->markPending(reason);
}

//----------------------------------------------------------------------------------------------------------------------

std::string CasmineContext::getConfigurationStringValue(std::string const& path, std::string const& defaultValue) const {
  rapidjson::Value const* value = getConfigValueFromPath(path);
  if (value == nullptr || !value->IsString())
    return defaultValue;
  return value->GetString();
}

//----------------------------------------------------------------------------------------------------------------------

int CasmineContext::getConfigurationIntValue(std::string const& path, int defaultValue) const {
  rapidjson::Value const* value = getConfigValueFromPath(path);
  if (value == nullptr || !value->IsInt())
    return defaultValue;
  return value->GetInt();
}

//----------------------------------------------------------------------------------------------------------------------

double CasmineContext::getConfigurationDoubleValue(std::string const& path, double defaultValue) const {
  rapidjson::Value const* value = getConfigValueFromPath(path);
  if (value == nullptr || !value->IsDouble())
    return defaultValue;
  return value->GetDouble();
}

//----------------------------------------------------------------------------------------------------------------------

bool CasmineContext::getConfigurationBoolValue(std::string const& path, bool defaultValue) const {
  rapidjson::Value const* value = getConfigValueFromPath(path);
  if (value == nullptr || !value->IsBool())
    return defaultValue;
  return value->GetBool();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Starts the ping thread if a ping timeout value > 0 was specified.
 */
void CasmineContext::startPing() {
  int pingTimeout = 2;
  if (configuration.GetObject().HasMember("reporting")) {
    auto &reporterConfig = configuration.GetObject()["reporting"];
    if (reporterConfig.HasMember("ping"))
      pingTimeout = reporterConfig["ping"].GetInt();
  }

  _pingMutex.lock();
  if (pingTimeout > 0) {
    _pingThread = std::thread([this, pingTimeout]() {
      while (!_stopPing) {
        if (!_pingMutex.try_lock_for(std::chrono::seconds(pingTimeout))) {
          // Timeout reached -> no reporter call within this time frame.
          std::cout << stylesReset;
          if (_pingCount == 0)
            std::cout << std::endl << "processing: ";
          if (++_pingCount % 50 == 0)
            std::cout << std::endl;
          std::cout << "." << std::flush;
        } else {
          _pingMutex.unlock();
          std::this_thread::sleep_for(20ms);
        }
      }
    });
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Releases the ping mutex for a moment to signal the ping thread there was output.
 */
void CasmineContext::resetPing() {
  _pingMutex.unlock();
  std::this_thread::sleep_for(10ms);
  _pingMutex.lock();
  _pingCount = 0;
}

//----------------------------------------------------------------------------------------------------------------------

Value const* CasmineContext::getConfigValueFromPath(std::string const& path) const {
  std::stringstream ss(path);
  std::string item;
  std::vector<std::string> parts;

  Value const* current = &configuration;
  while (std::getline(ss, item, '/')) {
    auto member = current->FindMember(item);
    if (member == current->MemberEnd())
      return nullptr;
    current = &member->value;
  }

  return current;
}

//----------------------------------------------------------------------------------------------------------------------
