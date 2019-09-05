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

#include "jsexport.h"
#include "scripting-context.h"

#include "utilities.h"
#include "platform.h"
#include "aal/role.h"

#include "global.h"

using namespace mga;
using namespace std::chrono;

//----------------------------------------------------------------------------------------------------------------------

std::string Global::_capture;
bool Global::_capturing = false;

// A list of times where time() was called with a specific label.
static std::map<std::string, system_clock::time_point> times;

void Global::activate(ScriptingContext &context, JSObject &exports) {
  std::ignore = exports;
  JSGlobalObject global(&context);

  global.defineFunction({ "setImmediate" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    // We simply forward the call to the scripting context. It will take everything what's needed from the
    // caller stack.
    args.pushResult(args.context()->setImmediate());
  });

  global.defineFunction({ "clearImmediate" }, 1, [](JSExport *, JSValues &args) {
    args.pushResult(args.context()->clearImmediate());
  });

  global.defineFunction({ "setInterval" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    args.pushResult(args.context()->setInterval());
  });

  global.defineFunction({ "clearInterval" }, 1, [](JSExport *, JSValues &args) {
    args.pushResult(args.context()->clearInterval());
  });

  global.defineFunction({ "setTimeout" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    args.pushResult(args.context()->setTimeout());
  });

  global.defineFunction({ "clearTimeout" }, 1, [](JSExport *, JSValues &args) {
    args.pushResult(args.context()->clearTimeout());
  });

  JSObject console(&context);
  console.defineFunction({ "assert" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    // parameters: value, (message, <varargs>?)?
    bool condition = args.as(ValueType::Boolean, 0);
    if (condition)
      return;
    args.removeValue(0);

    std::string text = args.context()->logOutput("AssertionError");
    if (_capturing)
      _capture += text;
    else
      Platform::get().writeText(text, true);
  });

  console.defineFunction({ "dir" }, 2, [](JSExport *, JSValues &args) {
    // parameters: object, options
    bool showHidden = false;
    size_t maxLevel = 2;

    if (args.is(ValueType::Object, 1)) {
      JSObject options = args.get(1);
      showHidden = options.get("showHidden", false);
      maxLevel = options.get("maxLevel", 2);
    }

    if (_capturing)
      _capture += JSObject(args.get(0)).dumpObject(showHidden, maxLevel);
    else
      Platform::get().writeText(args.dumpObject(0, showHidden, maxLevel));
  });

  console.defineFunction({ "error", "warn" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    std::string text = args.context()->logOutput("Error");
    if (_capturing)
      _capture += text;
    else
      Platform::get().writeText(text, true);
  });

  console.defineFunction({ "log", "info", "debug" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    std::string text = args.context()->logOutput();
    if (_capturing)
      _capture += text;
    else
      Platform::get().writeText(text, false);
  });

  console.defineFunction({ "trace" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    std::string text = args.context()->logOutput("Trace");
    if (_capturing)
      _capture += text;
    else
      Platform::get().writeText(text, true);
  });

  console.defineFunction({ "time" }, 1, [](JSExport *, JSValues &args) {
    // parameter: label
    std::string label = args.as(ValueType::String, 0);
    times[label] = system_clock::now();
  });

  console.defineFunction({ "timeEnd" }, 1, [](JSExport *, JSValues &args) {
    // parameter: label
    std::string label = args.as(ValueType::String, 0);
    auto iterator = times.find(label);
    if (iterator == times.end())
      args.context()->throwScriptingError(ScriptingError::Error, "No timer named '" + label + "' found.");
    else {
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now() - times[label]);
      if (_capturing)
        _capture += label + ": " + Utilities::formatTime(duration) + "\n";
      else {
        std::string output = label + ": " + Utilities::formatTime(duration) + "\n";
        Platform::get().writeText(output);
      }
      times.erase(iterator);
    }
  });

  console.defineFunction({ "startCapture" }, 0, [](JSExport *, JSValues &args) {
    _capture = "";
    _capturing = true;
    std::ignore = args;
  });
  console.defineFunction({ "endCapture" }, 0, [](JSExport *, JSValues &args) {
    _capturing = false;
    std::ignore = args;
  });
  console.defineFunction({ "captured" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(_capture);
  });

  global.defineProperty("console", console);

#if defined(_MSC_VER)
  global.defineProperty("isWin", true);
  global.defineProperty("isMac", false);
  global.defineProperty("isLinux", false);
