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
#include "jsexport.h"
#include "scripting-context.h"

#include "role.h"
#include "property.h"

namespace mga {

  class UIElement;

  class DebugAdapter;

  enum StatCounterType { Read, Write, Execute };

  // The main class to manage all the automation features.
  class AutomationContext : public JSExport {
  public:

    // Pass in full path to the application to run.
    AutomationContext(const std::string &name, const std::vector<std::string> &params, bool newInstance,
                      ShowState showState, std::chrono::milliseconds timeout);
    AutomationContext(const int pid);

    UIElement* getRoot() { return _root; }

    void enableStatistics(bool flag) { _collectStatistics = flag; };
    bool statisticsEnabled() { return _collectStatistics; };
    void incStatCount(UIElement *element, StatCounterType type, Property property);

    static void registerInContext(ScriptingContext &context, JSObject &module);
    
  private:
    int _pid;
    UIElement *_root;

    // Statistics.
    struct ElementCounters;
    using ElementStatistics = std::map<std::string, ElementCounters>;
    using ValueCounter = size_t[3]; // Read, write, execute. Indexed by StatCounterType.

    struct ElementCounters {
      aal::Role role;
      ElementStatistics children;
      ValueCounter counts[static_cast<size_t>(Property::PropertyCount)]; // Indexed by the Property enum.
    };

    bool _collectStatistics = false;
    ElementStatistics _statistics;

    static void constructor(JSObject *, JSValues&);

    static void enumerateValues(ScriptingContext *context, ElementCounters const& counters, JSObject &target);
    static void setupPrototype(JSObject &prototype);
  };

  // The context class which is connected to a single duktape thread.
  class JSContext {
  public:
    static bool stopRunloop;

    JSContext(bool debugMode);
    ~JSContext();

    void initialize(std::string const& configFile);

    void runEventLoop();
    void evaluate(std::string const& input);
    void onExit();

  private:
    std::unique_ptr<DebugAdapter> _debugAdapter;
    ScriptingContext _context;
    std::string _mainScript;
    bool _enableDebugMode;
  };

} // namespace mga
