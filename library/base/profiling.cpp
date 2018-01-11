/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/profiling.h"
#include "base/log.h"
#include "base/string_utilities.h"
#include <cmath>

DEFAULT_LOG_DOMAIN("Profiling")

namespace base {
  TimeAccumulator create_global_ta() {
    TimeAccumulator _ta;
    return _ta;
  }

  StopWatch create_global_sw() {
    StopWatch _sw;
    return _sw;
  }

  TimeAccumulator GlobalTA::_tacc = create_global_ta();
  StopWatch GlobalSW::_sw = create_global_sw();

  //----------------- Time Check --------------------------------------------------------
  std::string StopWatch::format_time(clock_t time) {
    float s = ((float)time) / ((float)CLOCKS_PER_SEC);
    int m = (int)(s / 60);
    s -= (m * 60);
    int h = m / 60;
    m -= (h * 60);

    return base::strfmt("%02d:%02d:%02.3f", h, m, s);
  }

  void StopWatch::start(const std::string& message) {
    _initialized = true;
    _start = clock();

    _lap_start = _start;

    logDebug("---> %s - [STARTED] %s\n", format_time(0).data(), message.data());
  }
  //-------------------------------------------------------------------------------------
  void StopWatch::lap(const std::string& message) {
    if (_initialized) {
      _end = clock();

      clock_t diff = _end - _lap_start;

      logDebug("---> %s - [LAP] %s\n", format_time(diff).data(), message.data());

      _lap_start = _end;
    }
  }

  void StopWatch::stop(const std::string& message) {
    if (_initialized) {
      _end = clock();

      clock_t diff = _end - _start;

      logDebug("---> %s - [COMPLETED] %s\n", format_time(diff).data(), message.data());
    }
  }

  //----------------- Time Profiler -----------------------------------------------------
  void TimeAccumulator::add(const std::string& id) {
    _accumulators[id] = 0;
    _starts[id] = 0;
  }
  //-------------------------------------------------------------------------------------
  void TimeAccumulator::on(const std::string& id) {
    clock_t current = clock();
    _starts[id] = current;
  }
  //-------------------------------------------------------------------------------------
  void TimeAccumulator::off(const std::string& id) {
    clock_t current = clock();

    double diff = current - _starts[id];
    double acc = _accumulators[id];
    acc += diff;
    _accumulators[id] = acc;
  }
  //-------------------------------------------------------------------------------------
  void TimeAccumulator::dump(const std::string& message) {
    std::map<std::string, double>::const_iterator index, end = _accumulators.end();

    logDebug("Dumping data for : %s\n", message.data());

    for (index = _accumulators.begin(); index != end; index++) {
      logDebug("--->Time on accumulator %s : %lf\n", index->first.data(), index->second / CLOCKS_PER_SEC);
    }
  }
  //-------------------------------------------------------------------------------------
  void TimeAccumulator::clear() {
    _accumulators.clear();
    _starts.clear();
  }
  //-------------------------------------------------------------------------------------
} // namespace base