#elif defined(__APPLE__)
  global.defineProperty("isWin", false);
  global.defineProperty("isMac", true);
  global.defineProperty("isLinux", false);
#else
  global.defineProperty("isWin", false);
  global.defineProperty("isMac", false);
  global.defineProperty("isLinux", true);
#endif

  // Datatypes
  JSObject mga = global.get("mga");
  mga.defineEnum("ElementType", [](JSObject &object) {
    for (int i = 0; i < static_cast<int>(aal::Role::Sentinel); ++i) {
      std::string role = aal::roleToString(static_cast<aal::Role>(i));
      object.defineProperty(role, i);
    }
  });

  mga.defineEnum("MouseButton", [](JSObject &object) {
    object.defineProperty("Left", static_cast<int>(aal::MouseButton::Left));
    object.defineProperty("Right", static_cast<int>(aal::MouseButton::Right));
    object.defineProperty("Middle", static_cast<int>(aal::MouseButton::Middle));
  });

  mga.defineEnum("Key", [](JSObject &object) {
    for (int i = 0; i < static_cast<int>(aal::Key::Sentinel); ++i) {
      std::string key = aal::keyToString(static_cast<aal::Key>(i));
      object.defineProperty(key, i);
    }
  });

  mga.defineEnum("Modifier", [](JSObject &object) {
    object.defineProperty(aal::modifierToString(aal::Modifier::NoModifier),
                          static_cast<unsigned int>(aal::Modifier::NoModifier));
    object.defineProperty(aal::modifierToString(aal::Modifier::ShiftLeft),
                          static_cast<unsigned int>(aal::Modifier::ShiftLeft));
    object.defineProperty(aal::modifierToString(aal::Modifier::ShiftRight),
                          static_cast<unsigned int>(aal::Modifier::ShiftRight));
    object.defineProperty(aal::modifierToString(aal::Modifier::ControlLeft),
                          static_cast<unsigned int>(aal::Modifier::ControlLeft));
    object.defineProperty(aal::modifierToString(aal::Modifier::ControlRight),
                          static_cast<unsigned int>(aal::Modifier::ControlRight));
    object.defineProperty(aal::modifierToString(aal::Modifier::AltLeft),
                          static_cast<unsigned int>(aal::Modifier::AltLeft));
    object.defineProperty(aal::modifierToString(aal::Modifier::AltRight),
                          static_cast<unsigned int>(aal::Modifier::AltRight));
    object.defineProperty(aal::modifierToString(aal::Modifier::MetaLeft),
                          static_cast<unsigned int>(aal::Modifier::MetaLeft));
    object.defineProperty(aal::modifierToString(aal::Modifier::MetaRight),
                          static_cast<unsigned int>(aal::Modifier::MetaRight));
  });

  mga.defineEnum("CheckState", [](JSObject &object) {
    object.defineProperty("Unchecked", static_cast<int>(aal::CheckState::Unchecked));
    object.defineProperty("Checked", static_cast<int>(aal::CheckState::Checked));
    object.defineProperty("Indeterminate", static_cast<int>(aal::CheckState::Indeterminate));
  });

  mga.defineEnum("UiToolkit", [](JSObject &object) {
    object.defineProperty("Unknown", static_cast<unsigned int>(UiToolkit::Unknown));
    object.defineProperty("DotNet", static_cast<unsigned int>(UiToolkit::DotNet));
    object.defineProperty("Cocoa", static_cast<unsigned int>(UiToolkit::Cocoa));
    object.defineProperty("Qt", static_cast<unsigned int>(UiToolkit::Qt));
    object.defineProperty("Gtk", static_cast<unsigned int>(UiToolkit::Gtk));
  });
  
  global.defineProperty("uiToolkit", static_cast<unsigned int>(Platform::get().getUiToolkit()));

  mga.defineEnum("ShowState", [](JSObject &object) {
    object.defineProperty("Hidden", static_cast<int>(ShowState::Hidden));
    object.defineProperty("Normal", static_cast<int>(ShowState::Normal));
    object.defineProperty("Maximized", static_cast<int>(ShowState::Maximized));
    object.defineProperty("HideOthers", static_cast<int>(ShowState::HideOthers));
  });

}

//----------------------------------------------------------------------------------------------------------------------

bool Global::_registered = []() {
  ScriptingContext::registerModule("global", &Global::activate);
  return true;
}();
