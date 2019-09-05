/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting-context.h"

#include "utilities.h"
#include "events.h"

#include "child_process.h"
#include "platform.h"

using namespace mga;
using namespace std::literals::chrono_literals;

// The returned class for all process spawning functions.
class ChildProcess : public EventEmitter {
public:
  bool connected;
  int pid;

  static void registerInContext(ScriptingContext &context, JSObject &exports) {
    std::ignore = context;
    exports.defineClass("ChildProcess", "EventEmitter", 1, [](JSObject *instance, JSValues &args) {
      std::ignore = args;
      instance->setBacking(new ChildProcess());
    }, [](JSObject &prototype) {
      std::ignore = prototype;
    });
  }
};

//----------------------------------------------------------------------------------------------------------------------

void ChildProcesses::activate(ScriptingContext &context, JSObject &exports) {
  ChildProcess::registerInContext(context, exports);

  exports.defineFunction({ "exec" }, 3, [](JSExport *, JSValues &) {
    // parameters: command, options?, callback?
  });
  exports.defineFunction({ "execFile" }, 4, [](JSExport *, JSValues &) {
    // parameters: file, args?, options?, callback?
  });
  exports.defineFunction({ "fork" }, 3, [](JSExport *, JSValues &) {
    // parameters: modulePath, args?, options?
  });
  exports.defineFunction({ "spawn" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    // parameters: command, args?, options?
    std::vector<std::string> params;


    if (args.is(ValueType::String, 0)) {
      std::string name = args.get(0);
      std::string cwd = Process::cwd();
      std::map<std::string, std::string> envVars;

      if (args.size() > 1 && args.is(ValueType::Array, 1)) {
        JSArray cmdArgs = args.get(1, JSArray());
        if (cmdArgs.isValid() && cmdArgs.size() > 0) {
          params.resize(cmdArgs.size());
          for (std::size_t i = 0; i < cmdArgs.size(); i++) {
            params.push_back(cmdArgs.get(i));
          }
        }
      }

      if (args.size() > 2 && args.is(ValueType::Object, 2)) {
        JSObject options = args.get(2, JSObject());
        if (options.isValid()) {
          cwd = (std::string)options.get("cwd", Process::cwd());
          JSObject envp = options.get("env", JSObject());
          if (envp.isValid()) {
            auto keys = envp.getPropertyKeys();
            for(auto it: keys) {
              envVars[it] = (std::string)envp.get(it);
            }
          }
        }
      }
      // Create the context instance and set the root property for it.
      try {
        args.pushResult(Platform::get().launchApplication(name, params, true, mga::ShowState::Normal, envVars));
      } catch (std::runtime_error &e) {
        args.context()->throwScriptingError(ScriptingError::Error, e.what());
      }
    } else {
      args.context()->throwScriptingError(ScriptingError::Error, "Unhandled argument type");
    }
  });
  exports.defineFunction({ "execFileSync" }, 3, [](JSExport *, JSValues &) {
    // parameters: file, args?, options?
  });
  exports.defineFunction({ "execSync" }, 2, [](JSExport *, JSValues &) {
    // parameters: command, options?
  });
  exports.defineFunction({ "spawnSync" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    // parameters: command, args?, options?
    std::vector<std::string> params;

    if (args.is(ValueType::String, 0)) {
      std::string name = args.get(0);
      std::string cwd = Process::cwd();
      std::map<std::string, std::string> envVars;

      if (args.size() > 1 && args.is(ValueType::Array, 1)) {
        JSArray cmdArgs = args.get(1, JSArray());
        if (cmdArgs.isValid() && cmdArgs.size() > 0) {
          params.reserve(cmdArgs.size());
          for (std::size_t i = 0; i < cmdArgs.size(); i++) {
            params.push_back(cmdArgs.get(i));
          }
        }
      }

      if (args.size() > 2 && args.is(ValueType::Object, 2)) {
        JSObject options = args.get(2, JSObject());
        if (options.isValid()) {
          cwd = (std::string)options.get("cwd", Process::cwd());
          JSObject envp = options.get("env", JSObject());
          if (envp.isValid()) {
            auto keys = envp.getPropertyKeys();
            for(auto it: keys) {
              envVars[it] = (std::string)envp.get(it);
            }
          }
        }
      }

      // Create the context instance and set the root property for it.
      try {
        args.pushResult(Platform::get().launchApplication(name, params, true, mga::ShowState::Normal, envVars));
      } catch (std::runtime_error &e) {
        args.context()->throwScriptingError(ScriptingError::Error, e.what());
      }
    } else {
      args.context()->throwScriptingError(ScriptingError::Error, "Unhandled argument type");
    }
  });
}

//----------------------------------------------------------------------------------------------------------------------

bool ChildProcesses::_registered = []() {
  ScriptingContext::registerModule("child_process", &activate);
  return true;
}();
