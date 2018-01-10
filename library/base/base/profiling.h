/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _PROFILING_H_
#define _PROFILING_H_

#include "common.h"

#include <string>
#include <map>
#include <time.h>

namespace base {
  // This class has been created to provide a way to accumulate the
  // processing time spent on a specific section of the code.
  // Usage: add(id) to register a new accumulator
  //        on(id) to start accumulating time on the <id> accumulator
  //        off(if) to stop accumulating time on the <id> accumulator
  //
  // This class can be instantiated to monitor the processing time inside
  // a method or class or the static version GlobalTA below could be used
  // to monitor processing time among classes or libraries
  class BASELIBRARY_PUBLIC_FUNC TimeAccumulator {
  private:
    std::map<std::string, double> _accumulators;
    std::map<std::string, clock_t> _starts;

  public:
    void add(const std::string& id);
    void on(const std::string& id);
    void off(const std::string& id);
    void dump(const std::string& message);
    void clear();
  };

  class BASELIBRARY_PUBLIC_FUNC GlobalTA {
  private:
    static TimeAccumulator _tacc;

  public:
    static void add(const std::string& id) {
      _tacc.add(id);
    };
    static void on(const std::string& id) {
      _tacc.on(id);
    };
    static void off(const std::string& id) {
      _tacc.off(id);
    };
    static void dump(const std::string& message) {
      _tacc.dump(message);
    };
    static void clear() {
      _tacc.clear();
    };
  };

  // This class has been created to provide a way to time mark
  // a process on it's different phases
  // Usage: start(message) to record the time measuring
  //        lap(message) to mark the time used to that point in the logic
  //           since the last lap or the start
  //        stop(message) to stop the time measuring and printing the total
  //           time spent since start
  // This class can be instantiated to monitor the processing time inside
  // a method or class or the static version GlobalSW below could be used
  // to monitor processing time among classes or libraries
  class BASELIBRARY_PUBLIC_FUNC StopWatch {
  private:
    bool _initialized;
    clock_t _start;
    clock_t _lap_start;
    clock_t _end;

    std::string format_time(clock_t time);

  public:
    StopWatch() : _initialized(false) {
    }
    void start(const std::string& message);
    void lap(const std::string& message);
    void stop(const std::string& message);
  };

  class BASELIBRARY_PUBLIC_FUNC GlobalSW {
  private:
    static StopWatch _sw;

  public:
    static void start(const std::string& message) {
      _sw.start(message);
    };
    static void lap(const std::string& message) {
      _sw.lap(message);
    };
    static void stop(const std::string& message) {
      _sw.stop(message);
    };
  };
} // namespace base ends here

#endif //_PROFILING_H_
