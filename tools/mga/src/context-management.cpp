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

#include "debugger.h"

#include "accessible.h"
#include "platform.h"
#include "uielement.h"

#include "filesystem.h"
#include "path.h"
#include "utilities.h"

#include "context-management.h"
#include "property.h"

using namespace mga;
using namespace aal;

using namespace std::literals::chrono_literals;

//----------------- AutomationContext ----------------------------------------------------------------------------------

AutomationContext::AutomationContext(const std::string &name, const std::vector<std::string> &params,  bool newInstance,
                                     ShowState showState, std::chrono::milliseconds timeout) {

  _pid = Platform::get().launchApplication(name, params, newInstance, showState);

  bool timeoutReached = false;
  std::future<bool> startup = std::async(std::launch::async, [&]() {
    AccessibleRef acc;
    while (!timeoutReached && (!acc || !acc->isValid())) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      acc = Accessible::getByPid(_pid);
    }

    // The memory of this instance is *not* managed by the context, but by a JS property we set on its JS equivalent.
    _root = new UIRootElement(*this, std::move(acc));
    return true;
  });

  if (startup.wait_for(timeout) == std::future_status::timeout)
    timeoutReached = true;

  if (timeoutReached) {
    Platform::get().terminate(_pid, true);
    throw std::runtime_error("Unable to get Accessibility Context for the application: " + name + " [" +
      std::to_string(_pid) + "]");
  }
}

//----------------------------------------------------------------------------------------------------------------------

AutomationContext::AutomationContext(const int pid) {
  _root = new UIRootElement(*this, Accessible::getByPid(pid));
  if (_root->isValid())
    _pid = pid;
  else
    _pid = 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Registers property access for a UI element in our statistics (if enabled).
 */
void AutomationContext::incStatCount(UIElement *element, StatCounterType type, Property property) {
  if (_collectStatistics) {
    std::stack<std::pair<std::string, aal::Role>> path;
    path.push({ element->getName(), element->getRole() });
    if (!_root->equals(element)) {
      auto run = element->getParent();
      while (true) {
        path.push({ run->getName(), run->getRole() });
        if (_root->equals(run.get()))
          break;

        run = run->getParent();
      }
    }

    ElementStatistics *stats = &_statistics;
    std::pair<ElementStatistics::iterator, bool> lookup;
    while (true) {
      lookup = stats->insert({ path.top().first, { path.top().second, {}, {}}});
      path.pop();
      if (path.empty())
        break;
      stats = &lookup.first->second.children;
    }

    ++lookup.first->second.counts[static_cast<size_t>(property)][static_cast<size_t>(type)];
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AutomationContext::registerInContext(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("AutomationContext", "", JSExport::VarArgs, constructor, setupPrototype);
}

//----------------------------------------------------------------------------------------------------------------------

void AutomationContext::constructor(JSObject *instance, JSValues &args) {

  // Extract the arguments we pass on to the application to be started.
  size_t applicationArgCount = args.size() - 2;
  std::vector<std::string> params;

  AutomationContext* ac = nullptr;
  if (args.is(ValueType::String, 0)) {
    std::string name = args.get(0);

    auto timeout = 5000ms;
    bool newInstance = false;
    ShowState showState = ShowState::Normal;

    if (args.is(ValueType::Object, 1)) {
      JSObject options = args.get(1);
      int value = options.get("showState", static_cast<int>(ShowState::Normal));
      showState = static_cast<ShowState>(value);
      newInstance = options.get("newInstance", false);

      double t = options.get("timeout", 5.0);
      timeout = std::chrono::milliseconds(static_cast<long long>(t * 1000));
    }

    if (applicationArgCount > 0) {
      for (size_t i = 0; i < applicationArgCount; ++i) {
        params.push_back(args.get(i + 2));
      }
    }

    // Create the context instance and set the root property for it.
    try {
      ac = new AutomationContext(name, params, newInstance, showState, timeout);
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  } else if (args.is(ValueType::Int, 0)) {
    int pid = args.get(0);

    try {
      ac = new AutomationContext(pid);
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  } else {
    args.context()->throwScriptingError(ScriptingError::Error, "Unhandled argument type");
  }

  instance->setBacking(ac);

  // Create a JS UIElement instance with our root as native reference.
  auto root = args.context()->createJsInstance("UIElement", { ac->_root });
  instance->defineProperty("root", root);
}

//----------------------------------------------------------------------------------------------------------------------

void AutomationContext::enumerateValues(ScriptingContext *context, ElementCounters const& counters, JSObject &target) {
  JSObject properties(context);
  size_t count = 0;
  for (size_t i = 0; i < static_cast<size_t>(Property::PropertyCount); ++i) {
    Property property = static_cast<Property>(i);
    auto values = counters.counts[i];
    if (values[0] > 0 || values[1] > 0 || values[2] > 0) {
      ++count;

      JSArray counts(context);
      counts.addValues({ values[0], values[1], values[2] });
      properties.set(propertyToString(property), counts);
    }
  }

  if (count > 0)
    target.set("properties", properties);

  if (!counters.children.empty()) {
    JSObject children(context);
    for (auto entry : counters.children) {
      JSObject child(context);
      enumerateValues(context, entry.second, child);
      children.set(entry.first.empty() ? "<no name>" : entry.first, child);
    }

    target.set("children", children);
  }

  target.set("type", roleToJSType(counters.role));
};

//----------------------------------------------------------------------------------------------------------------------

void AutomationContext::setupPrototype(JSObject &prototype) {
  prototype.defineVirtualProperty("running", [](ScriptingContext *, JSExport *ac, std::string const&) {
    auto aContext = dynamic_cast<AutomationContext *>(ac);
    if (aContext->_pid == 0)
      return false;

    // However if the pid is set, we need to check if maybe it's some leftover and update it.
    bool isRunning = Platform::get().isRunning(aContext->_pid);
    if (!isRunning) {
      aContext->_pid = 0;
    }
    return isRunning;
  }, nullptr);

  prototype.defineVirtualProperty("collectStatistics", [](ScriptingContext *, JSExport *ac, std::string const&) {
    return dynamic_cast<AutomationContext *>(ac)->_collectStatistics;
  }, [](ScriptingContext *, JSExport *ac, std::string const&, JSVariant const& value) {
    dynamic_cast<AutomationContext *>(ac)->_collectStatistics = value;
  });

  prototype.defineFunction({ "equals" }, 1, [](JSExport *ac, JSValues &args) {
    if (!args.is(ValueType::Object, 0)) {
      args.pushResult(false);
      return;
    }

    JSObject object = args.get(0);
    AutomationContext *other = dynamic_cast<AutomationContext *>(object.getBacking());
    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    if (other == nullptr || me == nullptr)
      args.pushResult(false);
    else {
      args.pushResult(me->_pid == other->_pid);
    }
  });

  prototype.defineFunction({ "terminate" }, 1, [](JSExport *ac, JSValues &args) {
    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    if (me->_pid == 0) {
      args.context()->throwScriptingError(ScriptingError::Error, "Application is not running");
      return;
    }

    bool success = Platform::get().terminate(me->_pid);
    if (success)
      me->_pid = 0;

    args.pushResult(success);
  });

  prototype.defineFunction({ "elementAtPoint" }, 1, [](JSExport *ac, JSValues &args) {
    // parameters: point in screen coordinates
    geometry::Point point = args.getPoint(0);
    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);

    UIElementRef element = UIElement::fromPoint(point, me->_root);
    if (element) {
      JSObject jsElement = args.context()->createJsInstance("UIElement", { element.release() });
      args.pushResult(jsElement);
    }
  });

  prototype.defineFunction({ "windows" }, 0, [](JSExport *ac, JSValues &args) {
    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    auto result = me->_root->windows();

    JSArray array(args.context());
    for (size_t i = 0; i < result.size(); ++i) {
      JSObject jsElement = args.context()->createJsInstance("UIWindow", { result[i].release() });
      array.addValue(jsElement);
    }
    args.pushResult(array);
  });

  prototype.defineFunction({ "mouseDown" }, 2, [](JSExport *ac, JSValues &args) {
    // parameters: point, button
    geometry::Point point = args.getPoint(0);
    int t = args.get(1, 0);
    MouseButton button = static_cast<MouseButton>(t);

    if (button != MouseButton::Left && button != MouseButton::Middle && button != MouseButton::Right) {
      args.context()->throwScriptingError(ScriptingError::Error, "Invalid mouse button specified");
    }

    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    me->_root->_accessible->mouseDown(point, button);
  });

  prototype.defineFunction({ "mouseUp" }, 2, [](JSExport *ac, JSValues &args) {
    // parameters: point, button
    geometry::Point point = args.getPoint(0);
    int t = args.as(ValueType::Int, 1);
    MouseButton button = static_cast<MouseButton>(t);

    if (button != MouseButton::Left && button != MouseButton::Middle && button != MouseButton::Right) {
      args.context()->throwScriptingError(ScriptingError::Error, "Invalid mouse button specified");
    }

    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    me->_root->_accessible->mouseUp(point, button);
  });

  prototype.defineFunction({ "mouseMoveBy" }, 1, [](JSExport *ac, JSValues &args) {
    // parameters: distance
    geometry::Point distance = args.getPoint(0);

    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    me->_root->_accessible->mouseMove(distance);
  });

  prototype.defineVirtualProperty("mousePosition", [](ScriptingContext *, JSExport *ac, std::string const&) {
    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    return me->_root->_accessible->getMousePosition();
  }, [](ScriptingContext *, JSExport *ac, std::string const&, JSVariant const& value) {
    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    geometry::Point point;
    JSObject argument = value;
    point.x = argument.get("x", 0);
    point.y = argument.get("y", 0);
    me->_root->_accessible->mouseMoveTo(point);
  });

  prototype.defineFunction({ "mouseDrag" }, 3, [](JSExport *ac, JSValues &args) {
    // parameters: point, point, button
    geometry::Point source = args.getPoint(0);
    geometry::Point target = args.getPoint(1);
    int t = args.get(2, 0);
    MouseButton button = static_cast<MouseButton>(t);

    if (button != MouseButton::Left && button != MouseButton::Middle && button != MouseButton::Right) {
      args.context()->throwScriptingError(ScriptingError::Error, "Invalid mouse button specified");
    }

    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    me->_root->_accessible->mouseDrag(source, target, button);
  });

  prototype.defineFunction({ "keyDown" }, 2, [](JSExport *ac, JSValues &args) {
    // parameters: key, modifier?
    int key = args.as(ValueType::Int, 0);
    size_t modifier = 0;
    if (args.isNumber(1))
      modifier = args.as(ValueType::UInt, 1);

    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    me->_root->_accessible->keyDown(static_cast<aal::Key>(key), modifierFromNumber(modifier));
  });

  prototype.defineFunction({ "keyUp" }, 2, [](JSExport *ac, JSValues &args) {
    // parameters: key, modifier?
    int key = args.as(ValueType::Int, 0);
    size_t modifier = 0;
    if (args.isNumber(1))
      modifier = args.as(ValueType::UInt, 1);

    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    me->_root->_accessible->keyUp(static_cast<aal::Key>(key), modifierFromNumber(modifier));
  });

  prototype.defineFunction({ "keyPress" }, 2, [](JSExport *ac, JSValues &args) {
    // parameters: key, modifier?
    int key = args.as(ValueType::Int, 0);
    size_t modifier = 0;
    if (args.isNumber(1))
      modifier = args.as(ValueType::UInt, 1);

    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    me->_root->_accessible->keyPress(static_cast<aal::Key>(key), modifierFromNumber(modifier));
  });

  prototype.defineFunction({ "typeString" }, 1, [](JSExport *ac, JSValues &args) {
    // parameters: string
    std::string input = args.as(ValueType::String, 0);

    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    me->_root->_accessible->typeString(input);
  });

  prototype.defineFunction( { "highlight" }, 1, [](JSExport *ac, JSValues &args) {
    // parameter: UIElement?
    JSVariant object = args.get(0);
    if (!object.isValid()) {
      AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
      me->_root->_accessible->removeHighlight();
      return;
    }

    UIElement *target = dynamic_cast<UIElement *>(static_cast<JSObject>(object).getBacking());
    if (target != nullptr)
      target->_accessible->highlight();
  });

  prototype.defineFunction({ "statistics" }, 0, [](JSExport *ac, JSValues &args) {
    AutomationContext *me = dynamic_cast<AutomationContext *>(ac);
    JSObject result(args.context());

    for (auto entry : me->_statistics) {
      JSObject object(args.context());
      enumerateValues(args.context(), entry.second, object);
      result.set(entry.first, object);
    }

    args.pushResult(result);
  });

}

//----------------- JSContext ------------------------------------------------------------------------------------------

bool JSContext::stopRunloop = false;

JSContext::JSContext(bool debugMode) : _enableDebugMode(debugMode) {
}

//----------------------------------------------------------------------------------------------------------------------

JSContext::~JSContext() {
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * The runloop executes as long as there are timers waiting. It can however be stopped prematurly by setting
 * stopRunloop to true;
 */
void JSContext::runEventLoop() {
  try {
    _context.bootstrap(_mainScript);
    Platform::get().runLoopRun(_context);
  } catch (std::runtime_error &e) {
    std::cerr << "Error encountered: " << _mainScript << std::endl << e.what() << std::endl;
    Process::exitCode = ExitCode::RunLoopError;
  } catch (...) {
    _context.checkForErrors();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void JSContext::onExit() {
  Process::onExit(_context);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Load settings and set up the JS context.
 */
void JSContext::initialize(std::string const& configFile) {
  // Set up the mga global object (most content comes from config file).
  if (!FS::isFile(configFile))
    throw std::runtime_error("The configuration file does not exist in this path: " + configFile);

  JSObject json = _context.loadJsonFile(configFile);

  // Switch to work dir (which is relative to the config file, if not absolute).
  std::string path = json.as(ValueType::String, "workDir");
  if (!path.empty()) {
    Process::chdir(Path::resolve({ Path::dirname(configFile), path }));
  }

  std::string t = json.get("main");
  _mainScript = t;
  std::string rootPath = Path::dirname(Path::resolve({ t }));

  // Make include paths absolute to avoid trouble when a script changes the current workdir.
  JSArray includes = json.get("includes", JSArray());
  if (includes.isValid() && includes.size() > 0) {
    for (size_t i = 0; i < includes.size(); ++i) {
      std::string include = includes.get(i);
      include = Path::resolve({ rootPath, include });
      includes.setValue(i, include);
    }
  }

  std::string directory = Path::resolve({ json.get("dataDir", "") });
  json.defineProperty("dataDir", directory); // Overwrite the exiting entry. Make it read-only too.
  directory = Path::resolve({ json.get("outputDir", "") });
  json.defineProperty("outputDir", directory);

  JSObject mga(&_context);
  {
    JSGlobalObject global(&_context);
    global.defineProperty("mga", mga);
    mga.defineProperty("config", json);
  }
  mga.defineProperty("rootDir", rootPath);

  // Register global modules. "events" is actually not global, but "Process" derives from "EventEmitter"
  // (and the "process" module *is* global, just as "stream" is).
  _context.evaluate("require('global');require('events');require('stream');require('process');");

  // Finally export classes that are bound to the mga module/namespace.
  AutomationContext::registerInContext(_context, mga);
  UIElement::registerInContext(_context, mga);

  mga.defineFunction({ "screens" }, 0, [](JSExport *ac, JSValues &args) {
    std::ignore = ac;
    JSArray screens(args.context());
    for (auto &screen : Platform::get().getScreens()) {
      JSObject entry(args.context());
      entry.defineProperty("bounds", screen.bounds);
      entry.defineProperty("resolutionX", screen.resolutionX);
      entry.defineProperty("resolutionY", screen.resolutionY);
      entry.defineProperty("scaleFactor", screen.scaleFactor);
      screens.addValue(entry);
    }
    args.pushResult(screens);
  });

  mga.defineFunction({ "getClipboardText" }, 0, [](JSExport *ac, JSValues &args) {
    std::ignore = ac;
    args.pushResult(Platform::get().getClipboardText());
  });

  mga.defineFunction({ "setClipboardText" }, 1, [](JSExport *ac, JSValues &args) {
    std::ignore = ac;
    Platform::get().setClipboardText(args.get(0));
  });


  if (_enableDebugMode)
    _debugAdapter.reset(new DebugAdapter(&_context));
}

//----------------------------------------------------------------------------------------------------------------------

